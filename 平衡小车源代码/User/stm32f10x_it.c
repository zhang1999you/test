/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTI
  
  AL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "delay.h"
#include "main.h"
#include "stm32f10x.h"
#include "mpu6050/mpu6050.h"
#include <math.h>
#include "motor/Encoder.h"
#include "motor/Motor.h"
/**
 * @brief  配置平衡小车全系统的中断优先级
 * @note   基于 NVIC 优先级分组 2（2位抢占，2位响应）
 * 确保 TIM1_UP_IRQn 拥有至高无上的抢占权（0,0）
 */
void Interrupt_Priority_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    uint32_t temp_priority;
    // 1. 统一设置 NVIC 每个中断拥有 2位抢占(0~3) 和 2位响应(0~3)
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // ============================================================
    // ? 独一档：TIM1 更新中断（10ms 姿态解算 + 直立环）
    // 抢占优先级: 0 (最高，不可被任何中断打断)
    // 响应优先级: 0
    // ============================================================
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // ============================================================
    // ? 第二档：SysTick 内核中断（1ms 滴答 + 50ms 速度环）
    // 抢占优先级: 1 (可以打断串口，但会被 TIM1 抢占)
    // 响应优先级: 0
    // 注意：SysTick 是内核中断，不走标准 NVIC 寄存器，需使用内核函数设置
    // ============================================================
    temp_priority = NVIC_EncodePriority(NVIC_PriorityGroup_2, 1, 0);
    NVIC_SetPriority(SysTick_IRQn, temp_priority);

    // ============================================================
    // ? 第三档：USART1 中断（PA9/PA10 调试/蓝牙 HC-08 接收）
    // 抢占优先级: 2 (会被 TIM1 和 SysTick 抢占)
    // 响应优先级: 0
    // ============================================================
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // ============================================================
    // ? 第三档：USART2 中断（PA2/PA3 WiFi ESP8266 接收）
    // 抢占优先级: 2 (会被 TIM1 和 SysTick 抢占)
    // 响应优先级: 1 (若与 USART1 同时到达，USART1 先执行)
    // ============================================================
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/* 定义定时器：一个 5ms、一个 50ms */
volatile SwTimer_t swTimers[NUM_TIMERS] = {
    { 10, 0, 0 },   //10ms 定时器
		{ 100, 0, 0 }, //打印速度
    { 50, 0, 0 },   //角度环
    { 50, 0, 0 },   //速度环 转向环
};


/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	if (TimingDelay != 0)
	{
		TimingDelay--;
	}
	//任务定时器
	for (int i = 0; i < NUM_TIMERS; i++) {
			if (++swTimers[i].counter >= swTimers[i].period_ms) {
					swTimers[i].counter = 0;
					swTimers[i].flag    = 1;
			}
	}
	
}

float sensorAngleOffset = 0.0f;//安装误差校准

int8_t Motor_DeadzoneCompensate(int8_t pwm)//死区补偿
{
    const int8_t MIN_EFFECTIVE_PWM = 10;
    const int8_t EPSILON = 2;

    if (pwm >= -EPSILON && pwm <= EPSILON)
        return 0;

    if (pwm > 0 && pwm < MIN_EFFECTIVE_PWM)
        return MIN_EFFECTIVE_PWM;

    if (pwm < 0 && pwm > -MIN_EFFECTIVE_PWM)
        return -MIN_EFFECTIVE_PWM;

    return pwm;
}

const float wheelCircumference=3.1416*7;//轮子周长cm
volatile float speedAngleOffset = 0.0f;
volatile bool speedEmergencyBrake = false;
static float speedAveFiltered=0.0f;
static float turnPwmFiltered=0.0f;
volatile int16_t encoderDeltaL = 0, encoderDeltaR = 0;
volatile int32_t encoderTotalL = 0, encoderTotalR = 0;

#define GYRO_CALIBRATION_SETTLE_TICKS  100U
#define GYRO_CALIBRATION_SAMPLES       400U
#define GYRO_CALIBRATION_MAX_RAW_SPAN  250

