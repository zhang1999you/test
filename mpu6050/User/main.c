#include "stm32f10x_it.h"
#include "stm32f10x.h"
#include "main.h"
#include "bsp_debug_usart.h"
#include "iic/bsp_hard_i2c.h"
#include "mpu6050/mpu6050.h"
#include "delay.h"
#include "math.h"
/**
  * @brief  主函数
  * @param  无  
  * @retval 无
  */
	
/* MPU6050数据 */
short Accel[3];
short Gyro[3];
short Temp;
static float ax,ay,az;//加速度计的结果，单位g
static float gx,gy,gz;//温度计的结果，单位摄氏度
static float temperature;//单位°/s
static float yaw,pitch,roll;//欧拉角单位°
int main(void)
{	
	/* 1. 系统复位以及启动 HSE/PLL 等 */
	SystemInit();                 // CMSIS: 复位并配置系统时钟源到默认状态
	SystemClock_Config();         // SPL: 您自己写的 72MHz 时钟配置函数
	SysTick_Config(SystemCoreClock/1000);
	DEBUG_USART_Config();	
	printf("串口正常\r\n");
	MPU_I2C_Config();
	MPU6050_Init();
	printf("debug1\r\n");

	if(MPU6050ReadID() == 0)
	{
	printf("没有检测到MPU6050传感器\r\n");
			while(1);	
	}
	else
	{
		printf("检测到MPU6050传感器\r\n");
	}
	printf("debug2\r\n");
	while (1)
	{
        if (swTimers[0].flag)
		{
			swTimers[0].flag = 0;
			MPU6050ReadAcc(Accel);
			MPU6050ReadGyro(Gyro);
			MPU6050ReadTemp(&Temp);
		}
		if(swTimers[1].flag)
		{
			swTimers[1].flag = 0;
			ax=Accel[0]*6.1035e-5f;
			ay=Accel[1]*6.1035e-5f;
			az=Accel[2]*6.1035e-5f;
			temperature=Temp/340+36.53;
			gx=Gyro[0]*6.1035e-2f;
			gy=Gyro[1]*6.1035e-2f;
			gz=Gyro[2]*6.1035e-2f;
			//陀螺仪计算
			float yaw_g=yaw+gz*0.005;
			float pitch_g=pitch+gx*0.005;
			float roll_g=roll-gy*0.005;
			//加速度计算
			float pitch_a=atan2(ay,az)/3.1415927*180.0f;
			float roll_a=atan2(ax,az)/3.1415927*180.0f;
			//前两者进行融合
			yaw=yaw_g;
			pitch=0.95238*pitch_g+(1-0.95238)*pitch_a;
			roll=0.95238*roll_g+(1-0.95238)*roll_a;
			
//			printf("\r\n加速度： %f %f %f    ",ax,ay,az);
//			printf("陀螺仪： %f %f %f    ",gx,gy,gz);
//			printf("温度： %f°C",temperature);	
			printf("\r\n偏航角： %f",yaw);
			printf("俯仰角： %f",pitch);
			printf("翻滚角： %f",roll);	
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
