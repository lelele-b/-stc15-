#include "main.h"
#include "Delay.h"
#include "Timer.h"

static bit Delay_T2Ok = 0;		/* 1 = 定时器2 确实在计数，Delay_Us 才敢用它 */

/* 毫秒延时：直接数 Timer0 的软计时，不依赖循环指令周期，
   所以换时钟/换优化级别都不会失准。
   前提：已调用 Timer0_Init()，且调用期间 ET0/EA 没有被关掉。 */
void Delay_Ms(uint xms)
{
	uint start;

	start = Sys_TickMs;
	while ((uint)(Sys_TickMs - start) < xms)
	{
		;
	}
}

/* 读定时器2 的计数值。
   注意：这里**不能**写成"连读两次、一致才认"的 do-while——
   1T 自由计数器每 90ns 就变一次，两次读几乎永远不相等，
   一旦进了那个循环就再也出不来（上一版数码管全灭就是这个死循环卡住了）。
   读 T2L 时硬件会把 T2H 锁存下来，所以先读低字节再读高字节就是一致的。 */
static uint Delay_T2Count(void)
{
	uint lo;
	uint hi;

	lo = (uint)T2L;
	hi = (uint)T2H;

	return (hi << 8) | lo;
}

bit Delay_T2Alive(void)
{
	return Delay_T2Ok;
}

void Delay_Init(void)
{
	uint a;
	uint b;
	uchar i;

	AUXR |= 0x04;		/* T2x12 = 1：定时器2 按 1T（不分频）计数 */
	T2H = 0x00;
	T2L = 0x00;			/* 重载值 0：0x0000~0xFFFF 循环，5.9ms 绕一圈 */
	AUXR |= 0x10;		/* T2R = 1：启动计数 */

	/* 自检：跑几十条指令后再看，计数应该变了 */
	a = Delay_T2Count();
	for (i = 0; i < 50; i++)
	{
		;
	}
	b = Delay_T2Count();
	Delay_T2Ok = (bit)(a != b);
}

/* 微秒延时：优先用定时器2 的硬件计数（和 CPU 同源，优化级别/晶振都不影响）；
   万一定时器2 没跑起来，退回空循环，慢很多但绝不卡死。 */
void Delay_Us(uint us)
{
	uint start;
	uint ticks;
	uint guard;
	uint n;

	if (us == 0)
	{
		return;
	}

	if (Delay_T2Ok)
	{
		ticks = us * 11;					/* 11.0592MHz：1us 约 11 个计数 */
		start = Delay_T2Count();
		guard = ticks * 20 + 1000;			/* 兜底：最多等 20 倍时间，绝不死等 */
		while (((uint)(Delay_T2Count() - start) < ticks) && (--guard != 0))
		{
			;
		}
		return;
	}

	n = us * 3;
	while (n--)
	{
		;
	}
}
