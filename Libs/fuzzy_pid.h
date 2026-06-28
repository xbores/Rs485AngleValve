/***********************************************************************
文件名称：fuzzy_pid.h
功    能：模糊自整定 PID(增量式)
说    明：以 误差E、误差变化EC 为输入, 经 7x7 模糊规则在线整定 Kp/Ki/Kd,
          再做增量式 PID 计算, 输出控制增量 Δu。
          论域 [-3,3] 对应语言值 NB NM NS ZO PS PM PB。
***********************************************************************/
#ifndef _FUZZY_PID_H_
#define _FUZZY_PID_H_

#include <stdint.h>

typedef struct {
    float Kp0, Ki0, Kd0;        // 基础 PID 参数(可用 Modbus 的 P/I/D 实时赋值)
    float Ke,  Kec;             // 量化因子: 把 e、ec 映射到模糊论域[-3,3], 取 Ke=3/|e|max, Kec=3/|ec|max
    float Kup, Kui, Kud;        // 整定输出比例: 模糊输出[-3,3] 乘以它再叠加到基础参数上
    float e1, e2;               // 上次、上上次误差(增量式PID历史)
} FuzzyPID_t;

// 初始化: 设定基础参数与各比例因子, 并清零历史
void  FuzzyPID_Init(FuzzyPID_t *fp,
                    float Kp0, float Ki0, float Kd0,
                    float Ke,  float Kec,
                    float Kup, float Kui, float Kud);

// 清历史误差: 切换目标/模式时调用, 防止增量项突变
void  FuzzyPID_Reset(FuzzyPID_t *fp);

// 计算一步: 返回增量式输出 Δu(调用方再做量纲缩放与限幅)
float FuzzyPID_Calc(FuzzyPID_t *fp, float target, float actual);

#endif
