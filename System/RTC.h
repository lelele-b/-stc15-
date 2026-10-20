#ifndef __RTC_H__
#define __RTC_H__

#include "main.h"

typedef struct
{
	uchar year;		/* 0~99，例如 26 表示 2026 年 */
	uchar month;
	uchar day;
	uchar hour;
	uchar min;
	uchar sec;
} Time_T;

void RTC_Init(void);			/* 停振或数据非法时写入默认时间 */
void RTC_GetTime(Time_T *t);
void RTC_SetTime(Time_T *t);

/* 诊断用：直接读出 6 个寄存器的原值，不做 BCD 转换 */
extern uchar RTC_Raw[6];
extern uchar RTC_InitPath;
extern uchar RTC_Ok;	/* 1=最近一次读到了合法时间 0=没应答/数据非法 */
/* 诊断：DS1302 三根线的引脚自检，返回 63 表示三根线都正常 */
uchar RTC_Probe(void);
void RTC_DumpRaw(void);
uchar RTC_RamTest(void);

#endif
