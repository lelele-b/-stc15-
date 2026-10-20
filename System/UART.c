#include "main.h"
#include "UART.h"

#define TX_SIZE		64

static uchar xdata TxBuf[TX_SIZE];	/* 放内部扩展 RAM：STC15F2K60S2 有 1792 字节，不占宝贵的 idata */
static uchar TxHead = 0;		/* 下一个要发的 */
static uchar TxTail = 0;		/* 下一个要放的 */
static bit   TxBusy = 0;		/* 1 = 正在发送 */

/* 把队首字节塞进 SBUF；调用前要保证 ES=0 */
static void Uart_Start(void)
{
	uchar next;

	if (TxBusy || (TxHead == TxTail))
	{
		return;
	}
	TxBusy = 1;
	next = TxBuf[TxHead];
	if (++TxHead >= TX_SIZE)
	{
		TxHead = 0;
	}
	SBUF = next;
}

void Uart1_Isr(void) interrupt 4
{
	uchar next;

	if (TI)
	{
		TI = 0;
		if (TxHead != TxTail)
		{
			next = TxBuf[TxHead];
			if (++TxHead >= TX_SIZE)
			{
				TxHead = 0;
			}
			SBUF = next;
		}
		else
		{
			TxBusy = 0;
		}
	}

	if (RI)
	{
		RI = 0;			/* 暂时不解析上位机命令，收到就丢掉 */
	}
}

/* 串口1：定时器1 模式2（8 位自动重载）做波特率发生器，12T。
   11.0592MHz、SMOD=0 时 0xFD 正好是 9600：
       256 - 0xFD = 3，11.0592MHz/12/32/3 = 9600 */
void Uart1_Init(void)
{
	PCON &= 0x7F;			/* SMOD = 0，波特率不加倍 */
	SCON = 0x50;			/* 8 位数据，可变波特率 */
	AUXR &= 0xBF;			/* 定时器1 工作在 12T */
	AUXR &= 0xFE;			/* 串口1 选定时器1 做波特率发生器 */
	TMOD &= 0x0F;			/* 只改定时器1，不动定时器0 */
	TMOD |= 0x20;			/* 定时器1 模式2：8 位自动重载 */
	TL1 = 0xFD;				/* 9600bps @11.0592MHz */
	TH1 = 0xFD;
	ET1 = 0;
	TR1 = 1;
	ES = 1;
	EA = 1;
}


void Uart_SendChar(uchar dat)
{
	uchar next;

	ES = 0;
	next = TxTail + 1;
	if (next >= TX_SIZE)
	{
		next = 0;
	}
	if (next == TxHead)			/* 队列满：丢掉最旧的一个字节，绝不阻塞 */
	{
		if (++TxHead >= TX_SIZE)
		{
			TxHead = 0;
		}
	}
	TxBuf[TxTail] = dat;
	TxTail = next;
	Uart_Start();
	ES = 1;
}

void Uart_SendStr(char *str)
{
	while (*str)
	{
		Uart_SendChar((uchar)(*str));
		str++;
	}
}

void Uart_SendUInt(uint value)
{
	uchar buf[5];
	uchar i = 0;

	if (value == 0)
	{
		Uart_SendChar('0');
		return;
	}
	while (value && i < 5)
	{
		buf[i] = (uchar)('0' + value % 10);
		i++;
		value /= 10;
	}
	while (i)
	{
		i--;
		Uart_SendChar(buf[i]);
	}
}

void Uart_SendUInt2(uchar value)
{
	Uart_SendChar((uchar)('0' + value / 10));
	Uart_SendChar((uchar)('0' + value % 10));
}

void Uart_SendFixed1(int v10)
{
	if (v10 < 0)
	{
		Uart_SendChar('-');
		v10 = -v10;
	}
	Uart_SendUInt((uint)(v10 / 10));
	Uart_SendChar('.');
	Uart_SendChar((uchar)('0' + v10 % 10));
}
