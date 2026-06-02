#include "delay.h"
#include "stm32f10x.h"
volatile uint32_t TimingDelay;

void Delay(volatile uint32_t nTime)
{
    TimingDelay = nTime;
    while (TimingDelay);
}
