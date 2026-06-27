#ifndef _MAIN_H_
#define _MAIN_H_
#include "stm32f0xx.h"


#define RxLenMax													280
#define DeviceParamBase										0x8003C00
#define Rs485BuffNum											2


typedef struct{
	uint16_t len;
	uint16_t flag;
	uint8_t buffer[RxLenMax];
} RS485_Rx_t;




typedef struct{
	uint32_t flag;
	uint16_t baud;
	uint16_t modbusid;
	uint16_t InitStatus;									//设置初始状态

	uint16_t FlowRateCal;									//流量系数			P/L
	uint16_t MaxFlow;											//最大流量			
	uint16_t MaxLimit;										//最大行程			秒
	uint16_t Kalman;											//实时流量卡尔曼滤波使能
	
	uint16_t Angle0ADC;										//阀门关闭时的ADC
	uint16_t Angle90ADC;									//阀门打开时的ADC
	
	int16_t P;														//P
	int16_t I;														//I
	int16_t D;														//D	
	
	int16_t Adc4ma;												//4ma对应的值	
	int16_t Adc20ma;											//20ma对应的值	
	int16_t Offset;												//偏移量
	uint16_t MaxAmp;											//最大压力
	uint16_t MaxValueSet;									//最大给定值
	uint16_t Polarity;										//调节极性
	uint16_t TargetValueSet;												//给定值目标值

} Config_t;



typedef struct{
	uint16_t ControlType;										//控制方式			0:角度 1:流量
	int32_t TargetAngle;										//目标角度 0~900
	int32_t TargetRatio;										//目标开度 0~1000
	uint32_t TargetFlow;										//目标流量 L/H
	int32_t CurrentAngle;									//当前角度 0~900
	int32_t CurrentRatio;									//当前开度 0~1000
	uint32_t CurrentFlow;										//当前流量 L/H
	uint32_t FlowSum;												//总流量					L
	
	uint8_t TargetStatus;
	uint8_t CurrentStatus;
	
	uint16_t CurrentAmp;											//4~20ma读值
	uint16_t TargetAmp;												//4~20ma目标值
	
	uint16_t CurrentValueSet;											//给定值读值

} Valve_t;



void Write_Config(uint32_t paramaddr, uint32_t addr);
void Param_Init(Config_t *config, uint32_t param);
uint8_t Valve_Dat_Cal(Config_t *config, Valve_t *valve);
void Valve_Init(Config_t *config, Valve_t *valve);
uint16_t Get_RegValue(uint16_t addr);
uint8_t Write_RegValue(RS485_Rx_t *rx, uint16_t regaddr, uint16_t regvalue);
uint8_t Modbus_Process(RS485_Rx_t *rx);
void Delay_Poll(uint32_t ms);


extern __IO uint32_t FlowCnt, FlowCntSum;
extern __IO Config_t Config;
extern __IO Valve_t Valve;

#endif 
