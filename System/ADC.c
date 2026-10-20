#include "main.h"
#include "Hardware.h"

void ADC_Init(void)
{
	ADC_Read(ADC_CH_LIGHT);		/* 先把量路都采一遍，让内部通道定下来 */
	ADC_Read(ADC_CH_POT);
}

/* PCF8591 读一次：先写控制字选通道，新的一次转换是在控制字 ACK 之后才开始的，
   所以紧跟着读回来的第一个字节还是上一次的结果。这里连着做两次事务，
   第二次读到的才是本通道的新值。 */
uchar ADC_Read(uchar ch)
{
	uchar ctrl;
	uchar dat;

	ctrl = (uchar)(ch & 0x03);		/* bit1:0 = 通道号，bit6=0 关掉 DAC 输出 */

	IIC_Start();
	IIC_SendByte(0x90);
	IIC_WaitAck();
	IIC_SendByte(ctrl);
	IIC_WaitAck();
	IIC_Start();
	IIC_SendByte(0x91);
	IIC_WaitAck();
	IIC_Recive();					/* 旧值，丢掉 */
	IIC_SendNack();
	IIC_Stop();

	IIC_Start();
	IIC_SendByte(0x90);
	IIC_WaitAck();
	IIC_SendByte(ctrl);
	IIC_WaitAck();
	IIC_Start();
	IIC_SendByte(0x91);
	IIC_WaitAck();
	dat = IIC_Recive();
	IIC_SendNack();
	IIC_Stop();

	return dat;
}

/* 连采 4 次取平均：电位器/光敏的滑片是高压节点，单次读数会跳 2~3 个码，
   直接拿去更新最大值会不停触发 EEPROM 写。取平均后显示也稳。 */
uchar ADC_ReadAvg(uchar ch)
{
	uint sum = 0;
	uchar i;

	for (i = 0; i < 4; i++)
	{
		sum += ADC_Read(ch);
	}

	return (uchar)((sum + 2) / 4);
}