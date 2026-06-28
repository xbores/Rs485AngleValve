/***********************************************************************
文件名称：main.c
功    能：
编写时间：
编 写 人：
注    意：
***********************************************************************/
#include "main.h"
#include "bsp.h"
#include "string.h"
#include "libs.h"
#include<stdio.h>
#include <math.h>


__IO Config_t Config;
__IO Valve_t Valve;


#define baudmapsize			7
const uint32_t baudmap[baudmapsize] = {1200, 2400, 4800, 9600, 19200, 38400, 115200};

__IO uint32_t FlowCnt = 0, FlowCntSum = 0;

__IO uint8_t fmcSaveflag = 0;

float Kalman_Filter(float Z);
void  Kalman_Reset(float z);					//使能滤波时复位状态, 锁定到当前流量

//主程序
//以反转行程位基准, 正传时间换算成反转


//处理一帧已接收完成的 Modbus 报文(由 TIM3 帧间隔超时置 flag)
static void Modbus_Poll(void)
{
	if(RS485_Rx.flag == 1)
	{
		Modbus_Process(&RS485_Rx);
		RS485_Rx.len = 0;
		RS485_Rx.flag = 0;
	}
}

//可被通讯"穿透"的延时: 等待期间持续处理 Modbus 与喂狗, 替代会阻塞通讯的 Delay1ms
//注意: 调用链里禁止再触发本函数(Modbus_Process 不调用电机/本函数, 无重入)
void Delay_Poll(uint32_t ms)
{
	uint32_t t0 = Millis();
	while((Millis() - t0) < ms)
	{
		Modbus_Poll();
		IWDG_Feed();
	}
}

int main()
{
	HSI_Init();																																		//48M
	
	GPIO_Configuration();
	Flow_X0_Configuration();
	I2C_Configuration();

	Param_Init((Config_t *)&Config, DeviceParamBase);
	memset((uint8_t *)&Valve, 0, sizeof(Valve_t));

	Uart_Configuration(baudmap[Config.baud%baudmapsize]);
	TIM2_Init();
	TIM14_Init();
	SysTick_Init();											//1ms 系统时基, 用于主循环非阻塞周期
	ADC_Config();
	#if Printf_Enable	
		printf("\r\nSystem Init");
	#endif
	
	AT24CXX_Read(0,(uint8_t *)&Valve.FlowSum,4);			//从eeprom读取流量
	
//	printf("\r\nsum: %d", Valve.FlowSum);
	
	IWDG_Config(IWDG_Prescaler_64 ,2500);			//4秒看门狗
	
	Delay1ms(500);


	Valve_Init((Config_t *)&Config, (Valve_t *)&Valve);
	uint8_t IsFlow;
	
	while (1)
	{
		Modbus_Poll();																		//一帧接收完成则立即处理(避免在中断里做Flash/EEPROM写)

		IsFlow = Valve_Dat_Cal((Config_t *)&Config, (Valve_t *)&Valve);																//更新阀门数据

		switch(Valve.ControlType)
		{
			case 0:
				Valve.TargetStatus = Moto_Angle_Pos((Config_t *)&Config, (Valve_t *)&Valve);				//计算阀门的目标状态
				if(Valve.TargetStatus != Valve.CurrentStatus)
				{
					if((Valve.TargetStatus + Valve.CurrentStatus) < 3)																//确保不会从正转直接切换反转或者反转直接切换正转			
					{
						Valve.CurrentStatus = Valve.TargetStatus;
						if(Valve.TargetStatus == 0)																												//关闭时必须刹车
						{
							Motor_Control(3);																																//刹车+停止
						}
						else
						{
							Motor_Control(Valve.TargetStatus);
						}
					}			
					else
					{
						Motor_Control(3);																																	//刹车+停止
						Valve.CurrentStatus = 0;
					}	
				}				
			break;
				
			case 1:																																			//流量模式
				if(IsFlow != 0)	
				{
					Moto_Flow_Pos();
				}				
			break;
				
			case 2:																														//压力模式
				Moto_Amp_Pos();
			break;	
			
			case 3:																														//给定模式
				Moto_ValueSet_Pos();
			break;	
			
			default:
			break;
		}

		if(fmcSaveflag != 0)
		{
			Write_Config((uint32_t)&Config, DeviceParamBase);
			fmcSaveflag = 0;
		}
		IWDG_Feed();

		Delay_Poll(100);																	//100ms 控制周期, 其间持续处理 Modbus 保证通讯实时性
	}
}


