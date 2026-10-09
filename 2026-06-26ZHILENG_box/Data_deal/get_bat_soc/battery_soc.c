// battery_soc.c
#include "battery_soc.h"
#include "main.h"
#include <math.h>
#include "FreeRTOS.h"
#include "string.h"
#include "task.h"
#include "get_bat.h"

extern float bat_voltage;
extern float battery_soc;
extern char ping_send_bat[100];
extern uint8_t ping_end_data[3];
// 静态变量
static Battery_SOC_t battery = {0};

// 电池特性参数
typedef struct {
    float voltage_adc;  // ADC电压(分压后)
    float soc;          // 电量百分比
} Voltage_SOC_Table_t;

// 更平滑的放电曲线（防止SOC跳变）
static const Voltage_SOC_Table_t soc_table[] = {
    {1.80f, 0.0f},     // 9.0V   -> 0%
    {1.82f, 1.0f},     // 9.1V   -> 1%
    {1.84f, 2.0f},     // 9.2V   -> 2%
    {1.86f, 3.5f},     // 9.3V   -> 3.5%
    {1.88f, 5.0f},     // 9.4V   -> 5%
    {1.90f, 7.0f},     // 9.5V   -> 7%
    {1.92f, 10.0f},    // 9.6V   -> 10%
    {1.94f, 13.0f},    // 9.7V   -> 13%
    {1.96f, 17.0f},    // 9.8V   -> 17%
    {1.98f, 21.0f},    // 9.9V   -> 21%
    {2.00f, 25.0f},    // 10.0V  -> 25%
    {2.02f, 30.0f},    // 10.1V  -> 30%
    {2.04f, 35.0f},    // 10.2V  -> 35%
    {2.06f, 40.0f},    // 10.3V  -> 40%
    {2.08f, 45.0f},    // 10.4V  -> 45%
    {2.10f, 50.0f},    // 10.5V  -> 50%
    {2.12f, 55.0f},    // 10.6V  -> 55%
    {2.14f, 60.0f},    // 10.7V  -> 60%
    {2.16f, 65.0f},    // 10.8V  -> 65%
    {2.18f, 70.0f},    // 10.9V  -> 70%
    {2.20f, 75.0f},    // 11.0V  -> 75%
    {2.22f, 78.0f},    // 11.1V  -> 78%
    {2.24f, 81.0f},    // 11.2V  -> 81%
    {2.26f, 84.0f},    // 11.3V  -> 84%
    {2.28f, 87.0f},    // 11.4V  -> 87%
    {2.30f, 90.0f},    // 11.5V  -> 90%
    {2.32f, 92.0f},    // 11.6V  -> 92%
    {2.34f, 94.0f},    // 11.7V  -> 94%
    {2.36f, 95.5f},    // 11.8V  -> 95.5%
    {2.38f, 97.0f},    // 11.9V  -> 97%
    {2.40f, 98.0f},    // 12.0V  -> 98%
    {2.42f, 99.0f},    // 12.1V  -> 99%
    {2.44f, 99.5f},    // 12.2V  -> 99.5%
    {2.46f, 100.0f},   // 12.3V  -> 100%
};

#define SOC_TABLE_SIZE (sizeof(soc_table)/sizeof(soc_table[0]))

// 参数配置
#define VOLTAGE_DIVIDER_RATIO  5.122f    // 分压比 = 12.6/2.46
#define FILTER_ALPHA           0.3f      // 低通滤波器系数
#define JUMP_THRESHOLD         0.25f     // 电压跳变阈值(V)，超过此值认为是换电池
#define UPDATE_INTERVAL_TICKS  (pdMS_TO_TICKS(1000))  // 更新间隔1秒
#define STABLE_COUNT_THRESHOLD 3         // 稳定计数阈值
#define MIN_DISCHARGE_VOLTAGE  1.80f     // 最小放电电压

