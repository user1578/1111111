#include "main.h"
#include "key_ping.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"


extern uint8_t ping_end_data[3];
uint8_t key_ping_send[100];
/**
 * @brief 电源键处理：关机短按开机，开机长按关机，带防抖，防连按
 * @note  需在主循环中每40ms调用一次
 */
void PowerKey_Hand(void)
{
    static uint32_t press_start_time = 0;
    static uint8_t last_state = 1;          // 上次按键状态（假设高电平未按下）
    static uint8_t pressed = 0;              // 是否处于按下状态
    static uint8_t long_press_triggered = 0; // 长按是否已触发
    static uint8_t device_on = 0;            // 0: 关机, 1: 开机（初始为关机）

    #define LONG_PRESS_MS  2000              // 长按阈值2秒

    uint8_t current_state = power_key;       // 读取当前按键电平（0表示按下）

    // 按键按下边沿检测
    if (last_state == 1 && current_state == 0) {
        press_start_time = HAL_GetTick();    // 记录按下时刻
        pressed = 1;
        long_press_triggered = 0;
    }
    // 按键释放边沿检测
    else if (last_state == 0 && current_state == 1) {
        if (pressed && !long_press_triggered) {
            uint32_t duration = HAL_GetTick() - press_start_time;
            if (duration < LONG_PRESS_MS && !device_on) {
                // 短按且设备处于关机状态 -> 开机
                sprintf(key_ping_send, "page start");
                memcpy(key_ping_send + strlen(key_ping_send), ping_end_data, 3);
                HAL_UART_Transmit(&huart5, (uint8_t*)key_ping_send, strlen(key_ping_send), 1000);
                memset(key_ping_send, 0, 100);
                device_on = 1;
            }
        }
        pressed = 0;
        long_press_triggered = 0;
    }

    // 长按检测（按下过程中）
    if (pressed && !long_press_triggered) {
        uint32_t duration = HAL_GetTick() - press_start_time;
        if (duration >= LONG_PRESS_MS && device_on) {
            // 长按且设备处于开机状态 -> 关机
            sprintf(key_ping_send, "page clsose");
            memcpy(key_ping_send + strlen(key_ping_send), ping_end_data, 3);
            HAL_UART_Transmit(&huart5, (uint8_t*)key_ping_send, strlen(key_ping_send), 1000);
            memset(key_ping_send, 0, 100);
            device_on = 0;
            long_press_triggered = 1;        // 防止重复触发
        }
    }

    last_state = current_state;              // 更新状态
}