void Param_Init(Config_t *config, uint32_t param)
{
	if(((Config_t *)param)->flag != 0x5AA5)		
	{
		memset((uint8_t *)config, 0, sizeof(Config_t));
		AT24CXX_Write(0,(uint8_t *)config, 4);	
		
		config->flag = 0x5AA5;
		config->baud = 3;
		config->modbusid = 13;
		config->InitStatus = 2;
		
		config->FlowRateCal = 350;
//		config->MinFlow = 50;
		config->MaxFlow = 800;
		config->MaxAmp = 200;					//0.2mpa
		config->MaxValueSet = 200;					//默认最大给定值
		config->MaxLimit = 20;
		config->P = 10;
		config->I = 50;
		config->D = 12;
		config->Adc20ma = 1000;

		config->FuzzyEn = 0;						//默认经典PID, 现场需要时再写寄存器32启用
		config->FuzzyKecRatio = 4;
		config->FuzzyKupPct = 30;
		config->FuzzyKuiPct = 30;
		config->FuzzyKudPct = 30;

		Write_Config((uint32_t)config, param);
	}
	memcpy((uint8_t *)config, (uint8_t *)param, sizeof(Config_t));				//读取设备配置信息

	//老固件Flash无下列字段(读出为0xFFFF/0), 升级后合法化, 不丢失原有标定/参数
	if(config->FuzzyEn > 1)								config->FuzzyEn = 0;
	if(config->FuzzyKecRatio == 0 || config->FuzzyKecRatio > 50)	config->FuzzyKecRatio = 4;
	if(config->FuzzyKupPct > 200)						config->FuzzyKupPct = 30;
	if(config->FuzzyKuiPct > 200)						config->FuzzyKuiPct = 30;
	if(config->FuzzyKudPct > 200)						config->FuzzyKudPct = 30;
}



