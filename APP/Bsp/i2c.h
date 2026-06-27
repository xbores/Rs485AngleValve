#ifndef _I2C_H_
#define _I2C_H_
#include "stm32f0xx.h"

#define I2C_ACK				1
#define I2C_NoACK			0




#define AT24C01		127
#define AT24C02		255
#define AT24C04		511
#define AT24C08		1023
#define AT24C16		2047
#define AT24C32		4095
#define AT24C64	    8191
#define AT24C128	16383
#define AT24C256	32767

//DS1307模块上所使用的是24c32，所以定义EE_TYPE为AT24C32
#define EE_TYPE AT24C02	//需要根据模块具体型号选择
		

#define WR									0x0
#define RD									0x1

#define	temperature					1
#define	pressure						2


#define	Normal							0x21B1
#define	Lowpower						0x21ac


#define  SCL_PORT      							GPIOA
#define  SCL_IO        							GPIO_Pin_9

#define  SDA_PORT      							GPIOA
#define  SDA_IO        							GPIO_Pin_10


#define SCL_H	  			GPIO_SetBits(SCL_PORT, SCL_IO) 
#define SCL_L	  			GPIO_ResetBits(SCL_PORT, SCL_IO) 


#define SDA_H	  			GPIO_SetBits(SDA_PORT, SDA_IO) 
#define SDA_L	  			GPIO_ResetBits(SDA_PORT, SDA_IO) 
#define	SDA_Status		GPIO_ReadInputDataBit(SDA_PORT , SDA_IO)

//#define IIC_SDA_ON		SDA_H
//#define IIC_SDA_OFF		SDA_L

//#define IIC_SCL_OFF		SCL_H
//#define IIC_SCL_ON		SCL_L

//#define READ_SDA 			SDA_Status

void I2C_Configuration(void);

uint8_t I2C_GetACK(void);
void I2C_PutACK(uint8_t ack);

void I2C_Delay(void);

uint8_t I2C_WriteByte(uint8_t data);
uint8_t I2C_ReadByte(uint8_t ack);

void I2C_Start(void);
void I2C_End(void);

uint8_t AT24CXX_ReadOneByte(uint16_t ReadAddr);
void AT24CXX_WriteOneByte(uint16_t WriteAddr, uint8_t DataToWrite);

void AT24CXX_Read(uint16_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead);
void AT24CXX_Write(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite);


extern __IO uint8_t i2c_buffer[];

#endif 


