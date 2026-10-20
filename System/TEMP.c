#include "main.h"
#include "Hardware.h"

/* SCH_V31 的 DS18B20 单总线在 P1.4，板上 10k 上拉 */
sbit DQ = P1^4;

/* 复位脉冲 + 存在脉冲检测。
   主机拉低 >=480us，释放后 15~60us 内器件拉低 60~240us。
   Delay_Us 现在由定时器2 计数产生，是准的，直接按手册值给：
   拉低 600us，释放后持续轮询 300us（存在脉冲一定落在里面）。 */
static bit Temp_Reset(void)
{
	bit ok = 0;
	uchar t;

	DQ = 1;
	Delay_Us(10);
	DQ = 0;
	Delay_Us(600);
	DQ = 1;
	for (t = 0; t < 60; t++)
	{
		if (!DQ)
		{
			ok = 1;
			break;
		}
		Delay_Us(5);
	}
	Delay_Us(300);			/* 等存在脉冲结束，再进入命令阶段 */
	return ok;
}

/* 写一个字节：写 1 是低 2us 的短脉冲，写 0 是低 60us 的长脉冲。
   DS18B20 在每个时隙的第 15~60us 采样，所以两种都能被正确识别。 */
static void Temp_WriteByte(uchar dat)
{
	uchar i;

	for (i = 0; i < 8; i++)
	{
		DQ = 0;
		Delay_Us(2);
		if (dat & 0x01)
		{
			DQ = 1;
		}
		Delay_Us(60);
		DQ = 1;
		Delay_Us(2);			/* 时隙之间至少 1us 恢复 */
		dat >>= 1;
	}
}

/* 读一个字节：拉低 2us 启动时隙，释放后 8us 采样
   （器件在下降沿后 15us 内送出数据，8us 正好在窗口里）。 */
static uchar Temp_ReadByte(void)
{
	uchar i;
	uchar dat = 0;

	for (i = 0; i < 8; i++)
	{
		DQ = 0;
		Delay_Us(2);
		DQ = 1;
		Delay_Us(6);
		dat >>= 1;
		if (DQ)
		{
			dat |= 0x80;
		}
		Delay_Us(52);			/* 补满 60us 时隙 */
	}
	return dat;
}

bit TEMP_Read(int *t10)
{
	uchar tl;
	uchar th;
	uint raw;
	int sraw;
	bit ok = 0;

	ET0 = 0;				/* 单总线靠位间隙，期间必须停掉 108us 扫描中断 */
	ES  = 0;
	Seg_AllOff();	/* 扫描停摆这 ~2ms 先关屏，否则会有一位亮到临界区结束（像闪一下） */
	if (Temp_Reset())
	{
		Temp_WriteByte(0xCC);		/* 跳过 ROM */
		Temp_WriteByte(0x44);		/* 启动一次温度转换 */
		ET0 = 1;					/* 转换要 750ms，这段时间恢复中断 */
		ES  = 1;
		Delay_Ms(750);
		ET0 = 0;
		ES  = 0;
		Seg_AllOff();	/* 扫描停摆这 ~2ms 先关屏，否则会有一位亮到临界区结束（像闪一下） */
		if (Temp_Reset())
		{
			Temp_WriteByte(0xCC);
			Temp_WriteByte(0xBE);	/* 读暂存器 */
			tl = Temp_ReadByte();
			th = Temp_ReadByte();
			raw = ((uint)th << 8) | (uint)tl;
			if (raw != 0x0550)		/* 85.0℃ 是上电默认值，说明没真正转换 */
			{
				sraw = (int)raw;
				if (sraw >= 0)
				{
					*t10 = (int)((sraw * 5 + 4) / 8);	/* 0.0625℃/LSB -> x10 */
				}
				else
				{
					*t10 = (int)((sraw * 5 - 4) / 8);
				}
				ok = 1;
			}
		}
	}
	ET0 = 1;
	ES  = 1;
	return ok;
}
