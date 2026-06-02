#ifndef __USART3_H
#define	__USART3_H
#include "stm32f10x.h"
#include <stdio.h>

#define UART_BUFF_SIZE      1024
typedef struct 
{
  volatile    uint16_t datanum;
  uint8_t     uart_buff[UART_BUFF_SIZE];		
  uint8_t     receive_data_flag;
}ReceiveData;


void BLT_USART_Config(void);
char *get_rebuff(uint16_t *len);
void clean_rebuff(void);

#endif /* __USART3_H */
