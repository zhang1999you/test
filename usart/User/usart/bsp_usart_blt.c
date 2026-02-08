
#include "bsp_usart_blt.h"
#include <stdarg.h>
#include "hc05/bsp_hc05.h"



#include "stm32f10x.h"            // StdPeriph ????,?? RCC/GPIO/USART/NVIC ????
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "misc.h"                 // NVIC_InitTypeDef ????


#define BLT_USART3                  USART3
#define BLT_USART3_CLK              RCC_APB1Periph_USART3
#define BLT_USART3_TX_GPIO_CLK      RCC_APB2Periph_GPIOB
#define BLT_USART3_RX_GPIO_CLK      RCC_APB2Periph_GPIOB
#define BLT_USART3_TX_PIN           GPIO_Pin_10
#define BLT_USART3_RX_PIN           GPIO_Pin_11
#define BLT_USART3_GPIO_PORT        GPIOB
#define BLT_USART3_IRQn             USART3_IRQn
#define BLT_USART_BAUD_RATE         38400


USART_InitTypeDef  BLT_USART_InitStruct;
USART_TypeDef     *BLT_USARTx = BLT_USART3;
extern ReceiveData BLT_USART_ReceiveData;


void BLT_USART_Config(void)
{
	    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(BLT_USART_CLK, ENABLE);

    /* 2. ???? GPIO ????(TX ? RX ????? GPIOA,???????) */
    RCC_APB2PeriphClockCmd(BLT_USART_TX_GPIO_CLK | BLT_USART_RX_GPIO_CLK | RCC_APB2Periph_AFIO,ENABLE);

    /* 3. ?? TX(PA9)??????? */
    GPIO_InitStructure.GPIO_Pin   = BLT_USART_TX_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(BLT_USART_TX_GPIO_PORT, &GPIO_InitStructure);

    /* 4. ?? RX(PA10)????? */
    GPIO_InitStructure.GPIO_Pin   = BLT_USART_RX_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(BLT_USART_TX_GPIO_PORT, &GPIO_InitStructure);

    /* 5. ?? NVIC ???:?? USART ?? */
    NVIC_InitStructure.NVIC_IRQChannel                   = BLT_USART_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;    // ???????????
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 6. ?? USART ?? */
    USART_InitStructure.USART_BaudRate            = BLT_USART_BAUDRATE;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(BLT_USART, &USART_InitStructure);

    /* 7. ?? USART ????(RXNE)?????(IDLE) */
    USART_ITConfig(BLT_USART, USART_IT_RXNE, ENABLE);
	USART_ITConfig(BLT_USART, USART_IT_IDLE, ENABLE);
    /* 8. ???? USART ?? */
    USART_Cmd(BLT_USART, ENABLE);
	
	
	
	
	
//	
//	
// GPIO_InitTypeDef  GPIO_InitStructure;
//    USART_InitTypeDef USART_InitStructure;
//    NVIC_InitTypeDef  NVIC_InitStructure;

//    /* 1. 使能外设时钟
//     *    - USART3 在 APB1 总线上
//     *    - GPIOB 和 AFIO 在 APB2 总线上
//     */
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
//    RCC_APB2PeriphClockCmd(
//        RCC_APB2Periph_GPIOB  |
//        RCC_APB2Periph_AFIO, 
//        ENABLE
//    );

//    /* 2. 配置 PB10 为复用推挽输出 (USART3 TX) */
//    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
//    GPIO_Init(GPIOB, &GPIO_InitStructure);

//    /* 3. 配置 PB11 为浮空输入 (USART3 RX) */
//    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//    GPIO_Init(GPIOB, &GPIO_InitStructure);

//    /* 4. 配置 USART3 参数 */
//    USART_InitStructure.USART_BaudRate            = BLT_USART_BAUD_RATE;
//    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
//    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
//    USART_InitStructure.USART_Parity              = USART_Parity_No;
//    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
//    USART_Init(USART3, &USART_InitStructure);

//    /* 5. 配置 USART3 中断优先级并使能 */
//    NVIC_InitStructure.NVIC_IRQChannel                   = USART3_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
//    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);

//    /* 6. 使能 USART3 接收中断 (RXNE) 和 空闲中断 (IDLE) */
//    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
//    USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);

//    /* 7. 最后使能 USART3 外设 */
//    USART_Cmd(USART3, ENABLE);	
	
	
	
	

}


//获取接收到的数据和长度
char *get_rebuff(uint16_t *len) 
{
    *len = BLT_USART_ReceiveData.datanum;
    return (char *)&BLT_USART_ReceiveData.uart_buff;
}


//清空缓冲区
void clean_rebuff(void)
{
    uint16_t i=UART_BUFF_SIZE+1;
    BLT_USART_ReceiveData.datanum = 0;
	  BLT_USART_ReceiveData.receive_data_flag = 0;
	while(i)
		BLT_USART_ReceiveData.uart_buff[--i]=0;
}




/******************* (C) COPYRIGHT 2024 WildFire Team *****END OF FILE************/
