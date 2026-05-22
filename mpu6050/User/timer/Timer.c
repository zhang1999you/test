#include "stm32f10x.h"                  // Device header
#include "timer.h"
void Timer_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	
	TIM_InternalClockConfig(TIM1);
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
	
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_Cmd(TIM1, ENABLE);
}

void TIM1_Control_Init(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 1. 开启 TIM1 时钟 (注意：TIM1 在 APB2 总线上！)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); 

    // 2. 定时器基础配置：72MHz / 7200 = 10kHz (计数器每 0.1ms 加 1)
    TIM_TimeBaseStructure.TIM_Period = 100 - 1;          // 自动重装载值 100，即 100 * 0.1ms = 10ms
    TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;      // 预分频器
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;     // TIM1高级定时器特有，必须显式设为 0
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // 3. 开启 TIM1 更新中断
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);

    // 4. 配置 NVIC 中断通道
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;   // TIM1 更新中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级（建议调高，优先于串口）
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;    
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 5. 使能 TIM1
    TIM_Cmd(TIM1, ENABLE);
}
/*
void TIM1_UP_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
	{
		
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	}
}
*/
