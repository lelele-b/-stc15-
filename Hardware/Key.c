#include "main.h"
#include "Hardware.h"
#include <INTRINS.H>

/* 这块板的 4x4 键盘：行 ROW1~ROW4 = P3.0/P3.1/P3.2/P3.3，
   列 COL1~COL4 = P4.4/P4.2/P3.5/P3.4。

   本工程只用 COL3、COL4 两列的 4 个键（S13/S12 与 S17/S16），原因：
   1) P3.0/P3.1 是串口1（接了 CH340C），做行线会和串口打架，绝对不碰；
   2) P4.2 是 74HC573 的 WR，拉高会让显示锁存器关闭，不能拿来做列线；
   3) COL1(P4.4) 会随 J5 跳线在“矩阵/独立按键”之间切换，J5 短接 2-3 时
      它被直接接地，做列线会一直判定为按下。
   剩下 P3.5、P3.4 两根列线干净，各带两个键，够切屏用。 */

sbit ROW3 = P3 ^ 2;			/* 第 3 行：S13 / S17 */
sbit ROW4 = P3 ^ 3;			/* 第 4 行：S12 / S16 */
sbit COL3 = P3 ^ 5;			/* 第 3 列 */
sbit COL4 = P3 ^ 4;			/* 第 4 列 */

#define KEY_DIV		10		/* Timer0 中断 1ms 一次，10 分频 = 每 10ms 扫一次 */
#define KEY_STABLE	3		/* 连续 3 拍一致才认：30ms 消抖 */
#define KEY_COOL	120		/* 两次按键事件至少间隔 120ms，掐掉连跳 */

static volatile uchar Key_Event = KEY_NONE;	/* 待主循环取走的事件 */

/* 关键点：准双向口被拉低之后，恢复高电平靠的是内部弱上拉（只有几十 uA），
   板上 16 个按键到锁存器那段走线有上百 pF，恢复要十几 us。释放列线后
   立刻去读，读到的还是旧的 0 —— 表现就是“手一碰板子页面就乱跳”。
   所以每次动完行/列都要等一小会儿再采。 */
static void Key_Delay(void)
{
	uchar t = 6;			/* ~20us @11.0592MHz，12T（一个 while 迭代约 3 个机器周期） */

	while (t--)
	{
		_nop_();
	}
}

/* 扫一遍。注意：S13/S12 都是“下一屏”，S17/S16 都是“上一屏”，
   两行的动作完全相同，所以不需要逐行扫——两根行线一起拉低就行，
   这样只需要一次等待，中断里花的时间短，也不会踩到“换行后列线还没恢复”的坑。 */
static uchar Key_RawScan(void)
{
	uchar c3, c4;
	uchar act = KEY_NONE;

	ROW3 = 1;
	ROW4 = 1;
	COL3 = 1;
	COL4 = 1;
	Key_Delay();			/* 等两根列线被弱上拉拉回高电平 */

	if ((!COL3) || (!COL4))	/* 空闲态就不干净，这次采样直接丢掉 */
	{
		return KEY_NONE;
	}

	ROW3 = 0;				/* 两根行线一起拉低 */
	ROW4 = 0;
	Key_Delay();			/* 等按键把列线拉低 */

	c3 = COL3;				/* 读进变量再恢复行线，避免边读边改 */
	c4 = COL4;
	ROW3 = 1;
	ROW4 = 1;

	if (!c3)
	{
		act = KEY_NEXT;		/* S13 / S12 */
	}
	else if (!c4)
	{
		act = KEY_PREV;		/* S17 / S16 */
	}

	return act;
}

/* 带消抖 + 边沿检测：每 10ms 调一次，连续 KEY_STABLE 拍一致才算稳定，
   只在“无键 -> 有键”的那一次变化上产生事件，按住不放只算一次。 */
static uchar Key_Scan(void)
{
	static uchar raw_old = KEY_NONE;
	static uchar stable  = KEY_NONE;
	static uchar cnt     = 0;
	static uint  last_ms = 0;
	uchar now;
	uchar event = KEY_NONE;

	now = Key_RawScan();

	if (now != raw_old)		/* 还在抖，重新计数 */
	{
		raw_old = now;
		cnt = 0;
		return KEY_NONE;
	}

	if (cnt < KEY_STABLE)
	{
		cnt++;
		return KEY_NONE;
	}

	if (now == stable)		/* 状态没变，不重复发事件 */
	{
		return KEY_NONE;
	}

	stable = now;
	if (now != KEY_NONE)	/* 0 -> 有键：一次新按下 */
	{
		if ((uint)(Sys_TickMs - last_ms) >= KEY_COOL)
		{
			last_ms = Sys_TickMs;
			event = now;
		}
	}

	return event;
}

/* 挂在 Timer0 中断里（每 1ms 调一次），10 分频后扫键盘。
   放在定时器里扫而不是主循环里扫，是因为主循环有 750ms 的测温阻塞窗口，
   那段时间按键会“失聪”；中断里扫就与主循环忙不忙无关了。 */
void Key_Tick(void)
{
	static uchar div = 0;
	uchar ev;

	if (++div < KEY_DIV)
	{
		return;
	}
	div = 0;

	ev = Key_Scan();
	if (ev != KEY_NONE)
	{
		Key_Event = ev;		/* 只记最新的一个，主循环取走即清 */
	}
}

/* 主循环取事件：有键返回事件码，没键返回 KEY_NONE */
uchar Key_Poll(void)
{
	uchar ev;

	ev = Key_Event;
	Key_Event = KEY_NONE;

	return ev;
}
