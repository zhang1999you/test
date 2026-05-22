#include "stm32f10x_it.h"
#include "stm32f10x.h"
#include "main.h"
#include <stdbool.h>
#include "delay.h"
#include <math.h>
#include <string.h>

#include "hc05/bsp_hc05.h"
#include "iic/bsp_hard_i2c.h"

#include "mpu6050/mpu6050.h"
#include "oled/OLED.h"
#include "timer/Timer.h"
#include "usart/bsp_usart_blt.h"
#include "usart/bsp_debug_usart.h"
#include "motor/Motor.h"
#include "motor/Encoder.h"
#include "pid/PID.h"





/**
  * @brief  主函数
  * @param  无  
  * @retval 无
  */
	
/* MPU6050数据 */



short Accel[3];
short Gyro[3];
short Temp;
int16_t ax,ay,az;//加速度计的结果，单位g
int16_t gx,gy,gz;
float angleAcc=0;
float angleGyro=0;
float angle=0;

volatile int8_t PWML = 0, PWMR = 0;
volatile int8_t PWMAve = 0, PWMDif = 0;

volatile float SPEEDL = 0, SPEEDR = 0;
volatile float SPEEDAve = 0, SPEEDDif = 0;

bool runFlag = false;
float angleAccOffset = 0.0f;


PID_t AnglePID={
    .Target = 0,
    .Actual = 0,
    .Out = 0,
    
    .Kp = 5,
    .Ki = 0.1,
    .Kd = 5,
    
    .Error0 = 0,
    .Error1 = 0,
    .ErrorInt = 0,
    
    .OutMax = 50,
    .OutMin = -50
};


PID_t SpeedPID={
    .Target = 0,
    .Actual = 0,
    .Out = 0,
    
    .Kp = 0,
    .Ki = 0,
    .Kd = 0,
    
    
    .OutMax = 20,
    .OutMin = -20
};
            // printf("\r\nAnglePID.Kp=%.2f, Ki=%.2f, Kd=%.2f | SpeedPID.Kp=%.2f, Ki=%.2f, Kd=%.2f\r\n", 
            //     AnglePID.Kp, AnglePID.Ki, AnglePID.Kd, 
            //     SpeedPID.Kp, SpeedPID.Ki, SpeedPID.Kd);
// void Parse_PID_Commands(void)
// {
//     if (RxFlag == 1)
//     {
//         float temp_val = 0;
//         int temp_int = 0;
//         bool match_success = true;
//         // int16_t temp_i16 = 0; // 如果代码其他地方用不到，这个变量可以删掉了

//         if (sscanf(RxBuffer, "SpeedPID.kp:%f", &temp_val) == 1)      {SpeedPID.Kp = temp_val; printf("SpeedPID.Kp=%.2f\r\n", SpeedPID.Kp);}
//         else if (sscanf(RxBuffer, "SpeedPID.ki:%f", &temp_val) == 1) {SpeedPID.Ki = temp_val; printf("SpeedPID.Ki=%.2f\r\n", SpeedPID.Ki);}
//         else if (sscanf(RxBuffer, "SpeedPID.kd:%f", &temp_val) == 1) {SpeedPID.Kd = temp_val; printf("SpeedPID.Kd=%.2f\r\n", SpeedPID.Kd);}
        
//         else if (sscanf(RxBuffer, "AnglePID.kp:%f", &temp_val) == 1) {AnglePID.Kp = temp_val; printf("AnglePID.Kp=%.2f\r\n", AnglePID.Kp);}
//         else if (sscanf(RxBuffer, "AnglePID.ki:%f", &temp_val) == 1) {AnglePID.Ki = temp_val; printf("AnglePID.Ki=%.2f\r\n", AnglePID.Ki);}
//         else if (sscanf(RxBuffer, "AnglePID.kd:%f", &temp_val) == 1) {AnglePID.Kd = temp_val; printf("AnglePID.Kd=%.2f\r\n", AnglePID.Kd);}
        
//         else if (sscanf(RxBuffer, "runFlag:%d", &temp_int) == 1)       runFlag = (temp_int != 0);
        
//         /* ====== 修改这里 ====== */
//         // 将 %hd 改为 %f，将接收变量改为 &temp_val
//         else if (sscanf(RxBuffer, "angleAccOffset:%f", &temp_val) == 1)    {angleAccOffset = temp_val; printf("angleAccOffset=%.2f\r\n", angleAccOffset);}

