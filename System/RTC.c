#include "main.h"
#include "Hardware.h"

/* SCH_V31 的 DS1302 三根线：SCK=P1.7，CE(RST)=P1.3，IO=P2.3 */
sbit DS1302_CE  = P1^3;
sbit DS1302_SCK = P1^7;
sbit DS1302_IO  = P2^3;

/* 诊断用：最近一次读到的 6 个寄存器原值，以及开机走了哪条分支
   0=数据正常 1=CH=1 写默认 2=数据非法写默认 3=秒寄存器读不合法(芯片没应答) */
uchar RTC_Raw[6];
uchar RTC_InitPath = 0;

/* 最近一次读到合法时间时的快照。读失败就拿它顶上：
   屏上宁可停在最后一个有效时间，也不要停在 00-00-00 让人以为时钟坏了。 */
static Time_T s_last = { 26, 1, 1, 12, 0, 0 };

/* 1 = 最近一次 RTC_GetTime 读到了合法时间；0 = 没应答/数据非法。
   主程序用它决定数码管显示时间还是显示诊断码。 */
uchar RTC_Ok = 0;

/* DS1302 在 SCLK 上升沿收数据、下降沿发数据。
   原来半个时钟只有一两条指令（不到 0.3us），低于手册要求的 1us 周期，
   读回值会随机变成 0xFF/0xA5 这类垃圾，所以每个半周期补 2us。 */
static void Rtc_WriteByte(uchar dat)
{
	uchar i;

	for (i = 0; i < 8; i++)
	{
		DS1302_SCK = 0;
		Delay_Us(2);
		DS1302_IO = (bit)(dat & 0x01);
		dat >>= 1;
		Delay_Us(2);
		DS1302_SCK = 1;
		Delay_Us(2);
	}
}

static uchar Rtc_ReadByte(void)
{
	uchar i;
	uchar dat = 0;

	for (i = 0; i < 8; i++)
	{
		DS1302_SCK = 0;			/* 下降沿后 DS1302 把这一位送到 IO */
		Delay_Us(2);
		dat >>= 1;
		if (DS1302_IO)
		{
			dat |= 0x80;
		}
		Delay_Us(2);
		DS1302_SCK = 1;
		Delay_Us(2);
	}
	return dat;
}

static void Rtc_WriteReg(uchar addr, uchar dat)
{
	DS1302_CE = 0;
	DS1302_SCK = 0;
	Delay_Us(2);
	DS1302_CE = 1;
	Delay_Us(2);
	Rtc_WriteByte(addr);
	Rtc_WriteByte(dat);
	DS1302_CE = 0;
	Delay_Us(2);
}

static uchar Rtc_ReadReg(uchar addr)
{
	uchar dat;

	DS1302_CE = 0;
	DS1302_SCK = 0;
	Delay_Us(2);
	DS1302_CE = 1;
	Delay_Us(2);
	Rtc_WriteByte((uchar)(addr | 0x01));
	DS1302_IO = 1;				/* 释放数据线，让 DS1302 驱动 */
	Delay_Us(2);
	dat = Rtc_ReadByte();
	DS1302_CE = 0;
	Delay_Us(2);
	return dat;
}

static uchar Dec2Bcd(uchar v)
{
	return (uchar)(((v / 10) << 4) | (v % 10));
}

static uchar Bcd2Dec(uchar v)
{
	return (uchar)(((v >> 4) * 10) + (v & 0x0F));
}

static bit Rtc_TimeValid(Time_T *t)
{
	if (t->year > 99)					return 0;
	if (t->month < 1 || t->month > 12)	return 0;
	if (t->day < 1 || t->day > 31)		return 0;
	if (t->hour > 23)					return 0;
	if (t->min > 59)					return 0;
	if (t->sec > 59)					return 0;
	return 1;
}

