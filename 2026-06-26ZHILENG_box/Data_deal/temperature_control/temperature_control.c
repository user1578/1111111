#include "temperature_control.h"
#include <math.h>
#include <stdio.h>

// 全局变量，存储6个传感器的温度值
extern float temp[8];
extern float humi[8];
uint16_t fengshan1_speed;
uint16_t fengshan2_speed;
uint16_t heat_data;
float last_time = 0.0;
uint16_t last_hour = 0;
uint16_t last_min = 0;
uint16_t coda_value=0;
float see_detaL=0.0;
float see_L1=0.0;
float see_L2=0.0;
// 将小时浮点数转换为小时和分钟（四舍五入）
void convert_hours_to_hhmm_float(float last_time, uint16_t *hours, uint16_t *minutes)
{
    *hours = (uint16_t)last_time;
    float decimal_part = last_time - (float)(*hours);
    *minutes = (uint16_t)(decimal_part * 60.0f + 0.5f);
    if (*minutes >= 60) {
        *hours += 1;
        *minutes = 0;
    }
}

/*****************验证函数：检查温度区间有效性*****************/
uint8_t ValidateTemperatureRange(float S1, float S2)
{
    if (S1 >= S2) return 0;

    float delta_S = S2 - S1;
    float delta_L;

    if (fabs(delta_S - 2.0f) < 0.1f) {
        delta_L = 0.6f;
    } else if (delta_S >= 3.0f && delta_S < 6.0f) {
        delta_L = 1.0f;
    } else if (delta_S >= 6.0f && delta_S < 10.0f) {
        delta_L = 2.0f;
    } else if (delta_S >= 10.0f) {
        delta_L = 4.0f;
    } else {
        return 0;
    }

    if (delta_S <= (delta_L + 1.2f)) return 0;

    float Tc = (S1 + S2) / 2.0f;
    float L1 = Tc - delta_L / 2.0f;
    float L2 = Tc + delta_L / 2.0f;

    if (S1 > L1 || L2 > S2) return 0;

    return 1;
}

/*****************初始化函数*****************/
void TempControl_Init(TempControlParams *params)
{
    params->S1 = 2.0f;
    params->S2 = 8.0f;
    params->control_mode = MODE_IDLE;
    params->remaining_time = 0.0f;
    params->remaining_capacity = 0.0f;

    params->box_type = BOX_91L;
    params->K1 = 1.16f;
    params->K2 = 1.75f;

    params->pcm_type = PCM_NORMAL;
    params->M0 = 1.0f;
    params->N = 14;
    params->delta_H = 355.0f;

    TempControl_SetTemperature(params, params->S1, params->S2);
}

/*****************重置控制参数*****************/
void TempControl_Reset(TempControlParams *params)
{
    motor1_stop();
    motor2_stop();
    moter3_stop();
    moter4_stop();
    heat_stop();

    params->control_mode = MODE_IDLE;
    fengshan1_speed = 0;
    fengshan2_speed = 0;
    heat_data = 0;
}

/*****************设置温度区间*****************/
void TempControl_SetTemperature(TempControlParams *params, float S1, float S2)
{
    if (!ValidateTemperatureRange(S1, S2)) {
        printf("无效的温度区间设置：S1=%.1f, S2=%.1f\n", S1, S2);
        return;
    }

    params->S1 = S1;
    params->S2 = S2;
    params->delta_S = S2 - S1;

    if (fabs(params->delta_S - 2.0f) < 0.1f) {
        params->delta_L = 0.6f;
    } else if (params->delta_S >= 3.0f && params->delta_S < 6.0f) {
        params->delta_L = 1.0f;
    } else if (params->delta_S >= 6.0f && params->delta_S < 10.0f) {
        params->delta_L = 2.0f;
    } else if (params->delta_S >= 10.0f) {
        params->delta_L = 4.0f;
    }

    params->Tc = (S1 + S2) / 2.0f;
    params->L1 = params->Tc - params->delta_L / 2.0f;
    params->L2 = params->Tc + params->delta_L / 2.0f;

    printf("控制参数：\n");
    printf("  S1=%.1f℃, S2=%.1f℃, ΔS=%.1f℃\n", params->S1, params->S2, params->delta_S);
    printf("  Tc=%.1f℃, ΔL=%.1f℃\n", params->Tc, params->delta_L);
    printf("  L1=%.1f℃, L2=%.1f℃\n", params->L1, params->L2);
    printf("  T1控制温区：[%.1f, %.1f]℃\n", params->L1, params->L2);
    printf("  T3/T4控制温区：[%.1f, %.1f]℃\n", params->L1, params->L2);
    printf("  T5控制温区：[%.1f, %.1f]℃\n", params->S1 + 0.6f, params->L1);
}

