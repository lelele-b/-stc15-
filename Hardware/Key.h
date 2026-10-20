#ifndef __KEY_H__
#define __KEY_H__

/* 键盘事件码 */
#define KEY_NONE	0
#define KEY_NEXT	1		/* S13 / S12：下一屏 */
#define KEY_PREV	2		/* S17 / S16：上一屏 */

/* 在 Timer0 中断里每 1ms 调一次（内部 10 分频，实际 10ms 扫一次） */
void Key_Tick(void);

/* 主循环里调，取走一次按键事件；没键返回 KEY_NONE */
uchar Key_Poll(void);

#endif
