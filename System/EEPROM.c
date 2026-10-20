#include "main.h"
#include "Hardware.h"

/* 记录布局（16 字节，正好两页）：
   0x00 0x5A      魔数 0
   0x01 0x01      版本
   0x02 0xA5      魔数 1
   0x03 light_max
   0x04 light_valid
   0x05 temp_lo
   0x06 temp_hi
   0x07 temp_valid
   0x08 year  0x09 month 0x0A day
   0x0B hour  0x0C min   0x0D sec
   0x0E 前 14 字节累加和
   0x0F 0x00（保留） */
#define REC_ADDR	0x00

static void EEPROM_WritePage8(uchar base, uchar *buf)
{
	uchar i;

	IIC_Start();
	IIC_SendByte(EEPROM_ADDR);
	IIC_WaitAck();
	IIC_SendByte(base);			/* 页内地址必须是 8 的整数倍 */
	IIC_WaitAck();
	for (i = 0; i < 8; i++)
	{
		IIC_SendByte(buf[i]);
		IIC_WaitAck();
	}
	IIC_Stop();
	Delay_Ms(10);				/* 等内部写周期结束（手册 5ms） */
}

void EEPROM_SaveMax(MaxRec *rec)
{
	uchar b[8];
	uchar sum = 0;
	uchar i;
	uint  t;

	/* 第一页：0x00~0x07 */
	b[0] = 0x5A;
	b[1] = 0x01;
	b[2] = 0xA5;
	b[3] = rec->light_max;
	b[4] = rec->light_valid;
	t = (uint)rec->temp_max10;
	b[5] = (uchar)(t & 0xFF);
	b[6] = (uchar)(t >> 8);
	b[7] = rec->temp_valid;

	for (i = 0; i < 8; i++)
	{
		sum += b[i];
	}
	EEPROM_WritePage8(REC_ADDR, b);

	/* 第二页：0x08~0x0F */
	b[0] = rec->year;
	b[1] = rec->month;
	b[2] = rec->day;
	b[3] = rec->hour;
	b[4] = rec->min;
	b[5] = rec->sec;
	for (i = 0; i < 6; i++)
	{
		sum += b[i];
	}
	b[6] = sum;			/* 0x0E：前 14 字节累加和 */
	b[7] = 0x00;		/* 0x0F：保留 */
	EEPROM_WritePage8(REC_ADDR + 8, b);
}

/* 一次连续读出 16 字节：地址由 AT24C02 自己加 1，
   前 15 个字节回 ACK，最后一个回 NACK 再 STOP。 */
bit EEPROM_LoadMax(MaxRec *rec)
{
	uchar b[16];
	uchar sum = 0;
	uchar i;

	IIC_Start();
	IIC_SendByte(EEPROM_ADDR);
	IIC_WaitAck();
	IIC_SendByte(REC_ADDR);
	IIC_WaitAck();
	IIC_Start();
	IIC_SendByte(EEPROM_ADDR | 0x01);
	IIC_WaitAck();
	for (i = 0; i < 15; i++)
	{
		b[i] = IIC_Recive();
		IIC_SendAck();
	}
	b[15] = IIC_Recive();
	IIC_SendNack();
	IIC_Stop();

	if ((b[0] != 0x5A) || (b[1] != 0x01) || (b[2] != 0xA5))
	{
		return 0;			/* 新片或记录被擦过 */
	}
	for (i = 0; i < 14; i++)
	{
		sum += b[i];
	}
	if (sum != b[14])
	{
		return 0;			/* 校验不过，按无记录处理 */
	}

	rec->light_max   = b[3];
	rec->light_valid = b[4];
	rec->temp_max10  = (int)(((uint)b[6] << 8) | (uint)b[5]);
	rec->temp_valid  = b[7];
	rec->year        = b[8];
	rec->month       = b[9];
	rec->day         = b[10];
	rec->hour        = b[11];
	rec->min         = b[12];
	rec->sec         = b[13];
	return 1;
}
