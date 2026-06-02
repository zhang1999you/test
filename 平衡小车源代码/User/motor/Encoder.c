#include "stm32f10x.h"                  // Device header

void Encoder_Init(void)
{
    /* 1. 开启时钟 */
    // PA0, PA1 -> TIM2 (APB1)
    // PA6, PA7 -> TIM3 (APB1)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 开启 TIM2 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); // 开启 TIM3 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    /* 2. 配置 GPIO */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    // 修改引脚为 PA0, PA1, PA6, PA7
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
        
    /* 3. 配置时基单元 */
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;       // ARR
    TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;         // PSC
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure); // 初始化 TIM2
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure); // 初始化 TIM3
    
    /* 4. 配置输入捕获 (IC) */
    TIM_ICInitTypeDef TIM_ICInitStructure;
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 0xF; // 滤波器
    
    // TIM2 通道配置 (PA0, PA1)
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInit(TIM2, &TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInit(TIM2, &TIM_ICInitStructure);
    
    // TIM3 通道配置 (PA6, PA7)
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInit(TIM3, &TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInit(TIM3, &TIM_ICInitStructure);
    
    /* 5. 配置编码器接口模式 */
    // 注意：这里配置的是定时器如何响应 TI1 和 TI2 的电平变化
    TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    
    /* 6. 使能定时器 */
    TIM_Cmd(TIM2, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
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
		Temp = TIM_GetCounter(TIM2);
		TIM_SetCounter(TIM2, 0);
		return Temp;
	}
	return 0;
}
