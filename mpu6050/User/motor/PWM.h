#ifndef __PWM_H
#define __PWM_H

void PWM_Init(void);
void PWM_SetCompare1(uint16_t Compare);
void PWM_SetCompare2(uint16_t Compare);
void PWM_TestGPIO_Toggle(void);
void PWM_SetHalfDuty(void);
#endif
