#include "stm32f10x.h"                  // Device header
#include "delay.h"

// void PWM_Init(void)
// {
// 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
// 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
// 	GPIO_InitTypeDef GPIO_InitStructure;
// 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
// 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
// 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
// 	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
// 	TIM_InternalClockConfig(TIM2);
	
// 	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
// 	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
// 	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
// 	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;		//ARR
// 	TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;		//PSC
// 	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
// 	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	
// 	TIM_OCInitTypeDef TIM_OCInitStructure;
// 	TIM_OCStructInit(&TIM_OCInitStructure);
// 	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
// 	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
// 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
// 	TIM_OCInitStructure.TIM_Pulse = 0;		//CCR
// 	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
// 	TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	
// 	TIM_Cmd(TIM2, ENABLE);
// }


void PWM_Init(void)
{
    /* 1. 开启时钟：改用 TIM4（APB1）并使用 GPIOB 的 PB8/PB9 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 2. 配置 GPIO：PB8 (TIM4_CH3) 和 PB9 (TIM4_CH4) */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;      // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; // 使用 B8 和 B9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /* 3. 配置时基单元 (与你之前的配置一致) */
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_InternalClockConfig(TIM4); // 使用内部时钟
    
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    // 使用较低的 PWM 频率以便在低占空比时增加平均电压/转矩
    // 例如：系统定时器时钟 72MHz, TIM clock = 72MHz -> PSC=71, ARR=999 => 1kHz PWM
    TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;     // ARR：自动重装值 (1000 ticks)
    TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;    // PSC：预分频值 (72)
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);
    
    /* 4. 配置输出比较 (PWM 模式) */
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);             // 给结构体赋默认值
    
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;                  // 初始占空比为 0 (CCR)
    
    // 初始化 TIM4 的通道 3 (PB8) 和通道 4 (PB9)
    TIM_OC3Init(TIM4, &TIM_OCInitStructure);
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);
    
    /* 5. 使能定时器 */
    TIM_Cmd(TIM4, ENABLE);
}


void PWM_SetCompare1(uint16_t Compare)
{
    /* 对应左电机使用 TIM4 CH3 (CCR3) */
    TIM_SetCompare3(TIM4, Compare);
}

void PWM_SetCompare2(uint16_t Compare)
{    
    /* 对应右电机使用 TIM4 CH4 (CCR4) */
    TIM_SetCompare4(TIM4, Compare);
}