void Write_Config(uint32_t paramaddr, uint32_t addr)
{
	FLASH_Unlock();
	FLASH_ErasePage(addr);			
	uint16_t i;
	for(i=0;i<((sizeof(Config_t)+1)/2);i++)
	{
		FLASH_ProgramHalfWord(addr+i*2, *(uint16_t *)(paramaddr+i*2));
	}
	FLASH_Lock();	
}



		
uint8_t Modbus_Process(RS485_Rx_t *rx)
{
	IWDG_Feed();

	if(rx->len < 8)																//帧长不足(本机功能码3/6/0x10最短8字节), 丢弃: 防止 rx->len-2 下溢越界读取与脏字节
	{
		return 2;
	}

	if((rx->buffer[0] != Config.modbusid) && (rx->buffer[0] != 255))
	{
		return 2;
	}

	if(CrcCheck(rx->buffer, rx->len-2) != 0)									//CRC错: 静默丢弃不应答(Modbus规范, 避免在RS485总线上对噪声帧回包冲突)
	{
		return 1;
	}

	uint8_t broadcast = (rx->buffer[0] == 255) ? 1 : 0;							//广播帧一律不应答
	uint16_t regaddr = (rx->buffer[2]<<8) + rx->buffer[3];
	uint16_t regnum = (rx->buffer[4]<<8) + rx->buffer[5];
	uint8_t func = rx->buffer[1];
	
	
	switch(func)
	{
		case 3:
			if((regaddr + regnum) <= 120)
			{
				rx->buffer[2] = regnum*2;
				uint16_t value, i;
				for(i=0;i<regnum;i++)
				{
					value = Get_RegValue(regaddr+i);
					rx->buffer[3+i*2] = (value>>8);
					rx->buffer[4+i*2] = value;
				}
				rx->len = rx->buffer[2] + 5;
				CrcCal(rx->buffer, rx->len-2);
				if(!broadcast)	Uart_Send_Pkg(rx->buffer, rx->len);
			}
			else
			{
				rx->buffer[1] |= 0x80;
				rx->buffer[2] = 2;
				CrcCal(rx->buffer, 3);
				if(!broadcast)	Uart_Send_Pkg(rx->buffer, 5);
				return 1;
			}
		break;

		case 6:
			if((Write_RegValue(rx, regaddr, regnum) == 0) && (!broadcast))				//广播帧只写不应答
			{
				Uart_Send_Pkg(rx->buffer, rx->len);
			}
		break;
		
		case 0x10:
			if((regaddr == 9)&&(regnum == 2))
			{
				Valve.FlowSum = 0;
				AT24CXX_Write(0,(uint8_t *)&(Valve.FlowSum), 4);
			}
		break;
		
		default:
			rx->buffer[1] |= 0x80;
			rx->buffer[2] = 3;
			CrcCal(rx->buffer, 3);
			if(!broadcast)	Uart_Send_Pkg(rx->buffer, 5);
			return 1;
	}
	return 3;
}

