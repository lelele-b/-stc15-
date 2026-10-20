#ifndef __EEPROM_H__
#define __EEPROM_H__

#include "main.h"

#define EEPROM_ADDR		0xA0	/* AT24C02 写地址，读地址 0xA1 */

/* 历史最大值记录（对应 AT24C02 的 0x00~0x0F 共 16 字节） */
typedef struct
{
	uchar light_max;		/* 光照历史最大值 0~255 */
	uchar light_valid;		/* 1 = light_max 有效 */
	int   temp_max10;		/* 温度历史最大值，单位 0.1℃，有符号 */
	uchar temp_valid;
	uchar year;				/* 创新高时的时刻 0~99 */
	uchar month;
	uchar day;
	uchar hour;
	uchar min;
	uchar sec;
} MaxRec;

void EEPROM_SaveMax(MaxRec *rec);			/* 写两页，约 20ms */
bit  EEPROM_LoadMax(MaxRec *rec);			/* 1 = 校验通过、记录有效 */

#endif
