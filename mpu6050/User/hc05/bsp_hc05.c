#include "hc05/bsp_hc05.h"
#include <string.h>
#include <stdio.h>
#include "stm32f10x.h"
#include "bsp_usart_blt.h"
#include "bsp_debug_usart.h"
#include "delay.h"

ReceiveData BLT_USART_ReceiveData;
ReceiveData DEBUG_USART_ReceiveData;
uint8_t HC05_Send_CMD(char* cmd,uint8_t clean)
{	 		 
  uint8_t retry=3;
	uint32_t i;
	uint16_t len;
  char * redata;
  Delay(100);
	while(retry--)
	{
        GPIO_SetBits(BLT_KEY_GPIO_PORT, BLT_KEY_GPIO_PIN);
        Usart_SendString(BLT_USART,(char *)cmd);
		
        i=200;              //初始化i，最长等待2秒
		Delay(10);
		do
        {
			
            if(BLT_USART_ReceiveData.receive_data_flag == 1)
			//if(1)
            {
				
                BLT_USART_ReceiveData.uart_buff[BLT_USART_ReceiveData.datanum] = '\0';
                redata = get_rebuff(&len); 
                if(len>0)
                {
                    if(strstr(redata,"OK"))				
                    {
                        //打印发送的蓝牙指令和返回信息
						Usart_SendString(DEBUG_USART, "send CMD: ");
						Usart_SendString(DEBUG_USART, cmd);
						Usart_SendString(DEBUG_USART, "\r\n");
						
						Usart_SendString(DEBUG_USART, "recv back: ");
						Usart_SendString(DEBUG_USART, redata);
						Usart_SendString(DEBUG_USART, "\r\n");
                        
                        if(clean==1)
                            clean_rebuff();
                        
                        return 0; //AT指令成功
                    }
                }
            }
            else
			{

			}
            Delay(10);
            
        }while( --i ); //继续等待
        Usart_SendString(DEBUG_USART, "send CMD: ");
		Usart_SendString(DEBUG_USART, cmd);
		Usart_SendString(DEBUG_USART, "\r\n");
		
		
		Usart_SendString(DEBUG_USART, "recv back: ");
		Usart_SendString(DEBUG_USART, redata);
		Usart_SendString(DEBUG_USART, "\r\n");
        
		// 将数字转换为字符串
		Usart_SendString(DEBUG_USART, "HC05 send CMD fail %d times");
		Usart_SendString(DEBUG_USART, (char *)retry);
		Usart_SendString(DEBUG_USART, "\r\n");
		
    
    }
	Usart_SendString(DEBUG_USART, "HC05 send CMD fail");
	if(clean==1)
		clean_rebuff();

	return 1; //AT指令失败
}





static void HC05_GPIO_Config(void)
{
	/*配置GPIO
    *INT -- 浮空输入 -- 判断是否模块连接
    *KEY -- 推挽输出 -- 决定是否模块进入AT模式
    */
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(BLT_INT_GPIO_CLK | BLT_KEY_GPIO_CLK, ENABLE);
    /* 2. 配置 INT 引脚：浮空输入，用于判断模块是否连接 */
    GPIO_InitStructure.GPIO_Pin   = BLT_INT_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz;
    GPIO_Init(BLT_INT_GPIO_PORT, &GPIO_InitStructure);

    /* 3. 配置 KEY 引脚：推挽输出，用于决定模块进入 AT 模式 */
    GPIO_InitStructure.GPIO_Pin  = BLT_KEY_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz;
    GPIO_Init(BLT_KEY_GPIO_PORT, &GPIO_InitStructure);

    /* 4. 默认将 KEY 引脚拉低（模块正常工作） */
    //GPIO_ResetBits(BLT_KEY_GPIO_PORT, BLT_KEY_GPIO_PIN);
	GPIO_SetBits(BLT_KEY_GPIO_PORT, BLT_KEY_GPIO_PIN);

}

uint8_t HC05_Init(void)
{
	//分两步 1、初始化int和key的GPIO    2、初始化uart、GPIO
	//1、
	HC05_GPIO_Config();//初始化int和key的GPIO
	//2、
	BLT_USART_Config();
	
	return HC05_Send_CMD("AT\r\n",1);
}






