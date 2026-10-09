// battery_soc.h
#ifndef __BATTERY_SOC_H
#define __BATTERY_SOC_H

#include <stdint.h>

// 电池SOC结构体
typedef struct {
    float voltage;          // 当前电压
    float soc;             // 当前电量百分比(0-100)
    float filtered_voltage; // 滤波后的电压
    uint32_t last_update;   // 上次更新时间
    uint8_t initialized;    // 初始化标志
    float min_voltage;      // 当前放电过程中的最低电压（用于SOC计算）
    uint8_t is_discharging; // 放电状态标志
    float voltage_buffer[5]; // 电压历史缓冲区
    uint8_t buffer_index;   // 缓冲区索引
} Battery_SOC_t;

// 初始化电池SOC计算模块
void Battery_SOC_Init(void);

// 更新电池电压并计算SOC
// voltage: ADC读取到的电压值
// timestamp: 当前时间戳(FreeRTOS ticks)
void Battery_SOC_Update(float voltage, uint32_t timestamp);

// 获取当前电量百分比(0-100)
float Battery_SOC_Get(void);

// 获取当前电压
float Battery_Voltage_Get(void);

// 手动设置电池为满电（用于换电池后重置）
void Battery_Set_Full(void);

void send_bat(void);

#endif