uint8_t Write_RegValue(RS485_Rx_t *rx, uint16_t regaddr, uint16_t regvalue)
{
	switch(regaddr)
	{
		case 0:
			Valve.TargetAngle = (regvalue>900)?900:regvalue;
			Valve.ControlType = 0;
		break;
			
		case 1:
			Valve.TargetRatio = (regvalue>1000)?1000:regvalue;
			Valve.TargetAngle = Valve.TargetRatio*900/1000;
			Valve.ControlType = 0;
		break;
		
		case 2:
			if(regvalue && !Config.Kalman)					//0->1 使能滤波: 复位状态并锁定到当前流量, 避免从0重新收敛
			{
				Kalman_Reset(Valve.CurrentFlow);
			}
			Config.Kalman = regvalue?1:0;
			fmcSaveflag = 1;
		break;

		case 5:
			Config.InitStatus = (regvalue>2)?2:regvalue;
			fmcSaveflag = 1;
		break;
		
		case 6:				
			memset((uint8_t *)&Config, 0, sizeof(Config_t));
			Write_Config((uint32_t)&Config, DeviceParamBase);
			NVIC_SystemReset();
		break;		
		
		case 7:
			Valve.TargetFlow = regvalue;
			Valve.ControlType = 1;
		break;

		case 9:
			Valve.FlowSum = 0;
			AT24CXX_Write(0,(uint8_t *)&(Valve.FlowSum), 4);
		break;
		
		case 10:
			Valve.FlowSum = 0;
			AT24CXX_Write(0,(uint8_t *)&(Valve.FlowSum), 4);
		break;
		
		case 11:
			Config.modbusid = regvalue;
			fmcSaveflag = 1;
		break;
		
		case 12:
			Config.baud = regvalue%baudmapsize;
			Uart_Configuration(baudmap[Config.baud%baudmapsize]);
			fmcSaveflag = 1;
		break;
		
		case 13:
			Config.FlowRateCal = regvalue ? regvalue : 1;				//作除数, 禁止为0(否则流量计算除零且累计循环死锁)
			fmcSaveflag = 1;
		break;

		case 14:
			Config.MaxFlow = regvalue ? regvalue : 1;					//作PID归一化除数, 禁止为0
			fmcSaveflag = 1;
		break;
		
		case 15:
			Config.MaxLimit = regvalue;
			fmcSaveflag = 1;
		break;
		
		case 16:
			Config.P = regvalue;
			fmcSaveflag = 1;
		break;
		
		case 17:
			Config.I = regvalue;
			fmcSaveflag = 1;
		break;
		
		case 18:
			Config.D = regvalue;
			fmcSaveflag = 1;
		break;
		
		case 22:									//4ma对应的值
			Config.Adc4ma = regvalue;
			fmcSaveflag = 1;
		break;
		
		case 23:										//20ma对应的值
			Config.Adc20ma = regvalue;
			fmcSaveflag = 1;
		break;
		
		case 24:										//偏移量
			Config.Offset = regvalue;
			fmcSaveflag = 1;
		break;
		
		case 25:													//目标压力值
			Valve.TargetAmp = regvalue;
			Valve.ControlType = 2;			
		break;

		case 27:																	//最大压力值
			Config.MaxAmp = regvalue ? regvalue : 1;					//作PID归一化除数, 禁止为0
			fmcSaveflag = 1;
		break;
		
		case 28:													//目标 给定值
			if(regvalue <= Config.MaxValueSet)
			{
				Config.TargetValueSet = regvalue;
				fmcSaveflag = 1;
			}				
		break;
		
		case 29:													//实际 给定值
			if(regvalue <= Config.MaxValueSet)
			{
				Valve.CurrentValueSet = regvalue;
				Valve.ControlType = 3;			
			}
		break;
		
		case 30:																	//最大给定值
			Config.MaxValueSet = regvalue ? regvalue : 1;				//作PID归一化除数, 禁止为0
			fmcSaveflag = 1;
		break;
		
		case 31:																	//PID调节极性	0;正向	1;反向
			Config.Polarity = regvalue?0xAA:0;
			fmcSaveflag = 1;
		break;

		case 32:																	//模糊PID使能 0:经典 1:模糊
			Config.FuzzyEn = regvalue?1:0;
			fmcSaveflag = 1;
		break;

		case 33:																	//Kec相对Ke的倍数
			Config.FuzzyKecRatio = (regvalue==0||regvalue>50)?4:regvalue;
			fmcSaveflag = 1;
		break;

		case 34:																	//Kp整定强度%
			Config.FuzzyKupPct = (regvalue>200)?30:regvalue;
			fmcSaveflag = 1;
		break;

		case 35:																	//Ki整定强度%
			Config.FuzzyKuiPct = (regvalue>200)?30:regvalue;
			fmcSaveflag = 1;
		break;

		case 36:																	//Kd整定强度%
			Config.FuzzyKudPct = (regvalue>200)?30:regvalue;
			fmcSaveflag = 1;
		break;

		default:
			rx->buffer[1] |= 0x80;
			rx->buffer[2] = 2;
			CrcCal(rx->buffer, 3);
			if(rx->buffer[0] != 255)	Uart_Send_Pkg(rx->buffer, 5);		//广播帧不应答
			return 1;
	}
	return 0;
}