void TIM1_UP_IRQHandler(void)
{
    static uint8_t global_tick = 0; // 统一的低频任务节拍器
    static bool gyroCalibrationActive = false;
    static bool gyroCalibrationJustFinished = false;
    static uint16_t gyroCalibrationSettleCount = 0;
    static uint16_t gyroCalibrationCount = 0;
    static int32_t gyroCalibrationSum = 0;
    static int16_t gyroCalibrationMin = 32767;
    static int16_t gyroCalibrationMax = -32768;
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
    {
        float correctedGx;

        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
        
        // GPIO_SetBits(GPIOB, GPIO_Pin_0); //调试用，查看中断频率

        // 1. 姿态解算  10ms 更新一次
        MPU6050ReadAcc(Accel); MPU6050ReadGyro(Gyro);
        if (gyroCalibrationRequest)
        {
            gyroCalibrationRequest = false;
            gyroCalibrationActive = true;
            gyroCalibrationSettleCount = 0;
            gyroCalibrationCount = 0;
            gyroCalibrationSum = 0;
            gyroCalibrationMin = 32767;
            gyroCalibrationMax = -32768;
        }

        if (gyroCalibrationActive)
        {
            if (gyroCalibrationSettleCount < GYRO_CALIBRATION_SETTLE_TICKS)
            {
                gyroCalibrationSettleCount++;
            }
            else
            {
                int16_t rawGx = Gyro[0];

                gyroCalibrationSum += rawGx;
                gyroCalibrationCount++;

                if (rawGx < gyroCalibrationMin)
                    gyroCalibrationMin = rawGx;
                if (rawGx > gyroCalibrationMax)
                    gyroCalibrationMax = rawGx;

                if (gyroCalibrationCount >= GYRO_CALIBRATION_SAMPLES)
                {
                    if ((int32_t)gyroCalibrationMax - gyroCalibrationMin <=
                        GYRO_CALIBRATION_MAX_RAW_SPAN)
                    {
                        gyroXOffset = (float)gyroCalibrationSum /
                                      (float)GYRO_CALIBRATION_SAMPLES;
                        gyroCalibrationResult = 1;
                    }
                    else
                    {
                        /* Keep the previous offset when the car moved during sampling. */
                        gyroCalibrationResult = 2;
                    }

                    gyroCalibrationBusy = false;
                    gyroCalibrationActive = false;
                    gyroCalibrationJustFinished = true;
                }
            }
        }

        correctedGx = (float)Gyro[0] - gyroXOffset;
        if (correctedGx > 32767.0f)
            correctedGx = 32767.0f;
        else if (correctedGx < -32768.0f)
            correctedGx = -32768.0f;

        ay = Accel[1]; az = Accel[2]; gx = (int16_t)correctedGx;
        angleAcc = atan2f(ay, az) / 3.14159f * 180.0f + sensorAngleOffset ; 
        if (gyroCalibrationJustFinished)
        {
            gyroCalibrationJustFinished = false;
            angleGyro = angleAcc;
            angle = angleAcc;
            PID_Init(&AnglePID);
            PID_Init(&SpeedPID);
        }
        else
        {
            angleGyro = angle + correctedGx / 32768.0f * 2000.0f * 0.005f;
            angle = FILTER_ALPHA * angleAcc + (1.0f - FILTER_ALPHA) * angleGyro;
        }
        // 2. 角度环
        if (runFlag)
        {
            float driveTiltBias;
            float tiltSpeedError;

            tiltSpeedError = SpeedPID.Target - SpeedPID.Actual;

            if (speedEmergencyBrake)
            {
                /* 紧急状态下倾向实际运动方向的反方向，协助电机刹车。 */
                driveTiltBias = (SpeedPID.Actual >= 0.0f) ?
                                -SPEED_EMERGENCY_TILT_DEG :
                                 SPEED_EMERGENCY_TILT_DEG;
            }
            else
            {
                /*
                 * 倾角只跟随速度误差并连续过零：低于目标时前倾，
                 * 超过目标后平滑改为后倾，避免目标倾角来回跳变。
                 */
                driveTiltBias = SPEED_TILT_ERROR_GAIN * tiltSpeedError;
                if (driveTiltBias > SPEED_TILT_ERROR_MAX)
                    driveTiltBias = SPEED_TILT_ERROR_MAX;
                else if (driveTiltBias < -SPEED_TILT_ERROR_MAX)
                    driveTiltBias = -SPEED_TILT_ERROR_MAX;
            }

            AnglePID.Target = angleAccOffset + driveTiltBias;
            AnglePID.Actual = angle;
            PID_Update(&AnglePID);

            /* 直立环保持平衡，速度环直接提供行驶 PWM。 */
            PWMAve = (int8_t)(AnglePID.Out + speedAngleOffset);
            PWML = PWMAve + PWMDif/2; 
            PWMR = PWMAve - PWMDif/2;

            if(PWML > 80) PWML = 80;
            if(PWML < -80) PWML = -80;
            if(PWMR > 80) PWMR = 80;
            if(PWMR < -80) PWMR = -80;

            /*
             * 行驶时把角度环的小输出映射到电机有效区间。
             * 停车时不补偿，避免改变原有的原地平衡效果。
             */
            if (fabsf(SpeedPID.Target) > 1.0f)
            {
                PWML = Motor_DeadzoneCompensate(PWML);
                PWMR = Motor_DeadzoneCompensate(PWMR);
            }

            Motor_SetPWM(1, PWML);
            Motor_SetPWM(2, -PWMR);
        }
        else
        {
            Motor_SetPWM(1, 0);
            Motor_SetPWM(2, 0);
            PWML = 0;
            PWMR = 0;
            PWMAve = 0;
            PID_Init(&AnglePID);
        }
        // GPIO_ResetBits(GPIOB, GPIO_Pin_0);//调试用，查看中断频率

        global_tick++;//
        if (global_tick >= 20) 
        {
            global_tick = 0;
        }
        // if (global_tick == 0 || global_tick == 10)
        // {
        //     Parse_PID_Commands();
        // }

        
        // 2. 速度环
        if (global_tick == 5 || global_tick == 10 || global_tick == 15 || global_tick == 0)
        {
          encoderDeltaL = -Encoder_Get(1);
          encoderDeltaR = -Encoder_Get(2);
          encoderTotalL += encoderDeltaL;
          encoderTotalR += encoderDeltaR;

          SPEEDL=encoderDeltaL / 44.0f / 0.025f / 9.27666f * wheelCircumference;//cm/s
          SPEEDR=encoderDeltaR / 44.0f / 0.025f / 9.27666f * wheelCircumference;
          SPEEDAve = (SPEEDL + SPEEDR) / 2;


          float speedFilterAlpha;
          if (fabsf(SPEEDAve) > 20.0f)
          {
              speedFilterAlpha = 0.40f;
          }
          else
          {
              speedFilterAlpha = 0.10f;
          }

          speedAveFiltered += speedFilterAlpha *
                              (SPEEDAve - speedAveFiltered);

          SPEEDDif = SPEEDL - SPEEDR;
          if (runFlag)
          {
              float targetStep;
              float targetDelta;
              float turnStep;
              float turnDelta;
              float speedError;
              bool overspeed;
              float speedFeedForward;
              /* 避免摇杆一推到底时，目标速度瞬间跳变 */
              targetStep = SPEED_TARGET_ACCEL_CM_S2 * SPEED_LOOP_DT;
              targetDelta = speedCommandCmS - SpeedPID.Target;

              if (targetDelta > targetStep)
                  SpeedPID.Target += targetStep;
              else if (targetDelta < -targetStep)
                  SpeedPID.Target -= targetStep;
              else
                  SpeedPID.Target = speedCommandCmS;

              /* Smooth the steering command before mixing left/right PWM. */
              turnStep = TURN_PWM_SLEW_PER_S * SPEED_LOOP_DT;
              turnDelta = turnCommandPwm - turnPwmFiltered;

              if (turnDelta > turnStep)
                  turnPwmFiltered += turnStep;
              else if (turnDelta < -turnStep)
                  turnPwmFiltered -= turnStep;
              else
                  turnPwmFiltered = turnCommandPwm;

              PWMDif = (int8_t)turnPwmFiltered;

              SpeedPID.Actual = speedAveFiltered;
              if (!speedEmergencyBrake &&
                  fabsf(SpeedPID.Actual) >= SPEED_HARD_LIMIT_CM_S)
              {
                  speedEmergencyBrake = true;
              }
              else if (speedEmergencyBrake &&
                       fabsf(SpeedPID.Actual) <= SPEED_HARD_RELEASE_CM_S)
              {
                  speedEmergencyBrake = false;
              }

              /* 高速紧急制动时先取消转向，保留两侧全部制动力。 */
              if (speedEmergencyBrake)
              {
                  turnPwmFiltered = 0.0f;
                  PWMDif = 0;
              }

              if (fabsf(SpeedPID.Target) < 0.5f &&fabsf(SpeedPID.Actual) < 2.0f)
              {
                  SpeedPID.Actual = 0.0f;
                  SpeedPID.ErrorInt = 0.0f;
              }

              speedError = SpeedPID.Target - SpeedPID.Actual;

              /*
               * When the speed error reverses, discard integral accumulated
               * for the old direction. Otherwise the car keeps braking after
               * it has already slowed below the target and comes to a stop.
               */
              if ((speedError > 1.0f && SpeedPID.ErrorInt < 0.0f) ||
                  (speedError < -1.0f && SpeedPID.ErrorInt > 0.0f))
              {
                  SpeedPID.ErrorInt = 0.0f;
              }

              /*
              * 实际速度已经超过目标速度时，开放更大的倾角用于刹车；
              * 正常行驶时仍限制较小倾角，避免满摇杆摔车。
              */
              overspeed =
                  (SpeedPID.Target > 0.5f &&
                  SpeedPID.Actual > SpeedPID.Target + 1.5f) ||

                  (SpeedPID.Target < -0.5f &&
                  SpeedPID.Actual < SpeedPID.Target - 1.5f) ||

                  (fabsf(SpeedPID.Target) <= 0.5f &&
                  fabsf(SpeedPID.Actual) > 1.5f);

              /* 超速时取消同方向前馈，避免抵消速度 PI 的刹车输出。 */
              speedFeedForward = overspeed ? 0.0f :
                                 SPEED_PWM_FF_GAIN * SpeedPID.Target;

              SpeedPID.OutMax = overspeed ?
                                SPEED_BRAKE_PWM_MAX :
                                SPEED_DRIVE_PWM_MAX;

              SpeedPID.OutMin = -SpeedPID.OutMax;

              PID_Update(&SpeedPID);

              if (speedEmergencyBrake)
              {
                  SpeedPID.ErrorInt = 0.0f;
                  SpeedPID.OutMax = SPEED_EMERGENCY_PWM;
                  SpeedPID.OutMin = -SPEED_EMERGENCY_PWM;
                  SpeedPID.Out = (SpeedPID.Actual >= 0.0f) ?
                                 -SPEED_EMERGENCY_PWM :
                                  SPEED_EMERGENCY_PWM;
                  speedFeedForward = 0.0f;
              }


              /* 摇杆直行前馈：Target 已经经过加减速斜坡处理 */


              if (speedFeedForward > SPEED_PWM_FF_MAX)
                  speedFeedForward = SPEED_PWM_FF_MAX;
              else if (speedFeedForward < -SPEED_PWM_FF_MAX)
                  speedFeedForward = -SPEED_PWM_FF_MAX;

              /*
               * 平衡车必须先移动轮子建立行驶倾角，再由直立环追上车身。
               * 因此这里保留与倾角目标一致的输出方向，不能按编码器
               * 速度符号直接翻转，否则两个控制量会互相抵消并导致摔倒。
               */
              speedAngleOffset = SPEED_PWM_OUTPUT_SIGN *
                                 (speedFeedForward + SpeedPID.Out);

              if (speedAngleOffset > SPEED_TOTAL_PWM_MAX)
                  speedAngleOffset = SPEED_TOTAL_PWM_MAX;
              else if (speedAngleOffset < -SPEED_TOTAL_PWM_MAX)
                  speedAngleOffset = -SPEED_TOTAL_PWM_MAX;
                        }
          else
          {
              speedAngleOffset = 0.0f;
              speedAveFiltered = 0.0f;
              turnPwmFiltered = 0.0f;
              speedEmergencyBrake = false;
              speedCommandCmS = 0.0f;
              turnCommandPwm = 0.0f;
              PID_Init(&SpeedPID);
              speedCommandCmS = 0.0f;
              PWMDif = 0;
          }

        }


    }
}

