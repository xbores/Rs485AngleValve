/***********************************************************************
ÎÄ¼þÃû³Æ£ºbsp.h
***********************************************************************/

#ifndef _BSP_H_
#define _BSP_H_
#include "stm32f0xx.h"
#include<stdio.h>

#include "uart1.h"
#include "gpio.h"
#include "rcc.h"
#include "wdg.h"
#include "tim.h"
#include "adc.h"
#include "i2c.h"


#define  MotorRun_PORT      					GPIOA

#define  MotorRunIn_IO        				GPIO_Pin_7
#define  MotorRunZ_IO        					GPIO_Pin_6
#define  MotorRunF_IO        					GPIO_Pin_5

#define Printf_Enable									0


#endif 
