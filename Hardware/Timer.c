#include "main.h"
#include "Hardware.h"

volatile uint Sys_TickMs = 0;

static uchar Seg_Sel = 0;	/* 当前正在显示的数码管位 */
static uchar SubMs   = 0;	/* 100us 累加到 1ms 的计数器 */

/* Timer0：每 100us 进一次，负责扫一位数码管、累加毫秒、顺带扫键盘 */
void Timer0_Isr(void) interrupt 1
{
	Disp_Seg(Seg_Sel, Seg_Buf[Seg_Sel]);

	if (++Seg_Sel >= SEG_DIGITS)
	{
		Seg_Sel = 0;
	}

	if (++SubMs >= 10)
	{
		SubMs = 0;
		Sys_TickMs++;
		Key_Tick();			/* 键盘扫描挂在这里：10ms 一次，与主循环忙闲无关 */
	}
}

void Timer0_Init(void)
{
	AUXR &= 0x7F;		/* Timer0 工作在 12T */
	TMOD &= 0xF0;		/* STC15 的 Timer0 模式 0 = 16 位自动重载 */
	TL0 = 0xA4;			/* 65536-92：100us @11.0592MHz */
	TH0 = 0xFF;
	TF0 = 0;
	TR0 = 1;
	ET0 = 1;
	EA = 1;
}
