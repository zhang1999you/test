#ifndef __DEBUG_USART_H
#define	__DEBUG_USART_H

#include <stdio.h>
#include "stm32f10x.h"



#define DEBUG_USART              USART1
#define DEBUG_USART_CLK          RCC_APB2Periph_USART1
#define DEBUG_USART_TX_GPIO_CLK  RCC_APB2Periph_GPIOA
#define DEBUG_USART_RX_GPIO_CLK  RCC_APB2Periph_GPIOA
#define DEBUG_USART_TX_GPIO_PIN  GPIO_Pin_9
#define DEBUG_USART_RX_GPIO_PIN  GPIO_Pin_10
#define DEBUG_USART_TX_GPIO_PORT GPIOA
#define DEBUG_USART_RX_GPIO_PORT GPIOA
#define DEBUG_USART_IRQn         USART1_IRQn
#define DEBUG_USART_BAUDRATE     115200



void DEBUG_USART_Config(void);
void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch);
void Usart_SendString( USART_TypeDef * pUSARTx, char *str);
void Usart_SendHalfWord( USART_TypeDef * pUSARTx, uint16_t ch);


#endif /* __USART1_H */
