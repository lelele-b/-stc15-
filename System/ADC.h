#ifndef __ADC_H__
#define __ADC_H__

#include "main.h"

#define ADC_CH_LIGHT	1	/* 板载光敏电阻 RD1 分压 -> AIN1 */
#define ADC_CH_POT		3	/* 电位器 RB2 -> AIN3 */

void  ADC_Init(void);
uchar ADC_Read(uchar ch);	/* 读 PCF8591 通道 ch(0~3) 的 8 位结果 */
uchar ADC_ReadAvg(uchar ch);	/* 连采 4 次取平均，抗滑片抖动 */

#endif