//         // else if (sscanf(RxBuffer, "angleGyroReset:%f", &temp_val) == 1) {angleGyro = 0; printf("\r\nangleGyroReset=%.2f", angleGyro);}
//         else {
//             match_success = false;
//             printf("ERR: Command Unrecognized! Received: [%s]\r\n", RxBuffer);
//         }
//         RxCounter = 0;
//         RxFlag = 0;
        
//         if(match_success) printf("ACK: Parameter Updated\r\n"); 

//     }
// }
void Parse_PID_Commands(void)
{
    if (RxFlag == 1)
    {
        float temp_val = 0;
        int temp_int = 0;
        bool match_success = false; 
        char *p_cmd = NULL; // 用于指向真正有效命令起始位置的指针

        // ? 核心改动：不再死板地从头匹配，而是在整个缓冲区里“搜寻”关键字（解决黏连问题）
        if ((p_cmd = strstr(RxBuffer, "SpeedPID.kp:")) != NULL)      { if(sscanf(p_cmd, "SpeedPID.kp:%f", &temp_val) == 1) {SpeedPID.Kp = temp_val; printf("SpeedPID.Kp=%.2f\r\n", SpeedPID.Kp); match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "SpeedPID.ki:")) != NULL) { if(sscanf(p_cmd, "SpeedPID.ki:%f", &temp_val) == 1) {SpeedPID.Ki = temp_val; printf("SpeedPID.Ki=%.2f\r\n", SpeedPID.Ki); match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "SpeedPID.kd:")) != NULL) { if(sscanf(p_cmd, "SpeedPID.kd:%f", &temp_val) == 1) {SpeedPID.Kd = temp_val; printf("SpeedPID.Kd=%.2f\r\n", SpeedPID.Kd); match_success = true;} }
        
        else if ((p_cmd = strstr(RxBuffer, "AnglePID.kp:")) != NULL) { if(sscanf(p_cmd, "AnglePID.kp:%f", &temp_val) == 1) {AnglePID.Kp = temp_val; printf("AnglePID.Kp=%.2f\r\n", AnglePID.Kp); match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "AnglePID.ki:")) != NULL) { if(sscanf(p_cmd, "AnglePID.ki:%f", &temp_val) == 1) {AnglePID.Ki = temp_val; printf("AnglePID.Ki=%.2f\r\n", AnglePID.Ki); match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "AnglePID.kd:")) != NULL) { if(sscanf(p_cmd, "AnglePID.kd:%f", &temp_val) == 1) {AnglePID.Kd = temp_val; printf("AnglePID.Kd=%.2f\r\n", AnglePID.Kd); match_success = true;} }
        
        else if ((p_cmd = strstr(RxBuffer, "runFlag:")) != NULL)       { if(sscanf(p_cmd, "runFlag:%d", &temp_int) == 1)     {runFlag = (temp_int != 0); match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "accOff:")) != NULL) { if(sscanf(p_cmd, "accOff:%f", &temp_val) == 1) {angleAccOffset = temp_val; printf("angleAccOffset=%.2f\r\n", angleAccOffset); match_success = true;} }
        
        // 如果连 strstr 都找不到任何关键字，才说明这包数据彻底废了
        if (!match_success) {
            printf("ERR: Command Unrecognized! Received: [%s]\r\n", RxBuffer);
        }

        // ? 每次处理完后，不管成功还是失败，把接收缓冲区彻底清零，并复位计数器
        memset(RxBuffer, 0, sizeof(RxBuffer)); 
        RxCounter = 0;
        RxFlag = 0;
        
        if(match_success) {
            printf("ACK: Parameter Updated\r\n"); 
        }
    }
}
void Debug_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 开启 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // 2. 配置 PB0 为推挽输出 (Output Push-Pull)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 50MHz高速
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 默认拉低
    GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}

