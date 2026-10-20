#ifndef __TEMP_H__
#define __TEMP_H__

#include "main.h"

bit TEMP_Read(int *t10);	/* 1 = 成功；*t10 = 温度 x10（256 表示 25.6℃） */

#endif
