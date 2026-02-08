/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   ²âÊÔled
  ******************************************************************************
  * @attention
  *
  * ÊµÑéÆ½Ì¨:Ò°»ð F103-MINI STM32 ¿ª·¢°å 
  * ÂÛÌ³    :http://www.firebbs.cn
  * ÌÔ±¦    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 
	
#include "stm32f10x.h"
#include "main.h"
#include "bsp_debug_usart.h"
#include "bsp_usart_blt.h"
#include "hc05/bsp_hc05.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#define SOFT_DELAY Delay(0x0FFFFF);


void Delay(__IO u32 nCount); 
extern ReceiveData BLT_USART_ReceiveData;

/**
  * @brief  Ö÷º¯Êý
  * @param  ÎÞ  
  * @retval ÎÞ
  */

int main(void)
{	
	/* 1. ÏµÍ³¸´Î»ÒÔ¼°Æô¶¯ HSE/PLL µÈ */
    SystemInit();                 // CMSIS: ¸´Î»²¢ÅäÖÃÏµÍ³Ê±ÖÓÔ´µ½Ä¬ÈÏ×´Ì¬
    SystemClock_Config();         // SPL: Äú×Ô¼ºÐ´µÄ 72MHz Ê±ÖÓÅäÖÃº¯Êý
	SysTick_Config(SystemCoreClock/1000);

	DEBUG_USART_Config();
	BLT_USART_Config();
	//Usart_SendString( BLT_USART,"Debug´®¿ÚÕý³£\n");

	while (1)
	{
//		uint16_t len;
//		char * redata;
//		if(BLT_USART_ReceiveData.receive_data_flag == 1)
//		{
//			    BLT_USART_ReceiveData.uart_buff[BLT_USART_ReceiveData.datanum] = '\0';
//                redata = get_rebuff(&len); 
//                if(len>0)
//                {
//                    if(strstr(redata,"OK"))				
//                    {
//						Usart_SendString( BLT_USART,"sucess\r\n");
//                    }
//                }
//				clean_rebuff();
//		}
		Usart_SendString( USART1,"zhangxu\r\n");
		Delay(10000);
	}
}


void SystemClock_Config(void)
{
    ErrorStatus HSEStartUpStatus;

    /* 1. ?? RCC ????????,????????????? */
    RCC_DeInit();

    /* 2. ?? HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* 3. ?? HSE ?? */
    HSEStartUpStatus = RCC_WaitForHSEStartUp();
    if (HSEStartUpStatus == SUCCESS)
    {
        /* 4. ?? Flash ?????? Flash ????? 2 ????? 
         *    (72MHz ???????,??????????:
         *     0–24MHz:0 WS
         *     24–48MHz:1 WS
         *     48–72MHz:2 WS)
         */
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        FLASH_SetLatency(FLASH_Latency_2);

        /* 5. ?? AHB(HCLK) = SYSCLK / 1 */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        /* 6. ?? APB2(PCLK2) = HCLK / 1 */
        RCC_PCLK2Config(RCC_HCLK_Div1);

        /* 7. ?? APB1(PCLK1) = HCLK / 2 
         *    (APB1 ?? 36MHz,?? 72MHz ???? /2)
         */
        RCC_PCLK1Config(RCC_HCLK_Div2);

        /* 8. ?? PLL:
         *    - PLL ???:HSE / 1 (???? HSE/2 ???,?? RCC_PLLSource_HSE_Div2)
         *    - PLL ????:9 ? ?? 8MHz * 9 = 72MHz
         */
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

        /* 9. ?? PLL */
        RCC_PLLCmd(ENABLE);

        /* 10. ?? PLL ?? */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

        /* 11. ?? PLL ??????? */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        /* 12. ??????,?????????? PLL */
        while (RCC_GetSYSCLKSource() != 0x08);  
        // 0x08 ?? PLL ??? SYSCLK;????? RCC_SYSCLKSource_PLLCLK<<2 ????
    }
    else
    {
        /* ?? HSE ????,??????????????? */
        while (1) {;}
    }
}




/*********************************************END OF FILE**********************/
