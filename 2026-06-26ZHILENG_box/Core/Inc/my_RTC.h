#ifndef __MY_RTC_H
#define __MY_RTC_H

#include "stm32f4xx_hal.h"



typedef struct {
    uint8_t year;   // 后两位，例如23表示2023
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} RTC_DateTime_t;



// 初始化 RTC
void RTC_Init(void);

// 设置 RTC 时间（从 GPS 数据）
void RTC_SetTimeFromGPS(uint16_t year, uint8_t month, uint8_t day,
                        uint8_t hour, uint8_t minute, uint8_t second);

// 获取当前 RTC 时间（可选，用于调试或显示）
void RTC_GetDateTime(RTC_DateTime_t *datetime);

uint32_t DateTimeToSeconds(uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute, uint8_t second);

// 校验日期时间是否合法（年份 2026~2035，月日时分秒范围正确，含闰年）
// 返回 1 合法，0 非法
int is_valid_datetime(uint16_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t minute, uint8_t second);

#endif /* __MY_RTC_H */
