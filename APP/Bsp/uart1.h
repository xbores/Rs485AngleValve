/***********************************************************************
ÎÄ¼þÃû³Æ£ºuart1.h
***********************************************************************/

#ifndef _UART1_H_
#define _UART1_H_
#include "stm32f0xx.h"
#include "main.h"

void Uart_Configuration(uint32_t baud);   
void TIM3_Init(uint32_t baud);
void Uart_Send_Pkg(uint8_t *buffer,uint8_t len);



extern RS485_Rx_t RS485_Rx;


#endif 
