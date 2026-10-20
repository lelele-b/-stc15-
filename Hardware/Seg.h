#ifndef __SEG_H__
#define __SEG_H__

#include "main.h"

#define SEG_DIGITS	8		/* 8 位数码管：[0] 最左，[7] 最右 */

/* 段码表下标（见 Seg.c 的 SegNum[]） */
#define SG_0		0
#define SG_1		1
#define SG_2		2
#define SG_3		3
#define SG_4		4
#define SG_5		5
#define SG_6		6
#define SG_7		7
#define SG_8		8
#define SG_9		9
#define SG_DASH		10		/* '-' */
#define SG_BLANK	11		/* 空格 */
#define SG_L		12		/* 'L' */
#define SG_H		13		/* 'H' */
#define SG_t		14		/* 't' */
#define SG_E		15		/* 'E' */

#define SEG_DP		0x7F	/* 共阳：与段码相与即为点亮小数点 */

extern uchar Seg_Buf[SEG_DIGITS];

void Disp_Seg(uchar Location, uchar Value);			/* 定时器中断里逐位刷新 */
void Seg_AllOff(void);								/* 关掉全部位选：进长临界区前先关屏 */
void Seg_Clr(void);									/* 全部清成空格 */
void Seg_Set(uchar pos, uchar glyph);				/* 写一个字符 */
void Seg_SetDp(uchar pos, uchar on);				/* 控制该位小数点 */
void Seg_Show2(uchar pos, uchar value);				/* 两位十进制，10~99 */
void Seg_Show3(uchar pos, uint value);				/* 三位十进制，0~999 */
void Seg_ShowUInt(uchar endPos, uchar len, uint value);	/* 右对齐，剩余补空格 */
void Seg_ShowTemp1(uchar endPos, int t10);			/* 温度，1 位小数，带符号 */

#endif
