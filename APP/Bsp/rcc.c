#include "bsp.h"

void HSI_Init (void)										//为了使频率加倍，采用 PLL倍频的方法，PLL如果使用HSI，默认是 HSI / 2 = 4Mhz
{
	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
	RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
	RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE_DIV1;
	RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12); 													// 8M/2 * 12 = 48M
	RCC->CR |= RCC_CR_PLLON;
	while ((RCC->CR & RCC_CR_PLLRDY) == 0)	{}
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); 																			// PLL 做系统时钟
	while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL)	{}
}	
	

