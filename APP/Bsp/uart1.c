#include "bsp.h"
#include "main.h"
#include "string.h"

int fputc(int ch, FILE *f)
{
	#if Printf_Enable
		USART_ClearFlag(USART1,USART_FLAG_TC);
		USART_SendData(USART1, (uint8_t) ch);
		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	#endif
	return ch;
}


RS485_Rx_t RS485_Rx;

void Uart_Configuration(uint32_t baud)    
{
	
	RS485_Rx.len = 0;
	RS485_Rx.flag = 0;
	
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);												
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure; 
	
		
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_1);
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	         
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	         
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure);

	
	USART_DeInit(USART1);
	USART_InitStructure.USART_BaudRate = baud;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(USART1, &USART_InitStructure);

	USART_ReceiveData(USART1);
  USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	
  USART_Cmd(USART1, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);	
	
	TIM3_Init(baud);	
}




/*
接收逻辑：
收到字节：打开定时器中断，清零计时器计数值
定时器溢出，触发中断，说明包接收完毕
*/



void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)  																												
	{ 
		if(RS485_Rx.flag == 0)
		{
			if(RS485_Rx.len >= RxLenMax)		RS485_Rx.len = 0;
			RS485_Rx.buffer[RS485_Rx.len] = USART_ReceiveData(USART1);
			RS485_Rx.len++;
		}
		TIM_SetCounter(TIM3, 0);																							//清零计时器，打开定时器中断
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  
		TIM_ITConfig(TIM3,TIM_IT_Update, ENABLE);  
		USART_ReceiveData(USART1);
	}
}



//发送字符串到Uart1
void Uart_Send_Pkg(uint8_t *buffer,uint8_t len)
{
	uint8_t i = 0;
	__disable_irq();	
	for(;i<len;i++)
	{
		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);	
		USART_ClearFlag(USART1,USART_FLAG_TC);
		USART_SendData(USART1, buffer[i]);
	}
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);						//等待最后一个字节发送完成
	__enable_irq(); 
}



//配置定时器 Tim2
void TIM3_Init(uint32_t baud)
{
	TIM_DeInit(TIM3);
	TIM_InternalClockConfig(TIM3);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	TIM_TimeBaseStructure.TIM_Period = 33333*1200/baud; 
	TIM_TimeBaseStructure.TIM_Prescaler = 47;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM3, DISABLE);                
	TIM_Cmd(TIM3, ENABLE);

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  
	TIM_ITConfig(TIM3,TIM_IT_Update, DISABLE);                   

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);	

}



void TIM3_IRQHandler(void)                                                 
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
  {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		TIM_ITConfig(TIM3,TIM_IT_Update, DISABLE);
		RS485_Rx.flag = 1;																//标记一帧接收完成, 交主循环处理(勿在中断里做Flash/EEPROM写)
	}
}



