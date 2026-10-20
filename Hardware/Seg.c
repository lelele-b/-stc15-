#include "main.h"
#include "Hardware.h"

/* 共阳数码管段码：bit7=dp bit6=g bit5=f bit4=e bit3=d bit2=c bit1=b bit0=a
   某位写 0 则该段点亮，所以 0xC0 是 '0'，0xFF 是全灭。 */
uchar code SegNum[] = {
	0xC0,	/* 0  '0' */
	0xF9,	/* 1  '1' */
	0xA4,	/* 2  '2' */
	0xB0,	/* 3  '3' */
	0x99,	/* 4  '4' */
	0x92,	/* 5  '5' */
	0x82,	/* 6  '6' */
	0xF8,	/* 7  '7' */
	0x80,	/* 8  '8' */
	0x90,	/* 9  '9' */
	0xBF,	/* 10 '-' */
	0xFF,	/* 11 ' ' */
	0xC7,	/* 12 'L' */
	0x89,	/* 13 'H' */
	0x87,	/* 14 't' */
	0x86	/* 15 'E' */
};

uchar Seg_Buf[SEG_DIGITS];

/* 位选在 Y6 锁存器，段选在 Y7 锁存器。
   刷新顺序：先把段选全灭（Y7=0xFF），再关所有位（Y6=0x00）、选中本位，
   最后写本位段码。这样本位点亮的那一刻段锁存器里是“全灭”，
   上一位的段码不会串到本位，彻底没有重影。 */
void Disp_Seg(uchar Location, uchar Value)
{
	SelectHC573(7);					/* Y7：段选，先全灭 */
	P0 = 0xFF;
	SelectHC573(6);					/* Y6：数码管位选 */
	P0 = 0x00;						/* 8 位全关 */
	P0 = (uchar)(0x01 << Location);	/* 选中本位 */
	SelectHC573(7);					/* Y7：写本位段码 */
	P0 = Value;
}

/* 进长临界区（DS1302 一次 6 字节读要 ~0.7ms、单总线一次 ~2ms）之前，
   先把 8 位位选全部关掉。
   这些临界区里 ET0 是停的，动态扫描跟着停摆：如果不关，最后点亮的那一位
   会一直亮到临界区结束，它比别的位亮得多，肉眼看到的就是“数码管突然闪一下”。
   关掉之后临界区里屏是暗的，人眼看不出来；ET0 恢复后第一拍 Disp_Seg
   会重新选位+写段码，画面自动接上。 */
void Seg_AllOff(void)
{
	SelectHC573(6);		/* Y6：位选锁存器 */
	P0 = 0x00;			/* 8 位全关 */
}

void Seg_Clr(void)
{
	uchar i;

	for (i = 0; i < SEG_DIGITS; i++)
	{
		Seg_Buf[i] = SegNum[SG_BLANK];
	}
}

void Seg_Set(uchar pos, uchar glyph)
{
	if (pos < SEG_DIGITS)
	{
		Seg_Buf[pos] = SegNum[glyph];
	}
}

void Seg_SetDp(uchar pos, uchar on)
{
	if (pos < SEG_DIGITS)
	{
		if (on)
		{
			Seg_Buf[pos] &= SEG_DP;
		}
		else
		{
			Seg_Buf[pos] |= 0x80;
		}
	}
}

void Seg_Show2(uchar pos, uchar value)
{
	if (pos + 1 < SEG_DIGITS)
	{
		Seg_Buf[pos]     = SegNum[value / 10];
		Seg_Buf[pos + 1] = SegNum[value % 10];
	}
}

void Seg_Show3(uchar pos, uint value)
{
	if (pos + 2 < SEG_DIGITS)
	{
		value %= 1000;
		Seg_Buf[pos]     = SegNum[value / 100];
		Seg_Buf[pos + 1] = SegNum[value / 10 % 10];
		Seg_Buf[pos + 2] = SegNum[value % 10];
	}
}

/* 右对齐显示：低位在 endPos，高位不够就补空格 */
void Seg_ShowUInt(uchar endPos, uchar len, uint value)
{
	uchar i;
	uchar pos;

	for (i = 0; i < len; i++)
	{
		if (endPos < i)
		{
			break;
		}
		pos = (uchar)(endPos - i);
		if (pos >= SEG_DIGITS)
		{
			continue;
		}
		if (value == 0 && i > 0)
		{
			Seg_Buf[pos] = SegNum[SG_BLANK];
		}
		else
		{
			Seg_Buf[pos] = SegNum[value % 10];
			value /= 10;
		}
	}
}

/* 温度显示：endPos 是最右一位（小数位），左起 [-][百][十][个.][小数]
   例：25.6 -> "  25.6"；-12.3 -> " -12.3"；125.0 -> " 125.0" */
void Seg_ShowTemp1(uchar endPos, int t10)
{
	uint mag;
	uchar base;
	uchar d[5];

	if (endPos < 4)
	{
		return;
	}
	base = (uchar)(endPos - 4);

	if (t10 < 0)
	{
		mag = (uint)(-t10);
		Seg_Buf[base] = SegNum[SG_DASH];
	}
	else
	{
		mag = (uint)t10;
		Seg_Buf[base] = SegNum[SG_BLANK];
	}

	d[4] = (uchar)(mag % 10);	mag /= 10;	/* 小数位 */
	d[3] = (uchar)(mag % 10);	mag /= 10;	/* 个位 */
	d[2] = (uchar)(mag % 10);	mag /= 10;	/* 十位 */
	d[1] = (uchar)(mag % 10);				/* 百位 */

	Seg_Buf[base + 4] = SegNum[d[4]];
	Seg_Buf[base + 3] = SegNum[d[3]];
	Seg_SetDp((uchar)(base + 3), 1);					/* 个位后面点小数点 */

	if (d[1] == 0 && d[2] == 0)
	{
		Seg_Buf[base + 2] = SegNum[SG_BLANK];
	}
	else
	{
		Seg_Buf[base + 2] = SegNum[d[2]];
	}

	if (d[1] == 0)
	{
		Seg_Buf[base + 1] = SegNum[SG_BLANK];
	}
	else
	{
		Seg_Buf[base + 1] = SegNum[d[1]];
	}
}