/*****************设置箱体类型*****************/
void TempControl_SetBoxType(TempControlParams *params, int box_type)
{
    params->box_type = box_type;

    switch (box_type) {
        case BOX_398L:
            params->K1 = 0.91f;   params->K2 = 1.66f;   break;
        case BOX_135L:
            params->K1 = 1.655f;  params->K2 = 2.912f;  break;
        case BOX_95L:
            params->K1 = 2.06f;   params->K2 = 3.08f;   break;
        case BOX_20L:
            params->K1 = 5.12f;   params->K2 = 6.31f;   break;
        case BOX_17L:
            params->K1 = 5.62f;   params->K2 = 6.18f;   break;
        case BOX_91L:            // 91L大箱(50XPS)
            params->K1 = 1.16f;   params->K2 = 1.75f;   break;
        case BOX_91L_THICK:      // 91L大箱加厚(50XPS+10VIP)
            params->K1 = 2.23f;   params->K2 = 3.38f;   break;
        case BOX_51L:            // 51L小箱(50XPS)
            params->K1 = 1.70f;   params->K2 = 2.86f;   break;
        case BOX_51L_THICK:      // 51L小箱加厚(50XPS+10VIP)
            params->K1 = 3.3f;    params->K2 = 5.5f;    break;
        default:
            params->K1 = 1.16f;   params->K2 = 1.75f;   break;
    }

    // M0: 91L大箱=1.0kg, 51L小箱=0.6kg; N由SetPCMType根据屏幕设置cold_data赋值
    if (box_type == BOX_51L || box_type == BOX_51L_THICK) {
        params->M0 = 0.6f;
    } else {
        params->M0 = 1.0f;
    }
}

/*****************设置PCM类型*****************/
void TempControl_SetPCMType(TempControlParams *params, int pcm_type, int N)
{
    params->pcm_type = pcm_type;
    params->N = N;

    if (pcm_type == PCM_NORMAL) {
        params->delta_H = 355.0f;
    } else {
        params->delta_H = 240.0f;
    }
}

/*****************计算η（相变完成度）*****************/
static float CalculateEta(TempControlParams *params, float T6)
{
    float eta = 0.0f;
    if (params->pcm_type == PCM_NORMAL)
    {
        // 常规PCM: T6<-5 → η=0; -5≤T6<6.0 → η=1-0.76/(1+e^(1.88*(T6-2.86))); T6≥6.0 → η=1
        if (T6 < -5.0f) {
            eta = 0.0f;
        } else if (T6 < 6.0f) {
            float exponent = 1.88f * (T6 - 2.86f);
            eta = 1.0f - 0.63f / (1.0f + expf(exponent));
        } else {
            eta = 1.0f;
        }
    }
    else // PCM_LOW
    {
        // 低温PCM: T6<-27 → η=0; -27≤T6<-19 → η=1-1/(1+e^((T6+23.146)/0.8755)); T6≥-19 → η=1
        if (T6 < -27.0f) {
            eta = 0.0f;
        } else if (T6 < -19.0f) {
            float exponent = (T6 + 23.146f) / 0.8755f;
            eta = 1.0f - 1.0f / (1.0f + expf(exponent));
        } else {
            eta = 1.0f;
        }
    }
    return eta;
}

/*****************获取分段系数（对应系数）*****************/
// 常规PCM按T6区间返回时间/冷量系数；低温PCM无分段系数(恒为1.0)
static float GetSegmentCoefficient(TempControlParams *params, float T6)
{
    if (params->pcm_type == PCM_NORMAL)
    {
        if (T6 < -5.0f)         return 1.0f;   // T6<-5        tm=t×1.0
        else if (T6 < 2.0f)     return 0.9f;   // [-5.0,2.0)   tm=t×0.9
        else if (T6 < 2.3f)     return 1.0f;   // [2.0,2.3)    tm=t×1.0
        else if (T6 < 2.6f)     return 1.1f;   // [2.3,2.6)    tm=t×1.1
        else if (T6 < 2.75f)    return 1.2f;   // [2.6,2.75)   tm=t×1.2
        else if (T6 < 3.8f)     return 1.0f;   // [2.75,3.8)   tm=t×1.0
        else if (T6 < 4.0f)     return 1.2f;   // [3.8,4.0)    tm=t×1.2
        else if (T6 < 4.3f)     return 1.5f;   // [4.0,4.3)    tm=t×1.5
        else if (T6 < 5.0f)     return 2.0f;   // [4.3,5.0)    tm=t×2.0
        else if (T6 < 6.0f)     return 1.0f;   // [5.0,6.0)    tm=t×1.0
        else                    return 0.0f;   // T6≥6.0 → η=1, tm=0
    }
    // 低温PCM: 显示计算值，无额外分段系数
    return 1.0f;
}