void Battery_SOC_Init(void)
{
    battery.voltage = 0;
    battery.soc = 100.0f;  // 默认满电
    battery.filtered_voltage = 0;
    battery.last_update = 0;
    battery.initialized = 0;
    battery.min_voltage = 2.46f;  // 初始为满电电压
    battery.is_discharging = 0;
    battery.buffer_index = 0;

    // 初始化电压缓冲区
    for (int i = 0; i < 5; i++) {
        battery.voltage_buffer[i] = 2.46f;  // 初始为满电电压
    }
}

// 中值滤波函数（用于去除尖峰干扰）
static float MedianFilter(float new_voltage)
{
    // 更新缓冲区
    battery.voltage_buffer[battery.buffer_index] = new_voltage;
    battery.buffer_index = (battery.buffer_index + 1) % 5;

    // 复制缓冲区进行排序
    float temp_buffer[5];
    for (int i = 0; i < 5; i++) {
        temp_buffer[i] = battery.voltage_buffer[i];
    }

    // 冒泡排序（简单实现）
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4 - i; j++) {
            if (temp_buffer[j] > temp_buffer[j + 1]) {
                float temp = temp_buffer[j];
                temp_buffer[j] = temp_buffer[j + 1];
                temp_buffer[j + 1] = temp;
            }
        }
    }

    // 返回中值
    return temp_buffer[2];
}

static float LowPassFilter(float new_value, float old_value, float alpha)
{
    return old_value * (1.0f - alpha) + new_value * alpha;
}

static float CalculateSOCFromVoltage(float voltage)
{
    // 边界检查
    if (voltage <= soc_table[0].voltage_adc) {
        return 0.0f;
    }
    if (voltage >= soc_table[SOC_TABLE_SIZE - 1].voltage_adc) {
        return 100.0f;
    }

    // 查找电压所在区间
    for (int i = 0; i < SOC_TABLE_SIZE - 1; i++) {
        if (voltage >= soc_table[i].voltage_adc &&
            voltage <= soc_table[i + 1].voltage_adc) {

            // 线性插值计算SOC
            float voltage_range = soc_table[i + 1].voltage_adc - soc_table[i].voltage_adc;
            float soc_range = soc_table[i + 1].soc - soc_table[i].soc;
            float ratio = (voltage - soc_table[i].voltage_adc) / voltage_range;

            return soc_table[i].soc + ratio * soc_range;
        }
    }

    return 50.0f;  // 默认值
}

// 基于最低电压的SOC计算方法（放电时只降不升）
static float CalculateSOCBasedOnMinVoltage(float current_voltage, float min_voltage)
{
    // 使用当前电压和最低电压中的较小值来计算SOC
    float voltage_for_soc = (current_voltage < min_voltage) ? current_voltage : min_voltage;

    // 确保电压在合理范围内
    if (voltage_for_soc > 2.46f) voltage_for_soc = 2.46f;
    if (voltage_for_soc < 1.80f) voltage_for_soc = 1.80f;

    // 计算SOC
    float soc = CalculateSOCFromVoltage(voltage_for_soc);

    // 添加滞后效应：电量下降后，电压回升不会立即增加电量
    // 只有当电压持续高于当前最低电压一定时间后，才更新最低电压
    static uint8_t voltage_recovery_count = 0;
    static float last_min_voltage = 2.46f;

    if (current_voltage > (min_voltage + 0.05f)) {  // 电压回升超过0.05V
        voltage_recovery_count++;
        if (voltage_recovery_count >= 5) {  // 连续5次测量都回升
            // 更新最低电压，允许SOC轻微回升
            battery.min_voltage = min_voltage + 0.02f;  // 允许SOC轻微回升
            voltage_recovery_count = 0;
        }
    } else {
        voltage_recovery_count = 0;
    }

    return soc;
}

