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


void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
        
        GPIO_SetBits(GPIOB, GPIO_Pin_0);

        // 1. 姿态解算
        MPU6050ReadAcc(Accel); MPU6050ReadGyro(Gyro);
        ax = Accel[0]; az = Accel[2]; gy = Gyro[1]; gy -= 28; 
        angleAcc = -atan2(ax, az) / 3.14159f * 180.0f + angleAccOffset; 
        angleGyro = angle + gy / 32768.0f*2000 * 0.01f; 
        angle = FILTER_ALPHA * angleAcc + (1.0f - FILTER_ALPHA) * angleGyro; 

        // 2. 直立环 PID 计算与物理输出
        if (runFlag)
        {
            AnglePID.Actual = angle;
            PID_Update(&AnglePID); 
            
            PWMAve = (int8_t)AnglePID.Out;
            // PWMDif 是由 SysTick 速度环计算并在后台更新的
            PWML = PWMAve + PWMDif/2; 
            PWMR = PWMAve - PWMDif/2;
            
            if (PWML > 0) {
                PWML += 50;
            } else if (PWML < 0) {
                PWML -= 50;
            }
            if (PWMR > 0) {
                PWMR += 50;
            } else if (PWMR < 0) {
                PWMR -= 50;
            }
            if(PWML > 100) PWML = 100;
            if(PWML < -100) PWML = -100;
            if(PWMR > 100) PWMR = 100;
            if(PWMR < -100) PWMR = -100;

            Motor_SetPWM(1, PWML);
            Motor_SetPWM(2, PWMR);
        }
        else
        {
            Motor_SetPWM(1, 0);
            Motor_SetPWM(2, 0);
            PID_Init(&AnglePID);
        }
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);
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