/*****************计算剩余保温时间（新版，含分段系数）*****************/
static float CalculateRemainingTimeNew(TempControlParams *params, float eta, float T6)
{
    float T1 = params->T[0];
    float T2 = params->T[1];

    // T1边界: T1≥S2 或 T1≤S1 → tm=0
    if (T1 >= params->S2 || T1 <= params->S1) return 0.0f;
    // η≈1(常规T6≥6.2, 低温T6≥-19) → tm=0
    if (eta >= 0.999f) return 0.0f;

    // 基础公式: t = (1/3.6) × [M0×N×ΔH×(1-η)/(T2-T6)] × { K1/[(T2-T1)/(T2-T6) + K1/K2] }
    float numerator = params->M0 * params->N * params->delta_H * (1.0f - eta); // kJ
    float denominator = T2 - T6;
    if (fabsf(denominator) < 0.001f) return 9999.0f;

    float term1 = (T2 - T1) / denominator;
    float term2 = params->K1 / params->K2;
    float part2 = params->K1 / (term1 + term2);
    float t = (1.0f / 3.6f) * (numerator / denominator) * part2;
    if (t < 0.0f) t = 0.0f;

    // 分段系数: 常规PCM按T6区间乘系数, 低温PCM系数恒为1.0
    t *= GetSegmentCoefficient(params, T6);

    return t;
}

/*****************计算剩余保温时间（对外接口，内部计算eta）*****************/
//float CalculateRemainingTime(TempControlParams *params)
//{
//    float T6 = params->T[5];
//    float eta = CalculateEta(params, T6);
//    return CalculateRemainingTimeWithEta(params, eta);
//}

