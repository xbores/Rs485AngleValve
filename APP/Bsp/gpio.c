#include "bsp.h"


void GPIO_Configuration(void)    
{
	GPIO_InitTypeDef GPIO_InitStructure;	
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MotorRunZ_IO | MotorRunF_IO;	         
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 
	GPIO_Init(MotorRun_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = MotorRunIn_IO;	         
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 
	GPIO_Init(MotorRun_PORT, &GPIO_InitStructure);		

}


void delay1us(uint16_t cnt)
{
	uint16_t i;
	for(i=0;i<cnt;i++)
	{
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
	}
}



void Delay1ms(uint32_t dly)
{
	uint32_t i;
	for(i=0;i<dly;i++)
	{
		delay1us(1000);
		IWDG_Feed();
	}
}



uint8_t Motor_Control(uint8_t status)
{
	switch(status)
	{
		case 0:
			GPIO_ResetBits(MotorRun_PORT, MotorRunZ_IO);
			GPIO_ResetBits(MotorRun_PORT, MotorRunF_IO);
		break;
		case 1:
			GPIO_ResetBits(MotorRun_PORT, MotorRunF_IO);
			GPIO_SetBits(MotorRun_PORT, MotorRunZ_IO);
		break;
		case 2:
			GPIO_ResetBits(MotorRun_PORT, MotorRunZ_IO);
			GPIO_SetBits(MotorRun_PORT, MotorRunF_IO);
		break;
		default:																								//刹车
			GPIO_SetBits(MotorRun_PORT, MotorRunZ_IO);
			GPIO_SetBits(MotorRun_PORT, MotorRunF_IO);
			Delay_Poll(200);																		//刹车保持200ms, 其间持续处理Modbus
			GPIO_ResetBits(MotorRun_PORT, MotorRunZ_IO);
			GPIO_ResetBits(MotorRun_PORT, MotorRunF_IO);
		break;
	}
	return status;
}

uint8_t Is_Motor_Run(void)
{
	return GPIO_ReadInputDataBit(MotorRun_PORT , MotorRunIn_IO)?0:1;
}



void Flow_X0_Configuration(void)    
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;	         
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource4);

  EXTI_InitStructure.EXTI_Line = EXTI_Line4;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_15_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);			
}


//流量传感器脉冲数
void EXTI4_15_IRQHandler(void)
{
	FlowCnt++;
	EXTI_ClearITPendingBit(EXTI_Line4);
}


uint16_t Motor_Run_To_Limit(uint8_t status, uint16_t dly)
{
	uint16_t time;
	Motor_Control(status);														//控制阀门
	TIM_SetCounter(TIM2, 0);													//清零计时器
	while(Is_Motor_Run() == 0)												//等待阀门开始转动
	{
		if(TIM_GetCounter(TIM2) > 500)	{break;}
		IWDG_Feed();
	}	
	TIM_SetCounter(TIM2, 0);													//清零计时器
	while(Is_Motor_Run() != 0)	{IWDG_Feed();}				//等待关闭
	time = TIM_GetCounter(TIM2);											//读取时间
	Motor_Control(3);																	//电机刹车
	Delay1ms(dly);																		//延时50ms
	return time;
}


uint8_t Moto_Angle_Pos(Config_t *config, Valve_t *valve)
{
	uint8_t status = 0;
	int16_t angeldiff = valve->TargetAngle - valve->CurrentAngle;
	if((angeldiff > 0)&&(angeldiff >= 5))
	{
		status = 1;
	}
	else if((angeldiff < 0)&&(angeldiff <= -5))
	{
		status = 2;
	}
	else if(angeldiff != 0)
	{
		valve->CurrentAngle	= valve->TargetAngle;
	}
	return status;
}




void Moto_Flow_Pos(void)
{
	static int32_t ValueErrEC[3] = {0,0,0};
	
	int32_t P_out,I_out,D_out,PID_Result;

	ValueErrEC[0] = ValueErrEC[1];
	ValueErrEC[1] = ValueErrEC[2];
	ValueErrEC[2] = Valve.TargetFlow - Valve.CurrentFlow;

	P_out = (ValueErrEC[2] - ValueErrEC[1])*Config.P;
	I_out = ValueErrEC[2]*Config.I;
	D_out = (ValueErrEC[0]+ValueErrEC[2]-2*ValueErrEC[1])*Config.D;
	PID_Result = (P_out + I_out + D_out)*(int32_t)Config.MaxLimit/(int32_t)(Config.MaxFlow ? Config.MaxFlow : 1);		//防除零

	if(PID_Result > 1000)
	{
		PID_Result = 1000;
	}
	else if(PID_Result < -1000)
	{
		PID_Result = -1000;
	}

	if(PID_Result != 0)
	{
		Motor_Control(PID_Result>0?1:2);
		Delay_Poll(PID_Result>0?PID_Result:(0-PID_Result));					//电机定时运行, 其间持续处理Modbus
		Motor_Control(3);
	}	
}



void Moto_Amp_Pos(void)
{
	static int32_t ValueErrEC[3] = {0,0,0};
	
	int32_t P_out,I_out,D_out,PID_Result;

	ValueErrEC[0] = ValueErrEC[1];
	ValueErrEC[1] = ValueErrEC[2];
	ValueErrEC[2] = Valve.TargetAmp - Valve.CurrentAmp;

	P_out = (ValueErrEC[2] - ValueErrEC[1])*Config.P;
	I_out = ValueErrEC[2]*Config.I;
	D_out = (ValueErrEC[0]+ValueErrEC[2]-2*ValueErrEC[1])*Config.D;
	PID_Result = (P_out + I_out + D_out)*(int32_t)Config.MaxLimit/(int32_t)(Config.MaxAmp ? Config.MaxAmp : 1); 		//防除零

	if(PID_Result > 1000)
	{
		PID_Result = 1000;
	}
	else if(PID_Result < -1000)
	{
		PID_Result = -1000;
	}

	if(PID_Result != 0)
	{
		Motor_Control(PID_Result>0?1:2);
		Delay_Poll(PID_Result>0?PID_Result:(0-PID_Result));					//电机定时运行, 其间持续处理Modbus
		Motor_Control(3);
	}	
}




void Moto_ValueSet_Pos(void)
{
	static int32_t ValueErrEC[3] = {0,0,0};
	
	int32_t P_out,I_out,D_out,PID_Result;

	ValueErrEC[0] = ValueErrEC[1];
	ValueErrEC[1] = ValueErrEC[2];
	
	
	if(Config.Polarity == 0xAA)
	{
		ValueErrEC[2] = Valve.CurrentValueSet - Config.TargetValueSet;
	}
	else
	{
		ValueErrEC[2] = Config.TargetValueSet - Valve.CurrentValueSet;
	}		

	P_out = (ValueErrEC[2] - ValueErrEC[1])*Config.P;
	I_out = ValueErrEC[2]*Config.I;
	D_out = (ValueErrEC[0]+ValueErrEC[2]-2*ValueErrEC[1])*Config.D;
	PID_Result = (P_out + I_out + D_out)*(int32_t)Config.MaxLimit/(int32_t)(Config.MaxValueSet ? Config.MaxValueSet : 1); 		//防除零

	if(PID_Result > 1000)
	{
		PID_Result = 1000;
	}
	else if(PID_Result < -1000)
	{
		PID_Result = -1000;
	}

	if(PID_Result != 0)
	{
		Motor_Control(PID_Result>0?1:2);
		Delay_Poll(PID_Result>0?PID_Result:(0-PID_Result));					//电机定时运行, 其间持续处理Modbus
		Motor_Control(3);
	}	
}


