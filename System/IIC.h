#ifndef __IIC_H__
#define __IIC_H__

#include "main.h"

/*
 * Software I2C driver (bit banging) for CT107D V3.1
 *   I2C_SCL -> P2.0
 *   I2C_SDA -> P2.1
 *
 * NOTE about the display refresh:
 *   Timer0 interrupts every 100us and writes P2 (74HC138 select) with
 *   "P2 = (P2 & 0x1f) | 0xC0".  P2 is a quasi-bidirectional port, so the
 *   value written back is the PIN level.  If the slave is holding SDA low
 *   at that moment, the port latch can be forced to 0 and the master keeps
 *   SDA low.  This driver re-writes "SDA = 1" before every bit that must be
 *   read, which makes it self healing, but if you ever see rare corrupted
 *   bytes during EEPROM / PCF8591 access, wrap the whole transaction:
 *
 *       EA = 0;
 *       ... I2C transaction ...
 *       EA = 1;
 *
 *   A transaction is below 1ms, the display simply freezes for that time.
 */

void  IIC_Delay(void);          /* bit timing helper, about 10us */

void  IIC_Init(void);           /* release the bus, 9-clock recovery */
void  IIC_Start(void);          /* START condition            */
void  IIC_Stop(void);           /* STOP  condition            */

void  IIC_SendByte(uchar Byte); /* 8 bits, MSB first, leaves SCL = 0 */
uchar IIC_Recive(void);         /* 8 bits, MSB first, leaves SCL = 0 */

bit   IIC_WaitAck(void);        /* 9th clock: 0 = ACK (ok), 1 = no ACK */
void  IIC_SendAck(void);        /* master ACK  : keep reading  */
void  IIC_SendNack(void);       /* master NACK : last byte     */

#endif
