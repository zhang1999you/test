#ifndef __DEBUG_USART_H
#define	__DEBUG_USART_H

#include <stdio.h>
#include "stm32f10x.h"



// #define DEBUG_USART              USART3
// #define DEBUG_USART_CLK          RCC_APB1Periph_USART3
// #define DEBUG_USART_TX_GPIO_CLK  RCC_APB2Periph_GPIOB
// #define DEBUG_USART_RX_GPIO_CLK  RCC_APB2Periph_GPIOB
// #define DEBUG_USART_TX_GPIO_PIN  GPIO_Pin_10
// #define DEBUG_USART_RX_GPIO_PIN  GPIO_Pin_11
// #define DEBUG_USART_TX_GPIO_PORT GPIOB
// #define DEBUG_USART_RX_GPIO_PORT GPIOB
// #define DEBUG_USART_IRQn         USART3_IRQn

#define DEBUG_USART             USART1                  // 更改为 USART1
#define DEBUG_USART_CLK         RCC_APB2Periph_USART1   // 注意：USART1 在 APB2 总线上
#define DEBUG_USART_TX_GPIO_CLK RCC_APB2Periph_GPIOA    // 更改为 GPIOA
#define DEBUG_USART_RX_GPIO_CLK RCC_APB2Periph_GPIOA    // 更改为 GPIOA
#define DEBUG_USART_TX_GPIO_PIN GPIO_Pin_9              // PA9
#define DEBUG_USART_RX_GPIO_PIN GPIO_Pin_10             // PA10
#define DEBUG_USART_TX_GPIO_PORT GPIOA                  // GPIOA
#define DEBUG_USART_RX_GPIO_PORT GPIOA                  // GPIOA
#define DEBUG_USART_IRQn        USART1_IRQn             // 中断号也要改
#define DEBUG_USART_BAUDRATE     115200



void DEBUG_USART_Config(void);
void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch);
void Usart_SendString( USART_TypeDef * pUSARTx, char *str);
void Usart_SendHalfWord( USART_TypeDef * pUSARTx, uint16_t ch);


#endif /* __USART1_H */
