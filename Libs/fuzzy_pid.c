/***********************************************************************
文件名称：fuzzy_pid.c
功    能：模糊自整定 PID(增量式)实现
算法流程：
  1) 误差 e=目标-实际, 误差变化 ec=e-上次e;
  2) 用量化因子 Ke、Kec 把 e、ec 映射到论域[-3,3];
  3) 三角隶属度把论域值分解到相邻两个语言值, E、EC 各取2个 -> 4 条激活规则;
  4) 查 ΔKp/ΔKi/ΔKd 规则表并按隶属度加权平均(重心法简化, Σ权=1);
  5) Kp=Kp0+ΔKp*Kup, Ki=Ki0+ΔKi*Kui, Kd=Kd0+ΔKd*Kud, 并限幅;
  6) 增量式: Δu = Kp*(e-e1) + Ki*e - Kd*(pv-2*pv1+pv2)  (D 用测量值微分)。
精细处理:
  - 稳态死区: |e| <= 满量程1%(论域内 |eq|<=0.03)时不整定, 防稳态附近抖动;
  - 测量值微分: D 项用测量值(pv)二阶差分而非误差, 消除改目标值时的微分冲击;
  - 整定限幅: Kp/Ki/Kd 整定后钳到 [0, 基准×FUZZY_KMAX], 防极端工况增益被推过大。
***********************************************************************/
#include "fuzzy_pid.h"

#define FUZZY_KMAX  4.0f        /* 整定后增益上限 = 基准的倍数 */

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
    fp->e1  = 0.0f; fp->pv1 = 0.0f; fp->pv2 = 0.0f;
}

void FuzzyPID_Reset(FuzzyPID_t *fp)
{
    fp->e1  = 0.0f;
    fp->pv1 = 0.0f;
    fp->pv2 = 0.0f;
}

float FuzzyPID_Calc(FuzzyPID_t *fp, float target, float actual)
{
    float e  = target - actual;
    float ec = e - fp->e1;

    float eq  = clampf(e  * fp->Ke,  -3.0f, 3.0f);
    float ecq = clampf(ec * fp->Kec, -3.0f, 3.0f);

    float dkp = 0.0f, dki = 0.0f, dkd = 0.0f;
    if((eq > 0.03f) || (eq < -0.03f))         /* 稳态死区(|e|<=满量程1%)内不整定, 防稳态抖动 */
    {
        fuzzy_tune(eq, ecq, &dkp, &dki, &dkd);
    }

    float Kp = clampf(fp->Kp0 + dkp * fp->Kup, 0.0f, fp->Kp0 * FUZZY_KMAX);   /* 整定后限幅: [0, 基准×KMAX] */
    float Ki = clampf(fp->Ki0 + dki * fp->Kui, 0.0f, fp->Ki0 * FUZZY_KMAX);
    float Kd = clampf(fp->Kd0 + dkd * fp->Kud, 0.0f, fp->Kd0 * FUZZY_KMAX);

    /* 增量式 PID: P/I 用误差, D 用测量值二阶差分(消除改目标值时的微分冲击) */
    float du = Kp * ec + Ki * e - Kd * (actual - 2.0f * fp->pv1 + fp->pv2);

    fp->e1  = e;
    fp->pv2 = fp->pv1;
    fp->pv1 = actual;
    return du;
}
