#ifndef __TEMPERATURE_CONTROL_H
#define __TEMPERATURE_CONTROL_H

#include "main.h"
#include "tim.h"
#include "sht30_1.h"
#include "motor_speed.h"   // 包含电机控制函数声明

/*****************控制参数结构体*****************/
typedef struct {
    float S1;           // 设置温度下限
    float S2;           // 设置温度上限
    float Tc;           // 中心温度
    float L1;           // T1控制温区下限
    float L2;           // T1控制温区上限
    float delta_S;      // ΔS = S2 - S1
    float delta_L;      // ΔL = L2 - L1

    // 箱体参数
    int box_type;       // 箱体类型：0-398L上开盖托盘箱, 1-135L推车箱, 2-95L推车箱,
                        // 3-20L拉杆箱, 4-17L拉杆箱,
                        // 0x0A-91L大箱(50XPS), 0x07-91L大箱加厚(50XPS+10VIP),
                        // 0x08-51L小箱(50XPS), 0x09-51L小箱加厚(50XPS+10VIP)
    float K1;           // 箱体参数K1(货仓热阻,℃/W)
    float K2;           // 箱体参数K2(蓄冷仓热阻,℃/W)
    float M0;           // 相变材料单重(kg)，91L大箱默认1.0kg，51L小箱默认0.6kg
    int N;              // 相变材料数量，无人车默认14个
    int pcm_type;       // 相变材料类型：0-常规PCM, 1-低温PCM
    float delta_H;      // 相变材料潜热值(kJ/kg)

    // 控制状态
    uint8_t control_mode;  // 0-保温, 1-制冷, 2-加热
    float remaining_time;  // 剩余保温时间(小时)
    float remaining_capacity; // 剩余冷量(kJ)

    // 温度传感器值
    float T[6];          // T1-T6温度值
} TempControlParams;

// 箱体类型定义
#define BOX_398L     0
#define BOX_135L     1
#define BOX_95L      2
#define BOX_20L      3
#define BOX_17L      4
#define BOX_91L       0x0A   // 91L大箱(50XPS)
#define BOX_91L_THICK 0x07   // 91L大箱加厚(50XPS+10VIP)
#define BOX_51L       0x08   // 51L小箱(50XPS)
#define BOX_51L_THICK 0x09   // 51L小箱加厚(50XPS+10VIP)

// PCM类型定义
#define PCM_NORMAL   0
#define PCM_LOW      1

// 控制模式
#define MODE_IDLE    0  // 保温
#define MODE_COOL    1  // 制冷
#define MODE_HEAT    2  // 加热

/*****************函数声明*****************/
void TempControl_Init(TempControlParams *params);
void TempControl_SetTemperature(TempControlParams *params, float S1, float S2);
void TempControl_SetBoxType(TempControlParams *params, int box_type);
void TempControl_SetPCMType(TempControlParams *params, int pcm_type, int N);
uint8_t TempControl_Update(TempControlParams *params);
void TempControl_Reset(TempControlParams *params);
void TempControl_UpdateDeltaL(TempControlParams *params, float new_delta_L);
void convert_hours_to_hhmm_float(float last_time, uint16_t *hours, uint16_t *minutes);

#endif /* __TEMPERATURE_CONTROL_H */
