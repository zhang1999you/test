#ifndef __HC05_H
#define	__HC05_H

#include "stm32f10x.h"

#define BLT_USART_TX_GPIO_CLK  RCC_APB2Periph_GPIOB
#define BLT_USART_RX_GPIO_CLK  RCC_APB2Periph_GPIOB
#define BLT_USART_TX_GPIO_PORT    	  GPIOB	
#define BLT_USART_RX_GPIO_PORT    	  GPIOB	
#define BLT_USART_TX_GPIO_PIN  GPIO_Pin_10
#define BLT_USART_RX_GPIO_PIN  GPIO_Pin_11



#define BLT_USART  USART3
#define BLT_USART_CLK  RCC_APB1Periph_USART3
#define BLT_USART_IRQn         USART3_IRQn
#define BLT_USART_BAUDRATE     115200



#define BLT_INT_GPIO_CLK    RCC_APB2Periph_GPIOA
#define BLT_KEY_GPIO_CLK    RCC_APB2Periph_GPIOA
#define BLT_INT_GPIO_PORT    	  GPIOA		
#define BLT_KEY_GPIO_PORT    	  GPIOA		
#define BLT_INT_GPIO_PIN   GPIO_Pin_5
#define BLT_KEY_GPIO_PIN   GPIO_Pin_7
uint8_t HC05_Init(void);


#endif /* _HC05_H */


