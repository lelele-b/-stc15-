#include "main.h"
#include "Hardware.h"

/* 数码管 4 页轮显，每 2 秒翻一页 */
#define PAGE_TIME	0
#define PAGE_TEMP	1
#define PAGE_LIGHT	2
#define PAGE_MAXV	3

static Time_T Now;			/* 当前时间（每秒刷新一次） */
static MaxRec Max;			/* 历史最大值记录（AT24C02） */

static uchar  Light  = 0;	/* 光照 0~255 */
static int    Temp10 = 0;	/* 温度 x10 */
static bit    TempOk = 0;

static uchar  Page = PAGE_TIME;
static uint   Tick100 = 0;
static uint   Tick500 = 0;
static uint   Tick1s  = 0;
static uint   Tick2s  = 0;

/* 把当前时刻记进最大值记录并落盘 */
static void Max_Save(void)
{
	Max.year  = Now.year;
	Max.month = Now.month;
	Max.day   = Now.day;
	Max.hour  = Now.hour;
	Max.min   = Now.min;
	Max.sec   = Now.sec;
	EEPROM_SaveMax(&Max);
}

static void Show_Page(void)
{
	switch (Page)
	{
		case PAGE_TIME:						/* HH-MM-SS */
			if (RTC_Ok)
			{
				Seg_Show2(0, Now.hour);
				Seg_Set(2, SG_DASH);
				Seg_Show2(3, Now.min);
				Seg_Set(5, SG_DASH);
				Seg_Show2(6, Now.sec);
			}
			else	/* DS1302 读不通：屏上报诊断码，不接串口也能看出卡在哪一段 */
			{
				Seg_Clr();
				Seg_Set(0, SG_E);
				Seg_Set(1, RTC_InitPath);				/* 1=CH=1 2=数据非法 3=没应答 */
				Seg_ShowUInt(7, 3, (uint)RTC_Raw[0]);	/* 秒寄存器原值：255=芯片没应答 */
			}
			break;

		case PAGE_TEMP:						/* t   25.6 */
			Seg_Clr();
			Seg_Set(0, SG_t);
			if (TempOk)
			{
				Seg_ShowTemp1(7, Temp10);
			}
			else
			{
				Seg_Set(5, SG_DASH);
				Seg_Set(6, SG_DASH);
				Seg_Set(7, SG_DASH);
			}
			break;

		case PAGE_LIGHT:					/* L    128 */
			Seg_Clr();
			Seg_Set(0, SG_L);
			Seg_Show3(5, (uint)Light);
			break;

		default:							/* H    200 */
			Seg_Clr();
			Seg_Set(0, SG_H);
			if (Max.light_valid)
			{
				Seg_ShowUInt(7, 3, (uint)Max.light_max);
			}
			else
			{
				Seg_Set(5, SG_DASH);
				Seg_Set(6, SG_DASH);
				Seg_Set(7, SG_DASH);
			}
			break;
	}
}

/* 26-10-20 21:30:05 T=25.6 L=128 H=200 */
static void Report(void)
{
	Uart_SendUInt2(Now.year);
	Uart_SendChar('-');
	Uart_SendUInt2(Now.month);
	Uart_SendChar('-');
	Uart_SendUInt2(Now.day);
	Uart_SendChar(' ');
	Uart_SendUInt2(Now.hour);
	Uart_SendChar(':');
	Uart_SendUInt2(Now.min);
	Uart_SendChar(':');
	Uart_SendUInt2(Now.sec);
	if (!RTC_Ok)
	{
		Uart_SendStr(" RTC=ERR");
	}

	Uart_SendStr(" T=");
	if (TempOk)
	{
		Uart_SendFixed1(Temp10);
	}
	else
	{
		Uart_SendStr("ERR");
	}
	Uart_SendStr(" L=");
	Uart_SendUInt((uint)Light);
	Uart_SendStr(" H=");
	Uart_SendUInt((uint)Max.light_max);
	Uart_SendStr("\r\n");
}

static void Report_MaxLog(void)
{
	Uart_SendStr("MAXLOG L=");
	Uart_SendUInt((uint)Max.light_max);
	Uart_SendStr(" @");
	Uart_SendUInt2(Max.year);
	Uart_SendChar('-');
	Uart_SendUInt2(Max.month);
	Uart_SendChar('-');
	Uart_SendUInt2(Max.day);
	Uart_SendChar(' ');
	Uart_SendUInt2(Max.hour);
	Uart_SendChar(':');
	Uart_SendUInt2(Max.min);
	Uart_SendChar(':');
	Uart_SendUInt2(Max.sec);
	Uart_SendStr("\r\n");
}

