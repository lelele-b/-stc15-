#include "main.h"
#include "Hardware.h"

/* 板上 P4.2 网络名 C2WR，接 74HC02 的 WR 端（同时是键盘 COL2）。
   74HC02：YnC = NOR(Yn, WR)，只有 Yn=0 且 WR=0 时锁存器才透明，
   所以 J13 必须短接 2-3（WR 接地）；这里再把它拉低做双保险。 */
sbit WR_PIN = P4^2;

/* 74HC138 的 A0/A1/A2 接 P2.5/P2.6/P2.7。
   ★ 只能一位一位地写，不能写成 P2 = (P2 & 0x1F) | 地址 这种整口读-改写。
   P2 是准双向口，读 P2 读到的是“引脚电平”，不是上次写进去的锁存值：
   P2.0/P2.1 是 I2C 的 SDA/SCL，P2.3 是 DS1302 的 IO，这三根线别人也会拉低。
   外设正把某根线拉低时，整口读回来那一位就是 0，再原样写回去等于往准双向口
   写 0 —— 那是强下拉，会把整根线钉死到下一次有人写 1 为止。
   而这条语句在 Timer0 中断里每 100us 就要跑一次，一旦撞上 I2C/DS1302 的
   传输，通讯就随机失败（表现就是时间读不出来、EEPROM 偶发读写错）。
   改成对 P2.5/P2.6/P2.7 逐位赋值，就完全不碰那三根线，从根上断掉互相踩踏。 */
sbit SEL_A0 = P2^5;
sbit SEL_A1 = P2^6;
sbit SEL_A2 = P2^7;

void SelectHC573(uchar n)
{
	if (n < 4 || n > 7)		/* 只有 Y4~Y7 接了锁存器 */
	{
		return;
	}
	SEL_A0 = (bit)(n & 0x01);
	SEL_A1 = (bit)((n >> 1) & 0x01);
	SEL_A2 = (bit)((n >> 2) & 0x01);
}

void Init_Periph(void)
{
	WR_PIN = 0;

	SelectHC573(4);		/* LED：1 灭 */
	P0 = 0xFF;

	SelectHC573(5);		/* 蜂鸣器/继电器/步进电机：0 关 */
	P0 = 0x00;

	SelectHC573(7);		/* 段码全灭 */
	P0 = 0xFF;

	SelectHC573(6);		/* 位选全关：放最后，免得前面写的段码漏亮一位 */
	P0 = 0x00;
}
