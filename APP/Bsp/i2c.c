/***********************************************************************
文件名称：i2c_eeprom.c
说明：提供3个读函数和3个写函数
			可以对8位 16位 32位进行读写操作
			Delay函数的延时，测试1000可以正常读写，120us左右，官方要求4ms?
***********************************************************************/
#include "bsp.h"
#include "string.h"


__IO uint8_t i2c_buffer[256];

void I2C_Configuration(void)    
{
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = SCL_IO;															//CLK
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(SCL_PORT, &GPIO_InitStructure);
	GPIO_SetBits(SCL_PORT, SCL_IO);
	
	GPIO_InitStructure.GPIO_Pin = SDA_IO;															//DIO
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(SDA_PORT, &GPIO_InitStructure);
	GPIO_SetBits(SDA_PORT, SDA_IO);
}


//I2C延时函数
void I2C_Delay(void)
{
	uint16_t tmp = 200;
	while(tmp--);
}

void SDA_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = SDA_IO;															//DIO
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(SDA_PORT, &GPIO_InitStructure);
}

void SDA_OUT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = SDA_IO;															//DIO
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(SDA_PORT, &GPIO_InitStructure);
	GPIO_SetBits(SDA_PORT, SDA_IO);
}

//产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT();     //sda线输出
	SDA_H;	  	  
	SCL_H;
	I2C_Delay();
 	SDA_L;//START:when CLK is high,DATA change form high to low 
	I2C_Delay();
	SCL_L;//钳住I2C总线，准备发送或接收数据 
}	  
//产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT();//sda线输出
	SCL_L;
	SDA_L;//STOP:when CLK is high DATA change form low to high
 	I2C_Delay();
	SCL_H; 
	SDA_H;//发送I2C总线结束信号
	I2C_Delay();							   	
}

//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
uint8_t IIC_Wait_Ack(void)
{
	uint8_t ucErrTime=0;
	SDA_IN();      //SDA设置为输入  
	SDA_H;
	I2C_Delay();	   
	SCL_H;
	I2C_Delay();	 
	while(SDA_Status)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	SCL_L;//时钟输出0 	   
	return 0;  
} 

//产生ACK应答
void IIC_Ack(void)
{
	SCL_L;
	SDA_OUT();
	SDA_L;
	I2C_Delay();
	SCL_H;
	I2C_Delay();
	SCL_L;
}

//不产生ACK应答		    
void IIC_NAck(void)
{
	SCL_L;
	SDA_OUT();
	SDA_H;
	I2C_Delay();
	SCL_H;
	I2C_Delay();
	SCL_L;
}				

//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void IIC_Send_Byte(uint8_t txd)
{                        
	uint8_t t;   
	SDA_OUT(); 	    
	SCL_L;//拉低时钟开始数据传输
	for(t=0;t<8;t++)			//IIC_SDA=(txd&0x80)>>7;
	{              
		if((txd&0x80)>>7)   SDA_H;
		else 								SDA_L;
		txd<<=1; 	  
		I2C_Delay();   //对TEA5767这三个延时都是必须的
		SCL_H;
		I2C_Delay(); 
		SCL_L;	
		I2C_Delay();
	}	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
uint8_t IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN();//SDA设置为输入
	for(i=0;i<8;i++ )
	{
		SCL_L; 
		I2C_Delay();
		SCL_H;
		receive<<=1;
		if(SDA_Status)receive++;   
		I2C_Delay(); 
	}					 
	if (!ack) IIC_NAck();//发送nACK
	else      IIC_Ack(); //发送ACK   
	return receive;
}


//在AT24CXX指定地址读出一个数据
//ReadAddr:开始读数的地址
//返回值  :读到的数据
uint8_t AT24CXX_ReadOneByte(uint16_t ReadAddr)
{
	uint8_t temp = 0;
	IIC_Start();
	if (EE_TYPE > AT24C16)
	{
		IIC_Send_Byte(0XA0);       //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr >> 8); //发送高地址
	}
	else IIC_Send_Byte(0XA0 + ((ReadAddr / 256) << 1)); //发送器件地址0XA0,写数据

	IIC_Wait_Ack();
	IIC_Send_Byte(ReadAddr % 256); //发送低地址
	IIC_Wait_Ack();
	IIC_Start();
	IIC_Send_Byte(0XA1);           //进入接收模式
	IIC_Wait_Ack();
	temp = IIC_Read_Byte(0);
	IIC_Stop();//产生一个停止条件
	return temp;
}
//在AT24CXX指定地址写入一个数据
//WriteAddr  :写入数据的目的地址
//DataToWrite:要写入的数据
void AT24CXX_WriteOneByte(uint16_t WriteAddr, uint8_t DataToWrite)
{
	IIC_Start();
	if (EE_TYPE > AT24C16)
	{
		IIC_Send_Byte(0XA0);        //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr >> 8); //发送高地址
	}
	else
	{
		IIC_Send_Byte(0XA0 + ((WriteAddr / 256) << 1)); //发送器件地址0XA0,写数据
	}
	IIC_Wait_Ack();
	IIC_Send_Byte(WriteAddr % 256); //发送低地址
	IIC_Wait_Ack();
	IIC_Send_Byte(DataToWrite);     //发送字节
	IIC_Wait_Ack();
	IIC_Stop();//产生一个停止条件
	I2C_Delay();
}



//在AT24CXX里面的指定地址开始读出指定个数的数据
//ReadAddr :开始读出的地址 对24c02为0~255
//pBuffer  :数据数组首地址
//NumToRead:要读出数据的个数
void AT24CXX_Read(uint16_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead)
{
	while (NumToRead)
	{
		*pBuffer++ = AT24CXX_ReadOneByte(ReadAddr++);
		Delay1ms(3);
		NumToRead--;
	}
}
//在AT24CXX里面的指定地址开始写入指定个数的数据
//WriteAddr :开始写入的地址 对24c02为0~255
//pBuffer   :数据数组首地址
//NumToWrite:要写入数据的个数
void AT24CXX_Write(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite)
{
	while (NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr, *pBuffer);
		Delay1ms(3);
		WriteAddr++;
		pBuffer++;
	}
}



