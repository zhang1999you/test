#ifndef __BSP_IIC_H
#define	__BSP_IIC_H

#include "stm32f10x.h"
#define  RCC_HARD_GPIO_PORT      RCC_APB2Periph_GPIOB
#define  RCC_HARD_I2C_PORT       RCC_APB1Periph_I2C1
#define  I2C_SCL_PIN             GPIO_Pin_6
#define  I2C_SDA_PIN             GPIO_Pin_7
#define  I2C_GPIO_PORT           GPIOB
#define  MPU_I2C    I2C1
#define I2Cx_OWN_ADDRESS7      0X0A 


void MPU_I2C_Config(void);
void I2C_ByteWrite(uint8_t pBuffer, uint8_t WriteAddr);                                 
void I2C_BufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead);
#endif /* __BSP_IIC_H */
