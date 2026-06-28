/***********************************************************************
文件名称：fuzzy_pid.c
功    能：模糊自整定 PID(增量式)实现
算法流程：
  1) 误差 e=目标-实际, 误差变化 ec=e-上次e;
  2) 用量化因子 Ke、Kec 把 e、ec 映射到论域[-3,3];
  3) 三角隶属度把论域值分解到相邻两个语言值, E、EC 各取2个 -> 4 条激活规则;
  4) 查 ΔKp/ΔKi/ΔKd 规则表并按隶属度加权平均(重心法简化, Σ权=1);
  5) Kp=Kp0+ΔKp*Kup, Ki=Ki0+ΔKi*Kui, Kd=Kd0+ΔKd*Kud;
  6) 增量式: Δu = Kp*(e-e1) + Ki*e + Kd*(e-2*e1+e2)。
***********************************************************************/
#include "fuzzy_pid.h"

/* 语言值索引: NB=0 NM=1 NS=2 ZO=3 PS=4 PM=5 PB=6, 对应论域值 -3..3 */

/* ΔKp 规则表  [E][EC] */
static const signed char ruleKp[7][7] = {
    { 3, 3, 2, 2, 1, 0, 0},   /* E=NB */
    { 3, 3, 2, 1, 1, 0,-1},   /* E=NM */
    { 2, 2, 2, 1, 0,-1,-1},   /* E=NS */
    { 2, 2, 1, 0,-1,-2,-2},   /* E=ZO */
    { 1, 1, 0,-1,-1,-2,-2},   /* E=PS */
    { 1, 0,-1,-2,-2,-2,-3},   /* E=PM */
    { 0, 0,-2,-2,-2,-3,-3},   /* E=PB */
};

/* ΔKi 规则表  [E][EC] */
static const signed char ruleKi[7][7] = {
    {-3,-3,-2,-2,-1, 0, 0},   /* E=NB */
    {-3,-3,-2,-1,-1, 0, 0},   /* E=NM */
    {-3,-2,-1,-1, 0, 1, 1},   /* E=NS */
    {-2,-2,-1, 0, 1, 2, 2},   /* E=ZO */
    {-1,-1, 0, 1, 1, 2, 3},   /* E=PS */
    { 0, 0, 1, 1, 2, 3, 3},   /* E=PM */
    { 0, 0, 1, 2, 2, 3, 3},   /* E=PB */
};

/* ΔKd 规则表  [E][EC] */
static const signed char ruleKd[7][7] = {
    { 1,-1,-3,-3,-3,-2, 1},   /* E=NB */
    { 1,-1,-3,-2,-2,-1, 0},   /* E=NM */
    { 0,-1,-2,-2,-1,-1, 0},   /* E=NS */
    { 0,-1,-1,-1,-1,-1, 0},   /* E=ZO */
    { 0, 0, 0, 0, 0, 0, 0},   /* E=PS */
    { 3,-1, 1, 1, 1, 1, 3},   /* E=PM */
    { 3, 2, 2, 2, 1, 1, 3},   /* E=PB */
};

static float clampf(float x, float lo, float hi)
{
    if(x < lo) return lo;
    if(x > hi) return hi;
    return x;
}

/* 三角隶属: 把论域值 x∈[-3,3] 分解到相邻两个语言值, 返回索引与隶属度(m0+m1=1) */
static void fuzzify(float x, int *i0, int *i1, float *m0, float *m1)
{
    if(x <= -3.0f){ *i0 = 0; *i1 = 0; *m0 = 1.0f; *m1 = 0.0f; return; }
    if(x >=  3.0f){ *i0 = 6; *i1 = 6; *m0 = 1.0f; *m1 = 0.0f; return; }

    int lo = (int)x;                 /* 向下取整(floor) */
    if((float)lo > x) lo--;
    float frac = x - (float)lo;      /* [0,1) */

    *i0 = lo + 3;                    /* 偏移到 [0,6] */
    *i1 = *i0 + 1;
    *m0 = 1.0f - frac;               /* 下相邻语言值隶属度 */
    *m1 = frac;                      /* 上相邻语言值隶属度 */
}

/* 模糊推理: 由量化后的 eq、ecq 求 ΔKp/ΔKi/ΔKd(均在[-3,3]) */
static void fuzzy_tune(float eq, float ecq, float *dkp, float *dki, float *dkd)
{
    int ei0, ei1, ci0, ci1;
    float em0, em1, cm0, cm1;
    fuzzify(eq,  &ei0, &ei1, &em0, &em1);
    fuzzify(ecq, &ci0, &ci1, &cm0, &cm1);

    float w00 = em0*cm0, w01 = em0*cm1, w10 = em1*cm0, w11 = em1*cm1;   /* Σw=1 */

    *dkp = w00*ruleKp[ei0][ci0] + w01*ruleKp[ei0][ci1] + w10*ruleKp[ei1][ci0] + w11*ruleKp[ei1][ci1];
    *dki = w00*ruleKi[ei0][ci0] + w01*ruleKi[ei0][ci1] + w10*ruleKi[ei1][ci0] + w11*ruleKi[ei1][ci1];
    *dkd = w00*ruleKd[ei0][ci0] + w01*ruleKd[ei0][ci1] + w10*ruleKd[ei1][ci0] + w11*ruleKd[ei1][ci1];
}

void FuzzyPID_Init(FuzzyPID_t *fp,
                   float Kp0, float Ki0, float Kd0,
                   float Ke,  float Kec,
                   float Kup, float Kui, float Kud)
{
    fp->Kp0 = Kp0; fp->Ki0 = Ki0; fp->Kd0 = Kd0;
    fp->Ke  = Ke;  fp->Kec = Kec;
    fp->Kup = Kup; fp->Kui = Kui; fp->Kud = Kud;
    fp->e1  = 0.0f; fp->e2 = 0.0f;
}

void FuzzyPID_Reset(FuzzyPID_t *fp)
{
    fp->e1 = 0.0f;
    fp->e2 = 0.0f;
}

float FuzzyPID_Calc(FuzzyPID_t *fp, float target, float actual)
{
    float e  = target - actual;
    float ec = e - fp->e1;

    float eq  = clampf(e  * fp->Ke,  -3.0f, 3.0f);
    float ecq = clampf(ec * fp->Kec, -3.0f, 3.0f);

    float dkp, dki, dkd;
    fuzzy_tune(eq, ecq, &dkp, &dki, &dkd);

    float Kp = fp->Kp0 + dkp * fp->Kup;
    float Ki = fp->Ki0 + dki * fp->Kui;
    float Kd = fp->Kd0 + dkd * fp->Kud;
    if(Kp < 0.0f) Kp = 0.0f;          /* 整定后参数不应为负 */
    if(Ki < 0.0f) Ki = 0.0f;
    if(Kd < 0.0f) Kd = 0.0f;

    /* 增量式 PID: e-e1=本次误差变化, e-2*e1+e2=误差二阶差分 */
    float du = Kp * (e - fp->e1) + Ki * e + Kd * (e - 2.0f * fp->e1 + fp->e2);

    fp->e2 = fp->e1;
    fp->e1 = e;
    return du;
}