/* 一次性读 6 个寄存器；读出来不是合法时间就重读，最多 3 次。
   成功：写回 t，更新 s_last，RTC_Ok=1。
   失败：把 s_last 顶给 t（不留 00-00-00），RTC_Ok=0，由主程序显示诊断码。 */
void RTC_GetTime(Time_T *t)
{
	Time_T tmp;
	uchar n;
	bit ok = 0;

	ET0 = 0;		/* 位操作期间停掉扫描：中断里要写 P0/P2 锁存器，会打断时序 */
	Seg_AllOff();	/* 扫描停摆这 ~0.7ms 先关屏，否则最后点亮的那位会亮到临界区结束（看起来像闪一下） */
	for (n = 0; n < 3; n++)
	{
		tmp.sec   = Bcd2Dec((uchar)(Rtc_ReadReg(0x81) & 0x7F));
		tmp.min   = Bcd2Dec(Rtc_ReadReg(0x83));
		tmp.hour  = Bcd2Dec((uchar)(Rtc_ReadReg(0x85) & 0x3F));
		tmp.day   = Bcd2Dec(Rtc_ReadReg(0x87));
		tmp.month = Bcd2Dec(Rtc_ReadReg(0x89));
		tmp.year  = Bcd2Dec(Rtc_ReadReg(0x8D));
		if (Rtc_TimeValid(&tmp))
		{
			ok = 1;
			break;
		}
	}
	ET0 = 1;

	if (ok)
	{
		*t     = tmp;
		s_last = tmp;
		RTC_Ok = 1;
	}
	else
	{
		*t     = s_last;
		RTC_Ok = 0;
	}
}

void RTC_SetTime(Time_T *t)
{
	ET0 = 0;
	Seg_AllOff();
	Rtc_WriteReg(0x8E, 0x00);					/* 关写保护 */
	Rtc_WriteReg(0x80, Dec2Bcd(t->sec));		/* 写秒同时清 CH 位，启动振荡 */
	Rtc_WriteReg(0x82, Dec2Bcd(t->min));
	Rtc_WriteReg(0x84, Dec2Bcd(t->hour));
	Rtc_WriteReg(0x86, Dec2Bcd(t->day));
	Rtc_WriteReg(0x88, Dec2Bcd(t->month));
	Rtc_WriteReg(0x8C, Dec2Bcd(t->year));
	Rtc_WriteReg(0x8E, 0x80);					/* 开写保护 */
	ET0 = 1;

	s_last = *t;
}

/* 读秒寄存器并做 BCD 合法性检查，读不到合法值就重试。
   原来的判据是 (s & 0x7F) <= 0x59，那是按“十进制 0~89”在比，
   0x1A（BCD 里非法的 A）这类垃圾值也能混过去；改成按位判 BCD：
   低 4 位 <= 9、高 3 位 <= 5（bit7 是 CH，单独判）。 */
static uchar Rtc_ReadSecondsRaw(void)
{
	uchar s = 0;
	uchar n;

	ET0 = 0;
	for (n = 0; n < 3; n++)
	{
		s = Rtc_ReadReg(0x81);
		if (((s & 0x0F) <= 0x09) && (((s >> 4) & 0x07) <= 0x05))
		{
			break;
		}
	}
	ET0 = 1;
	return s;
}

void RTC_Init(void)
{
	Time_T t;
	uchar s;

	s = Rtc_ReadSecondsRaw();

	/* 秒寄存器读 3 次都不是合法 BCD：三线悬空读回 0xFF，或者时序对不上。
	   这种情况写默认时间也没用（写不进去），直接标出来让屏上报诊断码。 */
	if (((s & 0x0F) > 0x09) || (((s >> 4) & 0x07) > 0x05))
	{
		RTC_InitPath = 3;
		return;
	}

	if (s & 0x80)		/* CH=1：振荡器停着，说明掉电后没走过 */
	{
		RTC_InitPath = 1;
		t.year = 26; t.month = 1; t.day = 1;
		t.hour = 12; t.min = 0; t.sec = 0;
		RTC_SetTime(&t);
		return;
	}

	RTC_GetTime(&t);
	if (!RTC_Ok)		/* 电池没电时寄存器可能是乱码 */
	{
		RTC_InitPath = 2;
		t.year = 26; t.month = 1; t.day = 1;
		t.hour = 12; t.min = 0; t.sec = 0;
		RTC_SetTime(&t);
		return;
	}
	RTC_InitPath = 0;
}

