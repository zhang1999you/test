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
volatile float speedCommandCmS = 0.0f;
volatile float turnCommandPwm = 0.0f;
volatile bool gyroCalibrationRequest = false;
volatile bool gyroCalibrationBusy = false;
volatile uint8_t gyroCalibrationResult = 0;
volatile float gyroXOffset = 0.0f;
volatile bool runFlag = false;
float angleAccOffset = 0.0f;


PID_t AnglePID={
    .Target = 0,
    .Actual = 0,
    .Out = 0,
    
    .Kp = 5.0f,
    .Ki = 0,
    .Kd = 10.0f,
    
    .Error0 = 0,
    .Error1 = 0,
    .ErrorInt = 0,
    
    .OutMax = 80,
    .OutMin = -80
};


PID_t SpeedPID={
    .Target = 0,
    .Actual = 0,
    .Out = 0,
    
    .Kp = 0.8f,
    .Ki = 0.012f,
    .Kd = 0,
    
    
    .OutMax =  SPEED_DRIVE_PWM_MAX,
    .OutMin = -SPEED_DRIVE_PWM_MAX,
};

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
        
        else if ((p_cmd = strstr(RxBuffer, "runFlag:")) != NULL) {
            if (sscanf(p_cmd, "runFlag:%d", &temp_int) == 1) {
                /* Never energize the motors while gyro calibration is active. */
                runFlag = (temp_int != 0) && !gyroCalibrationBusy;
                match_success = true;
            }
        }
        else if ((p_cmd = strstr(RxBuffer, "accOff:")) != NULL) { if(sscanf(p_cmd, "accOff:%f", &temp_val) == 1) {angleAccOffset = temp_val; match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "angleGyroReset:")) != NULL) { if(sscanf(p_cmd, "angleGyroReset:%f", &temp_val) == 1) {angleGyro = 0;  match_success = true;} }
        else if ((p_cmd = strstr(RxBuffer, "gyroCalibrate:")) != NULL) {
            if (sscanf(p_cmd, "gyroCalibrate:%d", &temp_int) == 1 && temp_int != 0) {
                if (!gyroCalibrationBusy) {
                    /* Calibration must run with both motors stopped and no pending drive command. */
                    runFlag = false;
                    speedCommandCmS = 0.0f;
                    turnCommandPwm = 0.0f;
                    gyroCalibrationResult = 0;
                    gyroCalibrationBusy = true;
                    gyroCalibrationRequest = true;
                    printf("GYRO_CAL:START\r\n");
                } else {
                    printf("GYRO_CAL:BUSY\r\n");
                }
                match_success = true;
            }
        }
        
        else if ((p_cmd = strstr(RxBuffer, "s:")) != NULL) { 
            
            if (sscanf(p_cmd, "s:%f,t:%d", &temp_val, &temp_int) == 2) {

                if (temp_val > BLE_SPEED_FULL_SCALE)
                    temp_val = BLE_SPEED_FULL_SCALE;
                else if (temp_val < -BLE_SPEED_FULL_SCALE)
                    temp_val = -BLE_SPEED_FULL_SCALE;
                // 小程序摇杆范围是 -100 ~ 100，换算为实际速度和差速转向目标。
                speedCommandCmS = temp_val * MAX_COMMAND_SPEED_CM_S / BLE_SPEED_FULL_SCALE;

                if (temp_int > (int)BLE_TURN_FULL_SCALE)
                    temp_int = (int)BLE_TURN_FULL_SCALE;
                else if (temp_int < -(int)BLE_TURN_FULL_SCALE)
                    temp_int = -(int)BLE_TURN_FULL_SCALE;

                turnCommandPwm = temp_int * MAX_TURN_PWM / BLE_TURN_FULL_SCALE;
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
void GitCUrrentAngle()
{
    MPU6050ReadAcc(Accel);

    angleAcc = atan2f((float)Accel[1], (float)Accel[2])
            * 180.0f / 3.1415926f
            + sensorAngleOffset;

    angle = angleAcc;
    angleGyro = angleAcc;
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
	// Timer_Init();
	Motor_Init();
	Encoder_Init();
    Debug_GPIO_Init(); 
    GitCUrrentAngle();
    TIM1_Control_Init();//TIM1 用于姿态解算和直立环控制的定时器，10ms 更新一次
    // while(1)
    // {
    //     printf("goRunning\r\n"); 
    //     TIM_SetCompare3(TIM4, 50);
    //     TIM_SetCompare4(TIM4, 50);
    // }
    bool runFlagLast = false;



    float cmd, ref, vraw, vf, spdOut, spdLim, off;
    float angRef, ang, angOut;
    float accAngle, gyroRate;
    int pwmL, pwmR, turn;
    int emergencyBrake;
    int encDeltaL, encDeltaR;
    float vl, vr;
    int basePwm;
    long encTotalL, encTotalR;
    unsigned int ccrL, ccrR, dirBits;

	while (1)
	{
        
        Parse_PID_Commands();

        if (gyroCalibrationResult != 0)
        {
            uint8_t calibrationResult;
            float calibrationOffset;

            __disable_irq();
            calibrationResult = gyroCalibrationResult;
            calibrationOffset = gyroXOffset;
            gyroCalibrationResult = 0;
            __enable_irq();

            if (calibrationResult == 1)
                printf("GYRO_CAL:DONE,offset:%.2f\r\n", calibrationOffset);
            else
                printf("GYRO_CAL:FAIL,MOVED\r\n");
        }
		if(swTimers[1].flag)//打印 当前状态
		{
			swTimers[1].flag = 0;
            bool debugStd=0;
            if(debugStd)
            {
                __disable_irq();   // 只在复制变量时短暂关闭中断
                vl      = SPEEDL;
                vr      = SPEEDR;
                basePwm = PWMAve;

                cmd    = speedCommandCmS;
                ref    = SpeedPID.Target;
                vraw   = SPEEDAve;
                vf     = SpeedPID.Actual;
                spdOut = SpeedPID.Out;
                spdLim = SpeedPID.OutMax;
                off    = speedAngleOffset;

                angRef = AnglePID.Target;
                ang    = AnglePID.Actual;
                angOut = AnglePID.Out;
                accAngle = angleAcc;
                gyroRate = gx / 32768.0f * 2000.0f;

                pwmL   = PWML;
                pwmR   = PWMR;
                turn   = PWMDif;
                emergencyBrake = speedEmergencyBrake ? 1 : 0;
                encDeltaL = encoderDeltaL;
                encDeltaR = encoderDeltaR;
                encTotalL = (long)encoderTotalL;
                encTotalR = (long)encoderTotalR;
                ccrL = (unsigned int)TIM4->CCR3;
                ccrR = (unsigned int)TIM4->CCR4;
                dirBits = (unsigned int)((GPIOB->ODR >> 12) & 0x0F);

                __enable_irq();    // 必须在 printf 前立刻恢复中断

                printf("cmd=%.1f ref=%.1f raw=%.1f v=%.1f "
                    "spdOut=%.2f lim=%.1f spdPWM=%.2f | "
                    "angRef=%.2f ang=%.2f angOut=%.1f "
                    "pwm=%d,%d turn=%d | "
                    "L=%.1f R=%.1f base=%d | "
                    "acc=%.2f gyro=%.1f cnt=%d,%d pos=%ld,%ld "
                    "ccr=%u,%u dir=%X ebrake=%d\r\n",
                    cmd, ref, vraw, vf,
                    spdOut, spdLim, off,
                    angRef, ang, angOut,
                    pwmL, pwmR, turn,
                    vl, vr, basePwm,
                    accAngle, gyroRate,
                    encDeltaL, encDeltaR, encTotalL, encTotalR,
                    ccrL, ccrR, dirBits, emergencyBrake);
                            }
            else
            {
                printf("Actual = %.2f, Target = %.2f, Out = %.2f\r\n", SpeedPID.Actual, SpeedPID.Target, SpeedPID.Out);
                printf("Plot: %f %f %f \r\n",angleAcc,angleGyro,angle);
                // printf("gx:%d\r\n",gx);

            }
		}
        
        if(runFlag && !runFlagLast)//突然站立的过程
        {
            PID_Init(&AnglePID);
            printf("goRunning\r\n"); 
            // if(angle>20)
            // {
            //     Motor_SetPWM(1, -60);
            //     Motor_SetPWM(2, -60);
            // }
            // else
            // {
            //     Motor_SetPWM(1, 80);
            //     Motor_SetPWM(2, 80);
            // }
        }
        if (fabsf(angle) > 35.0f)
        {
            runFlag = false;
            speedAngleOffset = 0.0f;
        }
        runFlagLast=runFlag;

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
