#ifndef __UART_H__
#define __UART_H__

#include "main.h"

void Uart1_Init(void);					/* 9600 8N1：定时器1 模式2，0xFD @11.0592MHz */
void Uart_SendChar(uchar dat);			/* 进环形缓冲，不阻塞 */
void Uart_SendStr(char *str);
void Uart_SendUInt(uint value);
void Uart_SendUInt2(uchar value);	/* 两位，不足补 0 */
void Uart_SendFixed1(int v10);		/* 一位小数，如 25.6 / -12.3 */

#endif
