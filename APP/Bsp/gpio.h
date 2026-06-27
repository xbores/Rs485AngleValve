/***********************************************************************
ÎÄ¼þÃû³Æ£ºgpio.h
***********************************************************************/

#ifndef _GPIO_H_
#define _GPIO_H_
#include "main.h"
#include "stm32f0xx.h"

void GPIO_Configuration(void);
void Delay1ms(uint32_t dly);

uint8_t Motor_Control(uint8_t status);
uint16_t Motor_Run_To_Limit(uint8_t status, uint16_t dly);

uint8_t Is_Motor_Run(void);
void Flow_X0_Configuration(void);


uint8_t Moto_Angle_Pos(Config_t *config, Valve_t *valve);

void Moto_Flow_Pos(void);
void Moto_Amp_Pos(void);
void Moto_ValueSet_Pos(void);

#endif 
