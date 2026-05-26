#ifndef __MAIN_H
#define __MAIN_H
#include <stdbool.h>
#include "pid/PID.h"

void SystemClock_Config(void);
#define FILTER_ALPHA  0.01f  // 降低这个值，收敛变快；升高这个值，抗震动变强

extern short Accel[3];
extern short Gyro[3];
extern short Temp;
extern int16_t ax,ay,az;//加速度计的结果，单位g
extern int16_t gx,gy,gz;
extern float angleAcc;
extern float angleGyro;
extern float angle;

extern volatile int8_t PWML, PWMR;
extern volatile int8_t PWMAve, PWMDif;

extern volatile float SPEEDL, SPEEDR;
extern volatile float SPEEDAve, SPEEDDif;

extern bool runFlag;
extern float angleAccOffset;
extern PID_t AnglePID;
extern PID_t SpeedPID;
#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
