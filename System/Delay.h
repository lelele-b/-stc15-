#ifndef __DELAY_H__
#define __DELAY_H__

#include "main.h"

void Delay_Init(void);
bit  Delay_T2Alive(void);
void Delay_Ms(uint xms);
void Delay_Us(uint us);

#endif
