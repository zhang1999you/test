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
    .Ki = 0,
    .Kd = 5,
    
    .Error0 = 0,
    .Error1 = 0,
    .ErrorInt = 0,
    
    .OutMax = 80,
    .OutMin = -60
};


PID_t SpeedPID={
    .Target = 0,
    .Actual = 0,
    .Out = 0,
    
    .Kp = 2,
    .Ki = 0.1,
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
        if ((p_cmd = strstr(RxBuffer, "SpeedPID.kp:")) != NULL)      { if(sscanf(p_cmd, "SpeedPID.kp:%f", &temp_val) == 1) {SpeedPID.Kp = temp_val; match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "SpeedPID.ki:")) != NULL) { if(sscanf(p_cmd, "SpeedPID.ki:%f", &temp_val) == 1) {SpeedPID.Ki = temp_val; match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "SpeedPID.kd:")) != NULL) { if(sscanf(p_cmd, "SpeedPID.kd:%f", &temp_val) == 1) {SpeedPID.Kd = temp_val; match_success = true;} }
        
        else if ((p_cmd = strstr(RxBuffer, "AnglePID.kp:")) != NULL) { if(sscanf(p_cmd, "AnglePID.kp:%f", &temp_val) == 1) {AnglePID.Kp = temp_val; match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "AnglePID.ki:")) != NULL) { if(sscanf(p_cmd, "AnglePID.ki:%f", &temp_val) == 1) {AnglePID.Ki = temp_val; match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "AnglePID.kd:")) != NULL) { if(sscanf(p_cmd, "AnglePID.kd:%f", &temp_val) == 1) {AnglePID.Kd = temp_val; match_success = true;} }
        
        else if ((p_cmd = strstr(RxBuffer, "runFlag:")) != NULL)       { if(sscanf(p_cmd, "runFlag:%d", &temp_int) == 1)     {runFlag = (temp_int != 0); match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "accOff:")) != NULL) { if(sscanf(p_cmd, "accOff:%f", &temp_val) == 1) {angleAccOffset = temp_val; match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "angleGyroReset:")) != NULL) { if(sscanf(p_cmd, "angleGyroReset:%f", &temp_val) == 1) {angleGyro = 0;  match_success = true;} }
        
        else if ((p_cmd = strstr(RxBuffer, "s:")) != NULL) { 
            
            if (sscanf(p_cmd, "s:%f,t:%d", &temp_val, &temp_int) == 2) {

                SpeedPID.Target=temp_val/30.0f;
                PWMDif=temp_int/4;
                match_success = true;
            } 
        }
        // 如果连 strstr 都找不到任何关键字，才说明这包数据彻底废了
        if (!match_success) {
            // printf("ERR: Command Unrecognized! Received: [%s]\r\n", RxBuffer);
        }

        // ? 每次处理完后，不管成功还是失败，把接收缓冲区彻底清零，并复位计数器
        memset(RxBuffer, 0, sizeof(RxBuffer)); 
        RxCounter = 0;
        RxFlag = 0;
        
        // if(match_success) {
        //     printf("ACK: Parameter Updated\r\n"); 
        // }
    }
}

void I2C1_BusRecover(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    uint8_t i;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /*
     * PB6=SCL、PB7=SDA
     * 临时配置成开漏输出
     */
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 先释放SDA和SCL，由上拉电阻拉高 */
    GPIO_SetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);
    Delay(1);

    /* 最多产生9个SCL脉冲，让从机退出未完成的传输 */
    for (i = 0; i < 9; i++)
    {
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) == Bit_SET)
        {
            break;
        }

        GPIO_ResetBits(GPIOB, GPIO_Pin_6);
        Delay(1);

        GPIO_SetBits(GPIOB, GPIO_Pin_6);
        Delay(1);
    }

    /* 手动产生STOP：SCL为高时，让SDA从低变高 */
    GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    Delay(1);

    GPIO_SetBits(GPIOB, GPIO_Pin_6);
    Delay(1);

    GPIO_SetBits(GPIOB, GPIO_Pin_7);
    Delay(1);
}