uint16_t Get_RegValue(uint16_t addr)
{
	switch(addr)
	{
		case 0:
			return Valve.CurrentAngle;							//当前角度 RW    0~900
		case 1:
			return Valve.CurrentRatio;							//当前开度百分比 RW    0~1000
		
		case 2:
			return Config.Kalman;										//实时流量卡尔曼滤波使能
		
		case 3:
			return Config.Angle90ADC;								//开启时ADC

		case 4:
			return Config.Angle0ADC;								//关闭时ADC

		case 5:
			return Config.InitStatus;								//阀门初始状态, RW  0:关闭,  1:打开,  2:记忆上次位置			默认2
		
		case 6:
			return Valve.ControlType;								//控制方式 RO	(0: 角度方式, 1: 流量方式, 2: ADC方式, 3: 通讯给定方式)			//设置目标流量或者开度数据会自动更新此寄存器
		
		case 7:
			return Valve.TargetFlow;								//目标瞬时流量 RW   L/H
		case 8:
			return Valve.CurrentFlow;								//实际瞬时流量 RO   L/H
		case 9:
			return (uint16_t)(Valve.FlowSum>>16);		//实际累积流量Sum高位 RO  L
		case 10:
			return (uint16_t)Valve.FlowSum;					//实际累积流量Sum低位 RO  L

		case 11:
			return Config.modbusid;									//ModbusId  RW		1~254													默认13
		case 12:
			return Config.baud;											//波特率 RW    {1200, 2400, 4800, 9600, 19200, 38400, 115200};  默认3

		case 13:
			return Config.FlowRateCal;							//流量系数 RW pulse/L				默认350
		case 14:
			return Config.MaxFlow;									//最大流量 RW L/H          	默认800
		case 15:
			return Config.MaxLimit;									//最大行程 RW 秒 仅流量模式有效  默认20
		case 16:
			return Config.P;												//P RW						默认80
		case 17:
			return Config.I;												//I RW						默认200
		case 18:
			return Config.D;												//D RW						默认0
		
		case 20:
			return ADC_Get(0);											//4~20ma输入 RO						默认0
//			return ADC_Get(0)*22/4095;							//4~20ma输入 RO						默认0
		
		case 21:
			return ADC_Get(1);											//角度传感器 RO						默认0
		
		case 22:
			return Config.Adc4ma;										//4ma对应的值 RW 						默认0
		
		case 23:
			return Config.Adc20ma;									//20ma对应的值 RW 					默认1000  1mpa
		
		case 24:
			return Config.Offset;										//偏移量
		
		case 25:												
			return Valve.TargetAmp;															//目标压力值
		
		case 26:												
			return Valve.CurrentAmp;														//实际压力值
		
		case 27:												
			return Config.MaxAmp;																//最大压力值
		
		case 28:												
			return Config.TargetValueSet;												//目标给定值
		
		case 29:												
			return Valve.CurrentValueSet;												//实际给定值
		
		case 30:												
			return Config.MaxValueSet;													//最大给定值
		
		case 31:																							//PID调节极性	0;正向	1;反向
			return (Config.Polarity == 0xAA)?1:0;

		case 32:
			return Config.FuzzyEn;									//模糊PID使能 0:经典 1:模糊	默认0
		case 33:
			return Config.FuzzyKecRatio;							//Kec相对Ke的倍数			默认4
		case 34:
			return Config.FuzzyKupPct;								//Kp整定强度% (Kup=P*%/100)	默认30
		case 35:
			return Config.FuzzyKuiPct;								//Ki整定强度%				默认30
		case 36:
			return Config.FuzzyKudPct;								//Kd整定强度%				默认30

		default:
			return 0;
	}
}



void Valve_Init(Config_t *config, Valve_t *valve)
{
	if((config->Angle0ADC == 0) || (config->Angle90ADC == 0))
	{
		Motor_Run_To_Limit(2, 50);																				//完全关闭阀门,带50ms刹车
		config->Angle0ADC = ADC_Get(1);
		config->MaxLimit  = Motor_Run_To_Limit(1, 50)/1000;								//获取完全打开阀门的时间, 得到阀门的正向行程
		config->Angle90ADC = ADC_Get(1);
		Write_Config((uint32_t)config, DeviceParamBase);
	}
	
	if(config->InitStatus == 0)																					//上电默认关
	{
		Motor_Run_To_Limit(2, 50);	
	}
	else if(config->InitStatus == 1)																		//上电默认开
	{
		Motor_Run_To_Limit(1, 50);	
	}
	
	Valve_Dat_Cal((Config_t *)&Config, valve);													//计算阀门数据
	
	valve->TargetAngle = valve->CurrentAngle;
	valve->TargetRatio = valve->CurrentRatio;
	
}




