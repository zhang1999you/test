#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

extern volatile uint32_t TimingDelay;  // 告诉其他文件这是个外部全局变量

void Delay(volatile uint32_t nTime);

#endif