void debugTest1()
{
    while(1)
    {
        printf("test\r\n");	
    }
}
void debugTest2()
{
    while(1)
    {
        if(MPU6050ReadID() == 0)
        {
            printf("没有检测到MPU6050传感器\r\n");	
        }
        else
        {
            printf("检测到MPU6050传感器\r\n");
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

void Debug_PA9_PA10_Test(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启GPIOA时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA9、PA10配置为普通推挽输出 */
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    while (1)
    {
        /* 第一阶段：PA9高，PA10低 */
        GPIO_SetBits(GPIOA, GPIO_Pin_9);
        GPIO_ResetBits(GPIOA, GPIO_Pin_10);
        Delay(500);

        /* 第二阶段：PA9低，PA10高 */
        GPIO_ResetBits(GPIOA, GPIO_Pin_9);
        GPIO_SetBits(GPIOA, GPIO_Pin_10);
        Delay(500);
    }
}
int main(void)
{	
	/* 1. 系统复位以及启动 HSE/PLL 等 */
	SystemInit();                 // CMSIS: 复位并配置系统时钟源到默认状态
	SystemClock_Config();         // SPL: 您自己写的 72MHz 时钟配置函数
	SysTick_Config(SystemCoreClock/1000);
    DEBUG_USART_Config();
    // Debug_PA9_PA10_Test();

	Interrupt_Priority_Config();//中断优先级设置
    I2C1_BusRecover();
	MPU_I2C_Config();
	MPU6050_Init();
    // while (1)
    // {
    //     printf("goRunning\r\n");
    //     Delay(100);
    // }
	Timer_Init();
	Motor_Init();
	Encoder_Init();
    Debug_GPIO_Init(); 
    TIM1_Control_Init();//TIM1 用于姿态解算和直立环控制的定时器，10ms 更新一次
    // while(1)
    // {
    //     printf("goRunning\r\n"); 
    //     TIM_SetCompare3(TIM4, 50);
    //     TIM_SetCompare4(TIM4, 50);
    // }
    bool runFlagLast = false;
	while (1)
	{
        runFlagLast=runFlag;
        // Parse_PID_Commands();
		if(swTimers[1].flag)//打印 当前状态
		{
			swTimers[1].flag = 0;
            bool debugStd=0;
            if(debugStd)
            {
                // printf("\r\nAnglePID.Kp=%.2f, Ki=%.2f, Kd=%.2f | SpeedPID.Kp=%.2f, Ki=%.2f, Kd=%.2f\r\n", 
                //     AnglePID.Kp, AnglePID.Ki, AnglePID.Kd, 
                //     SpeedPID.Kp, SpeedPID.Ki, SpeedPID.Kd);
                // printf("SPEEDL = %.2f, SPEEDR = %.2f\r\n", SPEEDL, SPEEDR);
                
                // printf(angleAccOffset == 0 ? "Angle Acc Offset: %d (Default)\r\n" : "Angle Acc Offset: %d\r\n", angleAccOffset);
                // printf("6");
                
                // printf("Angle.Kp=%.2f Angle.Ki=%.2f Angle.Kd=%.2f\r\n", AnglePID.Kp, AnglePID.Ki, AnglePID.Kd);
                // printf("Speed.Kp=%.2f Speed.Ki=%.2f Speed.Kd=%.2f\r\n", SpeedPID.Kp, SpeedPID.Ki, SpeedPID.Kd);
                // printf("Plot: %d %d \r\n",PWML,PWMR);
                
                
                printf("gxDebug: %d %d %d \r\n", (int)gx, (int)gy, (int)gz);
                printf("xDebug: %d %d %d \r\n", (int)ax, (int)ay, (int)az);
                printf("Plot: %f %f %f \r\n",angleAcc,angleGyro,angle);
            }
            else
            {
                printf("Actual = %.2f, Target = %.2f, Out = %.2f\r\n", SpeedPID.Actual, SpeedPID.Target, SpeedPID.Out);
                printf("Plot: %f %f %f \r\n",angleAcc,angleGyro,angle);

            }
		}
        
        if(runFlag && !runFlagLast)//突然站立的过程
        {
            PID_Init(&AnglePID);
            printf("goRunning\r\n"); 
            if(angle>20)
            {
                Motor_SetPWM(1, -60);
                Motor_SetPWM(2, -60);
            }
            else
            {
                Motor_SetPWM(1, 80);
                Motor_SetPWM(2, 80);
            }
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
