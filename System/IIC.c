#include "main.h"
#include "IIC.h"

/*
 *  Software I2C for CT107D V3.1 (IAP15F2K61S2)
 *
 *  Four rules this file follows:
 *    1. SDA only changes while SCL == 0   (START / STOP are the exceptions)
 *    2. the slave samples SDA while SCL == 1
 *    3. START : SDA falls while SCL == 1
 *       STOP  : SDA rises while SCL == 1
 *    4. every function enters with SCL == 0 and leaves with SCL == 0
 *       (IIC_Start enters idle = SCL 1, IIC_Stop leaves idle = SCL 1)
 *
 *  Function chain:
 *    IIC_Init  -> release bus / recover a slave that holds SDA low
 *    IIC_Start -> SDA=0 while SCL=1
 *    IIC_SendByte(0xA0) -> IIC_WaitAck() -> ...
 *    IIC_Recive() -> IIC_SendNack() / IIC_SendAck()
 *    IIC_Stop  -> SDA=1 while SCL=1
 *
 *  The board has no external pull-up on SCL/SDA, the quasi-bidirectional
 *  port does the job but the rising edge is slow, so keep the delay >= 10us.
 */

sbit SCL = P2 ^ 0;              /* I2C_SCL */
sbit SDA = P2 ^ 1;              /* I2C_SDA */

void IIC_Delay(void)            /* about 10us at 11.0592MHz (1T mode) */
{
	uchar t = 20;               /* make it 5 to speed up when it works */
	while(t--);
}

void IIC_Init(void)
{
	uchar i;
	SDA = 1;                    /* release both lines */
	SCL = 1;
	IIC_Delay();
	for(i = 0; i < 9; i++)      /* give a stuck slave 9 clocks to let go */
	{
		if(SDA) break;          /* bus is free already */
		SCL = 0;
		IIC_Delay();
		SCL = 1;
		IIC_Delay();
	}
	SCL = 0;
	IIC_Delay();
}

void IIC_Start(void)
{
	SDA = 1;
	SCL = 1;                    /* make sure the bus is idle     */
	IIC_Delay();
	SDA = 0;                    /* SDA falls while SCL is high   */
	IIC_Delay();
	SCL = 0;                    /* clock back low, ready to send */
	IIC_Delay();
}

void IIC_Stop(void)
{
	SCL = 0;
	SDA = 0;
	IIC_Delay();
	SCL = 1;                    /* clock high ...                */
	IIC_Delay();
	SDA = 1;                    /* ... SDA rises = STOP          */
	IIC_Delay();
}

void IIC_SendByte(uchar Byte)
{
	uchar i;
	SCL = 0;                    /* data may only change now      */
	for(i = 0; i < 8; i++)
	{
		if(Byte & 0x80)         /* 1. place the bit (SCL low)    */
		{
			SDA = 1;
		}
		else
		{
			SDA = 0;
		}
		Byte <<= 1;
		IIC_Delay();
		SCL = 1;                /* 2. clock high, slave samples  */
		IIC_Delay();
		SCL = 0;                /* 3. clock low, next bit        */
		IIC_Delay();
	}
}

bit IIC_WaitAck(void)           /* 0 = slave ACKed, 1 = no answer */
{
	bit ack;
	SDA = 1;                    /* release SDA, slave pulls it low to ACK */
	IIC_Delay();
	SCL = 1;                    /* 9th clock                     */
	IIC_Delay();
	ack = SDA;
	SCL = 0;
	IIC_Delay();
	return ack;
}

uchar IIC_Recive(void)
{
	uchar i;
	uchar dat = 0;              /* must be initialised           */
	for(i = 0; i < 8; i++)
	{
		SDA = 1;                /* release: slave drives the bit */
		IIC_Delay();
		SCL = 1;                /* read it while SCL is high     */
		IIC_Delay();
		dat <<= 1;              /* shift first, then fill        */
		if(SDA)
		{
			dat |= 0x01;
		}
		SCL = 0;
		IIC_Delay();
	}
	return dat;
}

void IIC_SendAck(void)          /* "send me another byte"         */
{
	SCL = 0;
	SDA = 0;                    /* pull SDA low during 9th clock  */
	IIC_Delay();
	SCL = 1;
	IIC_Delay();
	SCL = 0;
	IIC_Delay();
	SDA = 1;                    /* release again                  */
	IIC_Delay();
}

void IIC_SendNack(void)         /* "that was the last one"        */
{
	SCL = 0;
	SDA = 1;                    /* leave SDA high during 9th clock */
	IIC_Delay();
	SCL = 1;
	IIC_Delay();
	SCL = 0;
	IIC_Delay();
}