void Battery_SOC_Update(float voltage, uint32_t timestamp)
{
    static uint8_t filter_init_done = 0;
    static float last_raw_voltage = 0;

    // 检查更新间隔
    if ((timestamp - battery.last_update) < UPDATE_INTERVAL_TICKS && battery.initialized) {
        return;
    }

    // 1. 先进行中值滤波去除尖峰
    float median_voltage = MedianFilter(voltage);

    // 2. 检测电压跳变（换电池）
    float voltage_diff = 0;
    if (battery.initialized) {
        voltage_diff = median_voltage - last_raw_voltage;

        if (fabsf(voltage_diff) > JUMP_THRESHOLD) {
            // 检测到大跳变，认为是换电池
            // 重置放电状态
            battery.is_discharging = 0;
            battery.min_voltage = median_voltage;

            // 如果是电压上升跳变（换新电池），重置为满电
            if (voltage_diff > 0.1f) {
                battery.soc = 100.0f;
            }

            // 直接使用新电压值，不进行低通滤波
            battery.filtered_voltage = median_voltage;
            filter_init_done = 1;
        } else {
            // 正常状态，使用低通滤波器
            if (filter_init_done) {
                battery.filtered_voltage = LowPassFilter(median_voltage,
                    battery.filtered_voltage, FILTER_ALPHA);
            } else {
                battery.filtered_voltage = median_voltage;
                filter_init_done = 1;
            }

            // 检测是否进入放电状态
            if (battery.filtered_voltage < (last_raw_voltage - 0.01f)) {
                // 电压持续下降，进入放电状态
                battery.is_discharging = 1;
            }
        }
    } else {
        // 第一次初始化
        battery.filtered_voltage = median_voltage;
        battery.min_voltage = median_voltage;
        filter_init_done = 1;
        battery.initialized = 1;
    }

    // 3. 更新最低电压（放电时记录最低电压）
    if (battery.filtered_voltage < battery.min_voltage) {
        battery.min_voltage = battery.filtered_voltage;
    }

    // 4. 计算电池实际电压
    battery.voltage = battery.filtered_voltage * VOLTAGE_DIVIDER_RATIO;

    // 5. 计算SOC（放电时使用基于最低电压的算法）
    float new_soc = 0;

    if (battery.is_discharging) {
        // 放电状态：使用基于最低电压的算法，确保SOC只降不升
        new_soc = CalculateSOCBasedOnMinVoltage(battery.filtered_voltage, battery.min_voltage);

        // 确保SOC不会上升（添加小幅滞后）
        if (new_soc > battery.soc) {
            // 如果计算出的SOC比当前SOC高，保持当前SOC不变
            // 或者只允许微小的回升（防止电量显示不合理地增加）
            if (new_soc - battery.soc > 1.0f) {
                // 如果差异大于1%，说明可能有问题，使用较小值
                new_soc = battery.soc;
            } else {
                // 允许微小回升（不超过1%）
                new_soc = battery.soc + 0.2f;  // 每次最多增加0.2%
            }
        }

        // 对SOC进行轻度滤波（只对下降进行滤波，上升不滤波）
        if (new_soc < battery.soc) {
            battery.soc = LowPassFilter(new_soc, battery.soc, 0.5f);
        } else {
            battery.soc = new_soc;
        }
    } else {
        // 非放电状态（可能是换电池后或充电）：正常计算SOC
        new_soc = CalculateSOCFromVoltage(battery.filtered_voltage);

        // 使用低通滤波使SOC变化平滑
        battery.soc = LowPassFilter(new_soc, battery.soc, 0.3f);
    }

    // 6. 确保SOC在0-100范围内
    if (battery.soc > 100.0f) battery.soc = 100.0f;
    if (battery.soc < 0.0f) battery.soc = 0.0f;

    // 7. 更新记录
    last_raw_voltage = median_voltage;
    battery.last_update = timestamp;
}

float Battery_SOC_Get(void)
{
    return battery.soc;
}

float Battery_Voltage_Get(void)
{
    return battery.voltage;
}

void Battery_Set_Full(void)
{
    battery.soc = 100.0f;
    battery.min_voltage = 2.46f;  // 重置最低电压为满电电压
    battery.is_discharging = 0;
}

void send_bat(void)
{
    // 获取当前时间戳(假设你有获取毫秒时间的函数)
    uint32_t timestamp = HAL_GetTick();
	bat_voltage=get_bat_voltage();
    // 更新电池状态
    Battery_SOC_Update(bat_voltage, timestamp);

    // 获取电量百分比
    battery_soc= Battery_SOC_Get();
    if(battery_soc<=10)
    {
    	change_bat_ON();     //切换备用电源
    }

}
