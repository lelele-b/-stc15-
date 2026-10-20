#ifndef __TIMER_H__
#define __TIMER_H__

#include "main.h"

extern volatile uint Sys_TickMs;	/* 系统毫秒计数，Timer0 中断累加 */

void Timer0_Init(void);				/* Timer0 100us 中断：数码管动态扫描 + 系统计时 */

#endif