/*****************温度控制主函数*****************/
uint8_t TempControl_Update(TempControlParams *params)
{
    uint8_t i;
    uint8_t status = 0;

    // 读取所有温度传感器
    for (i = 0; i < 6; i++) {
        status = SHT30_Read_Humiture_Device(&sht30_devices[i], &temp[i], &humi[i]);
        if (status != 0) {
            temp[i] = -100.0f; // 无效值
        }
        params->T[i] = temp[i];
    }

    if (params->T[0] < -100.0f) {
        printf("T1读取失败，跳过本次控制\n");
        return 1;
    }

    float T1 = params->T[0];
    float T2 = params->T[1];
    float T3 = params->T[2];
    float T4 = params->T[3];
    float T5 = params->T[4];
    float T6 = params->T[5];

    printf("实时温度：T1=%.1f℃, T2=%.1f℃, T3=%.1f℃, T4=%.1f℃, T5=%.1f℃, T6=%.1f℃\n",
           T1, T2, T3, T4, T5, T6);

    // 计算η
    float eta = CalculateEta(params, T6);

    // 剩余冷量(%): 实时潜热量 = (1-η) × 100，限制在[0,100]
    float cap = (1.0f - eta) * 100.0f;
    if (cap < 0.0f) cap = 0.0f;
    if (cap > 100.0f) cap = 100.0f;
    params->remaining_capacity = cap;

    see_detaL= params->delta_L;
    see_L1=params->L1;
	see_L2= params->L2;
    // --- 温度控制逻辑 ---
    if (T2 >= params->S1 && T1 >= params->L1) {
        // 5.1 制冷程序
        if (params->control_mode != MODE_COOL) {
            printf("切换到制冷程序（T2≥S1且T1≥L1）\n");
            params->control_mode = MODE_COOL;
        }

        // 风扇1&2控制（电机1-4），全部由T1控制，温区 [L1, S2]
        // T1>S2 全速；L1<T1<=S2 线性减速；T1<=L1 停止（进风回风同步）
        if (T1 <= params->L1) {
            printf("风扇停止（T1=%.1f℃ ≤ L1=%.1f℃）\n", T1, params->L1);
            motor1_stop();
            motor2_stop();
            moter3_stop();
            moter4_stop();
            fengshan1_speed = 0;
            fengshan2_speed = 0;
        } else if (T1 > params->S2) {
            printf("风扇全速（T1=%.1f℃ > S2=%.1f℃）\n", T1, params->S2);
            moter1_speed(300);
            moter2_speed(300);
            moter3_speed(300);
            moter4_speed(300);
            fengshan1_speed = 2800;
            fengshan2_speed = 2800;
        } else {
            float speed = (T1 - params->L1) / (params->S2 - params->L1) * 300.0f;
            if (speed < 30) {
                printf("风扇停止（计算速度%.1f < 10%%）\n", speed);
                motor1_stop();
                motor2_stop();
                moter3_stop();
                moter4_stop();
                fengshan1_speed = 0;
                fengshan2_speed = 0;
            } else {
                printf("风扇转速：%.1f（T1从%.1f到%.1f）\n",
                       speed, params->L1, params->S2);
                moter1_speed((uint16_t)speed);
                moter2_speed((uint16_t)speed);
                moter3_speed((uint16_t)speed);
                moter4_speed((uint16_t)speed);
                fengshan1_speed = (uint16_t)(speed * 9.33f);
                fengshan2_speed = (uint16_t)(speed * 9.33f);
            }
        }

        // 制冷模式下关闭加热
        heat_stop();
        heat_data = 0;

    } else if (T2 < params->S1 && T1 <= params->L1) {
        // 5.2 加热程序
        if (params->control_mode != MODE_HEAT) {
            printf("切换到加热程序（T2<S1且T1≤L1）\n");
            params->control_mode = MODE_HEAT;
        }

        // 关闭所有风扇
        motor1_stop();
        motor2_stop();
        moter3_stop();
        moter4_stop();
        fengshan1_speed = 0;
        fengshan2_speed = 0;

        // 加热片控制
        if (T5 > params->L1) {
            printf("加热停止（T5=%.1f℃ > L1=%.1f℃）\n", T5, params->L1);
            heat_stop();
            heat_data = 0;
        } else if (T5 < (params->S1 + 0.6f)) {
            printf("加热最大功率（T5=%.1f℃ < S1+0.6=%.1f℃）\n",
                   T5, params->S1 + 0.6f);
            heat_warm(300);
            heat_data = 300;
        } else {
            float warm = (params->L1 - T5) / (params->L1 - (params->S1 + 0.6f)) * 300.0f;
            if (warm < 0) warm = 0;
            if (warm > 300) warm = 300;
            if (warm < 30) {
                printf("加热停止（计算功率%.1f < 10%%）\n", warm);
                heat_stop();
                heat_data = 0;
            } else {
                printf("加热功率：%.1f（T5从%.1f到%.1f）\n",
                       warm, params->L1, params->S1 + 0.6f);
                heat_warm((uint16_t)warm);
                heat_data = (uint16_t)warm;
            }
        }

    } else {
        // 5.3 保温模式（其他所有情况）
        if (params->control_mode != MODE_IDLE) {
            printf("切换到保温程序（不满足制冷/加热条件）\n");
            params->control_mode = MODE_IDLE;
        }

        // 关闭所有风扇和加热
        motor1_stop();
        motor2_stop();
        moter3_stop();
        moter4_stop();
        fengshan1_speed = 0;
        fengshan2_speed = 0;
        heat_stop();
        heat_data = 0;
    }

    // 计算剩余保温时间和冷量
    coda_value = (uint16_t)(params->remaining_capacity);
    params->remaining_time = CalculateRemainingTimeNew(params, eta, T6);   // 改用新函数
    last_time = params->remaining_time;
    convert_hours_to_hhmm_float(last_time, &last_hour, &last_min);

    return 0;
}

void TempControl_UpdateDeltaL(TempControlParams *params, float new_delta_L)
{
    params->delta_L = new_delta_L;
    params->L1 = params->Tc - params->delta_L / 2.0f;
    params->L2 = params->Tc + params->delta_L / 2.0f;

    printf("ΔL 已更新为 %.1f℃, L1=%.1f℃, L2=%.1f℃\n",
           params->delta_L, params->L1, params->L2);
}
