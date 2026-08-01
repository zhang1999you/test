#ifndef __MAIN_H
#define __MAIN_H
#include <stdbool.h>
#include "pid/PID.h"

void SystemClock_Config(void);
void Parse_PID_Commands(void);
#define FILTER_ALPHA  0.005f  // 降低这个值，收敛变快；升高这个值，抗震动变强




#define BLE_SPEED_FULL_SCALE        100.0f
#define MAX_COMMAND_SPEED_CM_S       5.0f   // 先从 15 开始，稳定后可试 20
#define SPEED_TARGET_ACCEL_CM_S2     15.0f
#define SPEED_LOOP_DT                0.025f
#define SPEED_DRIVE_TILT_MAX_DEG      0.8f   // 蓝牙正常行驶最大倾角
#define SPEED_BRAKE_TILT_MAX_DEG      0.8f   // 被推快后允许更强刹车

#define SPEED_CMD_FF_GAIN            0.10f  // ° / (cm/s)
#define SPEED_CMD_FF_MAX_DEG          1.2f
#define SPEED_TOTAL_TILT_MAX_DEG      1.6f

#define DRIVE_START_PWM              12
#define DRIVE_START_SPEED_CM_S       1.0f
#define DRIVE_START_ANGLE_ERR_DEG    0.4f

extern volatile float speedCommandCmS;


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
