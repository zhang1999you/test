#include "stm32f10x.h"                  // Device header

void Encoder_Init(void)
{
	/* 1. 开启时钟 */
	// TIM3 挂在 APB1，TIM1 和 GPIOA 挂在 APB2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	/* 2. 配置 GPIO */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
		
	/* 3. 配置时基单元 (TIM3 & TIM1 分别配置) */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;		// ARR
	TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;		// PSC
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
	
	/* 4. 配置输入捕获 (TIM3 & TIM1) */
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_ICFilter = 0xF;
	
	// TIM3 通道配置
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);
	
	// TIM1 通道配置
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInit(TIM1, &TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(TIM1, &TIM_ICInitStructure);
	
	/* 5. 配置编码器接口模式 */
	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	
	/* 6. 使能定时器 */
	TIM_Cmd(TIM3, ENABLE);
	TIM_Cmd(TIM1, ENABLE);
}

int16_t Encoder_Get(uint8_t n)
{
	int16_t Temp;
	if (n == 1)
	{
		Temp = TIM_GetCounter(TIM3);
		TIM_SetCounter(TIM3, 0);
		return Temp;
	}
	else if (n == 2)
	{
		Temp = TIM_GetCounter(TIM1);
		TIM_SetCounter(TIM1, 0);
		return Temp;
	}
	return 0;
}
