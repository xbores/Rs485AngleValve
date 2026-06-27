/***********************************************************************
ÎÄ¼þÃû³Æ£ºwdg.h
***********************************************************************/

#ifndef _WDG_H_
#define _WDG_H_
#include "stm32f0xx.h"
#include<stdio.h>

void IWDG_Config(uint8_t prv ,uint16_t rlv);
void IWDG_Feed(void);

#endif 
