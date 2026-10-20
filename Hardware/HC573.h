#ifndef __HC573_H__
#define __HC573_H__

#include "main.h"

void SelectHC573(uchar n);	/* 4=LED 5=蜂鸣器/继电器 6=数码管位选 7=数码管段选 */
void Init_Periph(void);

#endif