/**
  * @brief  USART1 中断服务函数 (处理 DEBUG_USART 数据)
  */
char RxBuffer[64];      // 实际的内存分配
uint8_t RxCounter = 0;
uint8_t RxFlag = 0;
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t res = USART_ReceiveData(USART1);
        
        // 如果没有接收完成
        if (RxFlag == 0)
        {
            // 收到换行符认为一条命令结束
            if (res == '\n' || res == '\r')
            {
                if (RxCounter > 0) 
                {
                    RxBuffer[RxCounter] = '\0'; // 添加字符串结束符
                    RxFlag = 1;                 // 标记可以解析了
                }
            }
            else
            {
                RxBuffer[RxCounter++] = res;
                if (RxCounter >= 64) RxCounter = 0; // 防止溢出
            }
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
    if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)
    {
        USART_ReceiveData(USART1); // 哪怕读出的是垃圾数据，也必须读一下来清除硬件锁死
    }
}



//  extern volatile int8_t PWML, PWMR;
// void USART1_IRQHandler(void)
// {
//     if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
//     {
//         uint8_t res = USART_ReceiveData(USART1);
//         if (res == 'W') {        // 比如收到 'W'，左右动力都增加
//             PWML+=10;
//             PWMR+=10;
//         } 
//         else if (res == 'S') {   // 比如收到 'S'，左右动力都减少
//             PWML-=10;
//             PWMR-=10;
//         }
//         else if (res == 'A') {   // 比如收到 'A'，左减右加（左转）
//             PWML-=10;
//             PWMR+=10;
//         }
//         else if (res == 'D') {   // 比如收到 'D'，左加右减（右转）
//             PWML+=10;
//             PWMR-=10;
//         }
//         // ----------------------------------------------------

//         // 清除中断标志位 (读取 USART_DR 寄存器后通常会自动清除，手动清除更保险)
//         USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//     }
// }


/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
