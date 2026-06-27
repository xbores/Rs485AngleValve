#include "bsp.h"


//配置定时器 Tim2  10ms*50000 = 500s
void TIM2_Init(void)
{
	TIM_DeInit(TIM2);
	TIM_InternalClockConfig(TIM2);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	TIM_TimeBaseStructure.TIM_Period = 50000;
	TIM_TimeBaseStructure.TIM_Prescaler = 47999;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 				//向上计数
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);                            		//清除标志位，以免一启用中断后立即产生中断
	TIM_ARRPreloadConfig(TIM2, DISABLE);                             		//禁止ARR预装载缓冲器  
	TIM_Cmd(TIM2, ENABLE);    	
}



//配置定时器 Tim14 1us一个心跳
void TIM14_Init(void) 
{
	
	TIM_DeInit(TIM14);
	TIM_InternalClockConfig(TIM14);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14,ENABLE);
	TIM_TimeBaseStructure.TIM_Period = 50000;
	TIM_TimeBaseStructure.TIM_Prescaler = 47999;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 				//向上计数
	TIM_TimeBaseInit(TIM14,&TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM14, DISABLE);                             		//禁止ARR预装载缓冲器
	TIM_Cmd(TIM14, ENABLE);
}



//SysTick 1ms 系统时基: 提供非阻塞延时所需的毫秒计数(优先级最低, 不抢占USART/TIM3/EXTI)
__IO uint32_t SysTickMs = 0;

void SysTick_Init(void)
{
	SysTick_Config(48000000 / 1000);				//48MHz, 1ms 一次中断
}

void SysTick_Handler(void)
{
	SysTickMs++;
}

uint32_t Millis(void)
{
	return SysTickMs;
}