/*****************************************
卡尔曼滤波算法
参数解释
P：状态估计误差协方差，反映了估计的不确定性。
P_：预测误差协方差。
X：当前状态的估计值。
X_：预测的状态值。
K：卡尔曼增益，用于平衡预测值和测量值的权重。
Q：过程噪声协方差，代表系统动态变化的随机性。
R：测量噪声协方差，表示测量数据的噪声水平。
Z：传感器的当前测量值。
******************************************/
float P = 1, P_, X = 0, X_, K = 0, Q = 0.01, R = 0.2;  // 卡尔曼滤波参数
float Kalman_Filter(float Z) {
    X_ = X;  // 预测状态：将上一状态作为预测的初始值
    P_ = P + Q;  // 预测误差协方差：加上过程噪声Q

    // 计算卡尔曼增益K：当前预测误差协方差除以测量误差协方差与预测误差协方差的和
    K = P_ / (P_ + R);

    // 更新状态：预测状态加上卡尔曼增益乘以测量值与预测状态的差
    X = X_ + K * (Z - X_);

    // 更新误差协方差：(1 - K)乘以预测误差协方差
    P = (1 - K) * P_;

    return X;  // 返回估计值
}

//复位滤波状态: 把估计值直接置为 z(瞬间锁定当前流量), 误差协方差置稳态值使后续保持平滑
void Kalman_Reset(float z)
{
    X = z;
    P = 0.05f;
}



uint8_t Valve_Dat_Cal(Config_t *config, Valve_t *valve)
{
	uint32_t flowtimetemp;
	uint8_t result = 0, cnt = 0;
	flowtimetemp = TIM_GetCounter(TIM14);																								//计算流量的最小周期为500ms
	if(flowtimetemp >= 500)
	{
		uint32_t flowcnttemp = FlowCnt, flow;
		FlowCnt = 0;
		result = 1;

		TIM_SetCounter(TIM14, 0);

		if(Config.FlowRateCal != 0)														//防除零: 分母为0时整段跳过(否则除零且累计while死循环)
		{
			flow = (uint32_t)((uint64_t)flowcnttemp*1000*3600/((uint64_t)Config.FlowRateCal*flowtimetemp));    	// L/H, 64位中间量防溢出

			if(Config.Kalman != 0)
			{
				valve->CurrentFlow = Kalman_Filter(flow);
			}
			else
			{
				valve->CurrentFlow = flow;
			}

			FlowCntSum += flowcnttemp;																												//计算总流量
			while(FlowCntSum >= config->FlowRateCal)									//FlowRateCal已保证非0, 不会死循环
			{
				FlowCntSum -= config->FlowRateCal;
				valve->FlowSum++;
				cnt |= 1;
			}
			if(cnt)
			{
				AT24CXX_Write(0,(uint8_t *)&(valve->FlowSum), 4);
//				printf("\r\nsum: %d", Valve.FlowSum);
			}
		}
	}

	int32_t angspan = (int32_t)config->Angle90ADC - (int32_t)config->Angle0ADC;		//用有符号避免无符号回绕
	if(angspan != 0)																//防除零: 标定失败(两端ADC相等)时保持上次比例
	{
		int32_t ratio = 1000*((int32_t)ADC_Get(1) - (int32_t)config->Angle0ADC)/angspan;
		if(ratio < 0)				ratio = 0;										//钳位0~1000, 防止角度寄存器越界
		else if(ratio > 1000)		ratio = 1000;
		valve->CurrentRatio = ratio;
	}
	valve->CurrentAngle = valve->CurrentRatio*900/1000;
	
	uint32_t amp = ADC_Get(0)*2200/4095;
	if(amp <= 400)
	{
		valve->CurrentAmp = Config.Adc4ma;
	}
	else if(amp >= 2000)
	{
		valve->CurrentAmp = Config.Adc20ma;
	}
	else
	{
		valve->CurrentAmp = (Config.Adc20ma-Config.Adc4ma)*(amp - 400)/1600 + Config.Adc4ma + Config.Offset;
	}
	
	return result;
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

