#include "stm32f10x_it.h"
#include "stm32f10x.h"
#include "main.h"

#include "delay.h"
#include <math.h>


#include "hc05/bsp_hc05.h"
#include "iic/bsp_hard_i2c.h"

#include "mpu6050/mpu6050.h"
#include "oled/OLED.h"
#include "timer/Timer.h"
#include "usart/bsp_usart_blt.h"
#include "usart/bsp_debug_usart.h"








/**
  * @brief  主函数
  * @param  无  
  * @retval 无
  */
	
/* MPU6050数据 */

#define FILTER_ALPHA  0.01f  // 降低这个值，收敛变快；升高这个值，抗震动变强

short Accel[3];
short Gyro[3];
short Temp;
int16_t ax,ay,az;//加速度计的结果，单位g
int16_t gx,gy,gz;//温度计的结果，单位摄氏度

float angleAcc=0;
float angleGyro=0;
float angle=0;
uint32_t test;


int main(void)
{	
	/* 1. 系统复位以及启动 HSE/PLL 等 */
	SystemInit();                 // CMSIS: 复位并配置系统时钟源到默认状态
	SystemClock_Config();         // SPL: 您自己写的 72MHz 时钟配置函数
	SysTick_Config(SystemCoreClock/1000);
	
//	OLED_Init(); 
//	OLED_ShowString(0, 0, "OLED Ready!", OLED_8X16);
//	OLED_Update();
	
	DEBUG_USART_Config();	
//	BLT_USART_Config();
	
	MPU_I2C_Config();
	MPU6050_Init();
	
	
	Timer_Init();
	if(MPU6050ReadID() == 0)
	{
		printf("没有检测到MPU6050传感器\r\n");
			while(1);	
	}
	else
	{
		printf("检测到MPU6050传感器\r\n");
	}
	while (1)
	{
        if (swTimers[0].flag)
		{
			swTimers[0].flag = 0;
			MPU6050ReadAcc(Accel);
			MPU6050ReadGyro(Gyro);
			MPU6050ReadTemp(&Temp);
			ax=Accel[0];
			ay=Accel[1];
			az=Accel[2];
			gx=Gyro[0];
			gy=Gyro[1];
			gz=Gyro[2];
			
			gy+=22;//校准陀螺仪零偏
			angleAcc=-atan2(ax,az)/3.14159*180;
			angleGyro=angle+gy/32768.0*2000*0.001;
			angle=FILTER_ALPHA*angleAcc+(1.0f-FILTER_ALPHA)*angleGyro;	
		}
		if(swTimers[1].flag)
		{
			swTimers[1].flag = 0;
			printf("\r\nPlot： %f %f %f",angleAcc,angleGyro,angle);
			
			// ax=Accel[0]*6.1035e-5f;
			// ay=Accel[1]*6.1035e-5f;
			// az=Accel[2]*6.1035e-5f;
			// gx=Gyro[0]*6.1035e-5f;
			// gy=Gyro[1]*6.1035e-5f;
			// gz=Gyro[2]*6.1035e-5f;
			// temperature=Temp/340+36.53;

			
			
			// printf("\r\nPlot： %d %d %d    ",gx,gy,gz);
//			printf("\r\nPlot： %d    ",ax);
//			printf("\r\nPlot： %d %d %d    ",gx,gy,gz);
//			printf("\r\nPlot： %f    ",angleAcc);
			
			
//			printf("陀螺仪： %f %f %f    ",gx,gy,gz);

			// //陀螺仪计算
			// float yaw_g=yaw+gz*0.005;
			// float pitch_g=pitch+gx*0.005;
			// float roll_g=roll-gy*0.005;
			// //加速度计算
			// float pitch_a=atan2(ay,az)/3.1415927*180.0f;
			// float roll_a=atan2(ax,az)/3.1415927*180.0f;
			// //前两者进行融合
			// yaw=yaw_g;
			// pitch = FILTER_ALPHA * pitch_g + (1.0f - FILTER_ALPHA) * pitch_a;
			// roll  = FILTER_ALPHA * roll_g  + (1.0f - FILTER_ALPHA) * roll_a;
			
			// printf("\r\n加速度： %f %f %f    ",ax,ay,az);
			// printf("陀螺仪： %f %f %f    ",gx,gy,gz);
			// printf("温度： %f°C",temperature);	

			// OLED_Clear(); 
			// OLED_Printf(0, 0, OLED_8X16, "Pitch: %.2f", pitch);
			// OLED_Printf(0, 18, OLED_8X16, "Roll:  %.2f", roll);
			// OLED_Printf(0, 36, OLED_8X16, "Yaw:   %.2f", yaw);
			// OLED_ShowString(0, 50, "Update Run!", OLED_6X8);
			// OLED_Update();
			// printf("\r\n偏航角： %f",yaw);
			// printf("俯仰角： %f",pitch);
			// printf("翻滚角： %f",roll);	
		}

	}
}


void SystemClock_Config(void)
{
    ErrorStatus HSEStartUpStatus;


    RCC_DeInit();


    RCC_HSEConfig(RCC_HSE_ON);


    HSEStartUpStatus = RCC_WaitForHSEStartUp();
    if (HSEStartUpStatus == SUCCESS)
    {

        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        FLASH_SetLatency(FLASH_Latency_2);


        RCC_HCLKConfig(RCC_SYSCLK_Div1);


        RCC_PCLK2Config(RCC_HCLK_Div1);

        RCC_PCLK1Config(RCC_HCLK_Div2);


        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);


        RCC_PLLCmd(ENABLE);


        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);


        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);


        while (RCC_GetSYSCLKSource() != 0x08);  
        
    }
    else
    {

        while (1) {;}
    }
}
/*********************************************END OF FILE**********************/
