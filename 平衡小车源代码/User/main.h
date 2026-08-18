#ifndef __MAIN_H
#define __MAIN_H
#include <stdbool.h>
#include "pid/PID.h"

void SystemClock_Config(void);
void Parse_PID_Commands(void);
#define FILTER_ALPHA  0.005f  // 降低这个值，收敛变快；升高这个值，抗震动变强




#define BLE_SPEED_FULL_SCALE        100.0f
#define MAX_COMMAND_SPEED_CM_S      30.0f
#define SPEED_TARGET_ACCEL_CM_S2     17.0f
#define SPEED_LOOP_DT                0.025f

#define BLE_TURN_FULL_SCALE         100.0f
#define MAX_TURN_DIFF_CM_S           30.0f
#define TURN_TARGET_ACCEL_CM_S2      80.0f
#define TURN_FEEDBACK_FILTER_ALPHA    0.25f
#define TURN_PWM_MAX                 32.0f
#define SPEED_DRIVE_PWM_MAX           28.0f
#define SPEED_BRAKE_PWM_MAX           30.0f

#define SPEED_PWM_FF_GAIN              0.65f // PWM / (cm/s)
#define SPEED_PWM_FF_MAX              10.0f
#define SPEED_TOTAL_PWM_MAX           40.0f
#define SPEED_PWM_OUTPUT_SIGN           1.0f // 速度输出先用于建立行驶倾角，不直接按编码器方向翻转

#define SPEED_TILT_ERROR_GAIN           0.05f // degrees / (cm/s error)
#define SPEED_TILT_ERROR_MAX            1.5f
#define SPEED_HARD_LIMIT_CM_S         65.0f
#define SPEED_HARD_RELEASE_CM_S       45.0f
#define SPEED_EMERGENCY_PWM            40.0f
#define SPEED_EMERGENCY_TILT_DEG        3.0f

#define DRIVE_START_PWM              12
#define DRIVE_START_SPEED_CM_S       1.0f
#define DRIVE_START_ANGLE_ERR_DEG    0.4f
#define MOTION_COMMAND_TIMEOUT_MS     500U

/* 自动起身：TIM1 控制周期为 5ms。 */
#define CONTROL_LOOP_PERIOD_MS              5U
#define BALANCE_FALL_ANGLE_DEG              35.0f
#define SELF_RIGHT_CAPTURE_ANGLE_DEG        18.0f
#define SELF_RIGHT_PWM_REDUCE_ANGLE_DEG     28.0f
#define SELF_RIGHT_MAX_START_ANGLE_DEG     120.0f
#define SELF_RIGHT_ABORT_ANGLE_DEG         135.0f
#define SELF_RIGHT_TIMEOUT_MS             2000U
#define SELF_RIGHT_TIMEOUT_TICKS \
    (SELF_RIGHT_TIMEOUT_MS / CONTROL_LOOP_PERIOD_MS)

/*
 * angle > 0 时默认给逻辑负 PWM，沿用原起身代码的方向约定。
 * 如果实车首次测试时向地面压得更紧，只需把 1 改成 -1。
 */
#define SELF_RIGHT_DIRECTION_SIGN            1
#define SELF_RIGHT_PWM_HIGH                  99
#define SELF_RIGHT_PWM_LOW                   72

#define SELF_RIGHT_STATE_IDLE                 0U
#define SELF_RIGHT_STATE_DRIVE                1U

#define SELF_RIGHT_EVENT_NONE                 0U
#define SELF_RIGHT_EVENT_STARTED              1U
#define SELF_RIGHT_EVENT_CAPTURED             2U
#define SELF_RIGHT_EVENT_TIMEOUT              3U
#define SELF_RIGHT_EVENT_BAD_ANGLE            4U
#define SELF_RIGHT_EVENT_FALL_STOP            5U

extern volatile float speedCommandCmS;
extern volatile float turnCommandDiffCmS;
extern volatile uint16_t motionCommandAgeMs;
extern volatile bool motionCommandWatchdogActive;
extern volatile bool motionStopRequest;
extern volatile bool speedEmergencyBrake;
extern volatile bool gyroCalibrationRequest;
extern volatile bool gyroCalibrationBusy;
extern volatile uint8_t gyroCalibrationResult;
extern volatile float gyroXOffset;
extern volatile uint8_t selfRightState;
extern volatile uint8_t selfRightEvent;
extern volatile float selfRightEventAngle;


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
extern volatile int16_t encoderDeltaL, encoderDeltaR;
extern volatile int32_t encoderTotalL, encoderTotalR;

extern volatile bool runFlag;
extern float angleAccOffset;
extern PID_t AnglePID;
extern PID_t SpeedPID;
extern PID_t TurnPID;
#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
