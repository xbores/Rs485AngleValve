/***********************************************************************
文件名称：uart.h
***********************************************************************/

#ifndef _TIM_H_
#define _TIM_H_
#include "stm32f0xx.h"
#include<stdio.h>

void TIM2_Init(void);
void TIM14_Init(void);

void SysTick_Init(void);					//1ms 系统时基
uint32_t Millis(void);						//开机以来的毫秒数

extern __IO uint32_t SysTickMs;

#endif