void main(void)
{
	uchar i;

	Seg_Clr();				/* 先把显示缓冲清成空白，避免上电瞬间全亮 */
	Delay_Init();			/* 定时器2 当微秒计数器，Delay_Us 依赖它（要先于 RTC/温度） */
	Timer0_Init();			/* 再起时基，Delay_Ms 依赖它 */
	EA = 0;					/* Timer0_Init 里已经开了 EA；Init_Periph 要连着写 4 个 573 锁存器，
							   中间要是被扫描中断插进来，P0/P2 就会被改成别人要的值，
							   上电那一瞬间数码管就会鬼影/乱亮。先关一下中断。 */
	Init_Periph();
	EA = 1;					/* 锁存器都设好了，再让扫描跑起来 */

	AUXR &= 0xFD;			/* 置 EXTRAM=0：xdata（串口发送缓冲）走片内扩展 RAM，MOVX 不会去拉外部总线 */

	IIC_Init();
	RTC_Init();

	Uart1_Init();			/* 定时器1 模式2：0xFD = 9600 @11.0592MHz */
	ADC_Init();

	RTC_GetTime(&Now);

	RTC_DumpRaw();						/* 诊断：DS1302 六个寄存器原值 */
	Uart_SendStr("RTCRAW");
	for (i = 0; i < 6; i++)
	{
		Uart_SendChar(' ');
		Uart_SendUInt((uint)RTC_Raw[i]);
	}
	Uart_SendStr(" path=");
	Uart_SendUInt((uint)RTC_InitPath);
	Uart_SendStr(" ram=");
	Uart_SendUInt((uint)RTC_RamTest());
	Uart_SendStr(" probe=");
	Uart_SendUInt((uint)RTC_Probe());
	Uart_SendStr("\r\n");

	Uart_SendStr("BOOT env-monitor t2=");
	if (Delay_T2Alive())
	{
		Uart_SendChar('1');
	}
	else
	{
		Uart_SendChar('0');
	}
	Uart_SendStr("\r\n");

	if (EEPROM_LoadMax(&Max))
	{
		Report_MaxLog();
	}
	else					/* 首次上电或记录校验不过，从零开始记 */
	{
		Max.light_max   = 0;
		Max.light_valid = 0;
		Max.temp_max10  = 0;
		Max.temp_valid  = 0;
		Max.year  = Now.year;
		Max.month = Now.month;
		Max.day   = Now.day;
		Max.hour  = Now.hour;
		Max.min   = Now.min;
		Max.sec   = Now.sec;
		Uart_SendStr("MAXLOG none\r\n");
	}

	Tick100 = Sys_TickMs;
	Tick500 = Sys_TickMs;
	Tick1s  = Sys_TickMs;
	Tick2s  = Sys_TickMs;

	while (1)
	{
		/* 每 500ms 采一次光照，创新高才落盘 */
		if ((uint)(Sys_TickMs - Tick500) >= 500)
		{
			Tick500 = Sys_TickMs;
			Light = ADC_ReadAvg(ADC_CH_LIGHT);
			if ((!Max.light_valid) || ((uint)Light > (uint)Max.light_max + 2))	/* 迟滞 2，噪声不会反复刷 EEPROM */
			{
				Max.light_max   = Light;
				Max.light_valid = 1;
				Max_Save();
			}
		}

		/* 每 1s 读一次 RTC 并向上位机上报一行 */
		if ((uint)(Sys_TickMs - Tick1s) >= 1000)
		{
			Tick1s = Sys_TickMs;
			RTC_GetTime(&Now);
			Report();
		}

		/* 每 2s 读一次温度（翻页改成手动按键，这里不再自动翻） */
		if ((uint)(Sys_TickMs - Tick2s) >= 2000)
		{
			Tick2s = Sys_TickMs;
			TempOk = TEMP_Read(&Temp10);
			if (TempOk && ((!Max.temp_valid) || (Temp10 > Max.temp_max10)))
			{
				Max.temp_max10 = Temp10;
				Max.temp_valid = 1;
				Max_Save();
			}
		}

		/* 键盘扫描已经在 Timer0 中断里做完了（每 10ms 一次），这里只取事件翻页：
		   S13/S12 下一屏，S17/S16 上一屏 */
		switch (Key_Poll())
		{
			case KEY_NEXT:
				Page++;
				if (Page > PAGE_MAXV)
				{
					Page = PAGE_TIME;
				}
				break;

			case KEY_PREV:
				if (Page == PAGE_TIME)
				{
					Page = PAGE_MAXV;
				}
				else
				{
					Page--;
				}
				break;

			default:
				break;
		}

		/* 100ms 重画一次显示缓冲。重画期间要关 ET0：否则中断正好扫到“改了一半”的
		   缓冲上，屏幕上就是半新半旧的画面——看起来就像页面在自己跳。 */
		if ((uint)(Sys_TickMs - Tick100) >= 100)
		{
			Tick100 = Sys_TickMs;
			ET0 = 0;
			Show_Page();
			ET0 = 1;
		}
	}
}