/* 诊断：把 DS1302 的三根线当普通 IO 用一遍，看板子上有没有把引脚钳死。
   准双向口写 1 就是弱上拉、写 0 就是强下拉，所以“写什么读回来就应该是什么”；
   读回对不上，说明那个脚被板子上的别的东西拉住了（跳线帽、限流电阻拉到地、
   或者和别的外设共用了）。

   bit0 = IO (P2.3) 写完 1 真的能读到高
   bit1 = IO (P2.3) 写完 0 真的能读到低
   bit2 = CE (P1.3) 写完 1 真的能读到高
   bit3 = CE (P1.3) 写完 0 真的能读到低
   bit4 = SCK(P1.7) 写完 1 真的能读到高
   bit5 = SCK(P1.7) 写完 0 真的能读到低

   健康板子返回 63（0x3F）。少哪一位就是那根线的问题，比如：
     63 -> 三根线都好，那问题在 DS1302 芯片本体或它的供电/晶振/电池
     59（缺 bit2）-> P1.3 永远读回低，CE 根本抬不起来，DS1302 一片死寂
     62（缺 bit0）-> P2.3 被外部拉低，读回来全是 0x00（寄存器全 0 那种） */
uchar RTC_Probe(void)
{
	uchar r = 0;

	ET0 = 0;
	Seg_AllOff();

	DS1302_IO = 1;
	Delay_Us(20);
	if (DS1302_IO)  r |= 0x01;
	DS1302_IO = 0;
	Delay_Us(20);
	if (!DS1302_IO) r |= 0x02;
	DS1302_IO = 1;

	DS1302_CE = 1;
	Delay_Us(20);
	if (DS1302_CE)  r |= 0x04;
	DS1302_CE = 0;
	Delay_Us(20);
	if (!DS1302_CE) r |= 0x08;

	DS1302_SCK = 1;
	Delay_Us(20);
	if (DS1302_SCK)  r |= 0x10;
	DS1302_SCK = 0;
	Delay_Us(20);
	if (!DS1302_SCK) r |= 0x20;

	ET0 = 1;
	return r;
}

/* 诊断：把 6 个寄存器原值读出来（照常走一遍读时序），主程序打印后即可判断
   是芯片没响应(0xFF)、寄存器真是 0(0x00)、还是写入没生效 */
void RTC_DumpRaw(void)
{
	ET0 = 0;
	Seg_AllOff();
	RTC_Raw[0] = Rtc_ReadReg(0x81);
	RTC_Raw[1] = Rtc_ReadReg(0x83);
	RTC_Raw[2] = Rtc_ReadReg(0x85);
	RTC_Raw[3] = Rtc_ReadReg(0x87);
	RTC_Raw[4] = Rtc_ReadReg(0x89);
	RTC_Raw[5] = Rtc_ReadReg(0x8D);
	ET0 = 1;
}

/* 诊断：往 DS1302 的 RAM 字节 0（命令 0xC0/0xC1）写 0x5A 再读回，
   读回 90(0x5A) 说明三线通、芯片在；读回 255 说明线或芯片有问题。
   用 RAM 而不是时间寄存器，不会破坏走时。 */
uchar RTC_RamTest(void)
{
	uchar v;

	ET0 = 0;
	Seg_AllOff();
	Rtc_WriteReg(0xC0, 0x5A);
	v = Rtc_ReadReg(0xC0);
	ET0 = 1;
	return v;
}