int main(void)
{	
	/* 1. 系统复位以及启动 HSE/PLL 等 */
	SystemInit();                 // CMSIS: 复位并配置系统时钟源到默认状态
	SystemClock_Config();         // SPL: 您自己写的 72MHz 时钟配置函数
	SysTick_Config(SystemCoreClock/1000);
	
//	OLED_Init(); 
//	OLED_ShowString(0, 0, "OLED Ready!", OLED_8X16);
//	OLED_Update();
	Interrupt_Priority_Config();//中断优先级设置

	DEBUG_USART_Config();	
	MPU_I2C_Config();
	MPU6050_Init();
	Timer_Init();
	Motor_Init();
	Encoder_Init();
    Debug_GPIO_Init();
    TIM1_Control_Init();//TIM1 用于姿态解算和直立环控制的定时器，10ms 更新一次
	// if(MPU6050ReadID() == 0)
	// {
	// 	printf("没有检测到MPU6050传感器\r\n");
	// 		while(1);	
	// }
	// else
	// {
	// 	printf("检测到MPU6050传感器\r\n");
	// }
    bool runFlagLast = false;
	while (1)
	{
        runFlagLast=runFlag;
        Parse_PID_Commands();
		if(swTimers[1].flag)//打印 
		{
			swTimers[1].flag = 0;
            
            // printf("\r\nAnglePID.Kp=%.2f, Ki=%.2f, Kd=%.2f | SpeedPID.Kp=%.2f, Ki=%.2f, Kd=%.2f\r\n", 
            //     AnglePID.Kp, AnglePID.Ki, AnglePID.Kd, 
            //     SpeedPID.Kp, SpeedPID.Ki, SpeedPID.Kd);
            // printf("SPEEDL = %.2f, SPEEDR = %.2f\r\n", SPEEDL, SPEEDR);
            // printf(angleAccOffset == 0 ? "Angle Acc Offset: %d (Default)\r\n" : "Angle Acc Offset: %d\r\n", angleAccOffset);
            // printf("6");
			printf("Plot: %f %f %f \r\n",angleAcc,angleGyro,angle);
			// printf("Plot: %f %f \r\n",angleAcc,angleGyro);
            // printf("Plot: %d %d %d \r\n", (int)gx, (int)gy, (int)gz);
            // printf("Plot: %d %d %d \r\n", (int)ax, (int)ay, (int)az);
		}
		if(0)// 角度环
		{
            swTimers[2].flag = 0;
            if(!runFlag)
            {
                PID_Init(&AnglePID);
                Motor_SetPWM(1, 0);
                Motor_SetPWM(2, 0);
                continue;
            }
            // if(angle>50 || angle<-50) 
            // {
            //     runFlag=false;
            // }
            else
            {
                AnglePID.Actual = angle;
                PID_Update(&AnglePID);
                PWMAve = (int8_t)AnglePID.Out;
                PWML=PWMAve+PWMDif/2;
                PWMR=PWMAve-PWMDif/2;

                if (PWML > 0) {
                    PWML += 50;
                } else if (PWML < 0) {
                    PWML -= 50;
                }
                if (PWMR > 0) {
                    PWMR += 50;
                } else if (PWMR < 0) {
                    PWMR -= 50;
                }
                if(PWML > 100) PWML = 100;
                if(PWML < -100) PWML = -100;
                if(PWMR > 100) PWMR = 100;
                if(PWMR < -100) PWMR = -100;

                Motor_SetPWM(1, PWML);
                Motor_SetPWM(2, PWMR);
            }

		}
		if(0)//速度环 转向环
		{
            swTimers[3].flag = 0;
            if(!runFlag)
            {
                PID_Init(&SpeedPID);
                Motor_SetPWM(1, 0);
                Motor_SetPWM(2, 0);
                continue;
            }
            SPEEDAve = (SPEEDL + SPEEDR) / 2;
            SPEEDDif = SPEEDL - SPEEDR;
            SpeedPID.Actual = SPEEDAve;
            PID_Update(&SpeedPID);
            AnglePID.Target = SpeedPID.Out;

            // printf("SPEEDL=%d, SPEEDR=%d\r\n", (int)SPEEDL, (int)SPEEDR);
            // printf("PWML=%d, PWMR=%d\r\n", (int)PWML, (int)PWMR);
		}
        if(runFlag && !runFlagLast)//突然站立的过程
        {
            PID_Init(&AnglePID);
            printf("goRunning\r\n"); 
            if(angle>50)
            {
                Motor_SetPWM(1, 100);
                Motor_SetPWM(2, 100);
            }
            else
            {
                Motor_SetPWM(1, -100);
                Motor_SetPWM(2, -100);
            }
            // Delay(2000);
            // Motor_SetPWM(1, 0);
            // Motor_SetPWM(2, 0);
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
