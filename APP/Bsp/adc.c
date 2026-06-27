#include "bsp.h"


__IO uint16_t ADCConvertedValue[10][2];	

 

void ADC_Config(void)  

{  

	ADC_InitTypeDef     ADC_InitStructure;
	GPIO_InitTypeDef    GPIO_InitStructure;
	DMA_InitTypeDef			DMA_InitStructure;  

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);            
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);             	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
    
	ADC_DeInit(ADC1);                                              
	ADC_StructInit(&ADC_InitStructure);                            
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;            																	//连续转换模式  
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;         																	//采样数据右对齐  
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;			 
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;         																	//12位分辨率  
	ADC_InitStructure.ADC_ScanDirection = ADC_ScanDirection_Upward;																	//向上扫描0-18通道  
	ADC_Init(ADC1, &ADC_InitStructure);  
	ADC_OverrunModeCmd(ADC1, ENABLE);                               																//使能数据覆盖模式  
	ADC_ChannelConfig(ADC1, ADC_Channel_0 | ADC_Channel_1, ADC_SampleTime_13_5Cycles);              //配置采样通道，采样时间125nS  
	ADC_GetCalibrationFactor(ADC1);                                 																//使能前校准ADC  
	ADC_Cmd(ADC1, ENABLE);                                          																//使能ADC1  
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_ADEN) == RESET);         																//等待ADC1使能完成  
	ADC_DMACmd(ADC1, ENABLE);                                       																//使能ADC_DMA  
	ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_Circular);           																//配置DMA请求模式为循环模式  
	ADC_StartOfConversion(ADC1);                                    																//开启一次转换（必须）  
	
	DMA_DeInit(DMA1_Channel1);                                      																//复位DMA1_channel1  
	DMA_StructInit(&DMA_InitStructure);                            																	//初始化DMA结构体  
	DMA_InitStructure.DMA_BufferSize = 20;            																							//DMA缓存数组大小设置  
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;             																	//DMA方向：外设作为数据源  
	DMA_InitStructure.DMA_M2M = DISABLE;                           																	//内存到内存禁用  
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADCConvertedValue;															//缓存数据数组起始地址  
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;															//数据大小设置为Halfword  
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;        																	//内存地址递增  
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;                																	//DMA循环模式，即完成后重新开始覆盖  
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(ADC1->DR);															//取值的外设地址  
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;											//外设取值大小设置为Halfword  
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;																//外设地址递增禁用  
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;             																//DMA优先级设置为高  
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);  
	DMA_Cmd(DMA1_Channel1, ENABLE);                                 																//使能DMA1  

}  

  


uint16_t ADC_Get(uint8_t channel)
{
	uint32_t MaxValue = 0, MinValue = 0xFFFF, SumValue = 0;
	uint8_t i = 0;
	for(;i<10;i++)
	{
		MaxValue = (MaxValue>ADCConvertedValue[i][channel]) ? MaxValue : ADCConvertedValue[i][channel];
		MinValue = (MinValue<ADCConvertedValue[i][channel]) ? MinValue : ADCConvertedValue[i][channel];
		SumValue += ADCConvertedValue[i][channel];
	}
	SumValue = (SumValue-MaxValue-MinValue)/8;
	return SumValue;
}














