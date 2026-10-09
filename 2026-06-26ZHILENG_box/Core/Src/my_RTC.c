#include "stm32f4xx_hal.h"
#include "my_RTC.h"

RTC_HandleTypeDef hrtc;

extern uint16_t RTC_year;
extern uint8_t RTC_month;
extern uint8_t RTC_day;
extern uint8_t RTC_hour;
extern uint8_t RTC_min;
extern uint8_t RTC_second;

void RTC_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    // 强制使用 LSI
    __HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);           // 关闭 LSE
    __HAL_RCC_LSI_ENABLE();                       // 使能 LSI
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET); // 等待 LSI 就绪

    // 配置 RTC 时钟源为 LSI
    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);
    __HAL_RCC_RTC_ENABLE();

    // 可选的延迟，确保时钟稳定
//    for (uint32_t i = 0; i < 1000; i++) __NOP();

    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
//    hrtc.Init.RefClockDetection = RTC_REFCLOCK_DISABLE;

    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        Error_Handler();
    }

    // 可选：设置默认时间（例如编译时间）
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        // 如果RTC未初始化过，可设置一个默认时间（示例：2023-01-01 00:00:00）
        sDate.Year = 23;      // 2023年，格式为 0-99
        sDate.Month = 1;
        sDate.Date = 1;
        sDate.WeekDay = RTC_WEEKDAY_MONDAY;
        HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        sTime.Hours = 0;
        sTime.Minutes = 0;
        sTime.Seconds = 0;
        HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    }
}

void RTC_SetTimeFromGPS(uint16_t year, uint8_t month, uint8_t day,
                        uint8_t hour, uint8_t minute, uint8_t second)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // 填充日期
    sDate.Year = year % 100;          // 年份取后两位（0-99）
    sDate.Month = month;
    sDate.Date = day;
    // 计算星期几（可选，若不需要可任意填）
    // 使用蔡勒公式或简单计算，这里假设用户已提供或省略，我们填1

    // 填充时间
    sTime.Hours = hour;
    sTime.Minutes = minute;
    sTime.Seconds = second;
    sTime.TimeFormat = RTC_HOURFORMAT_24;

    // 写日期和时间
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}

// 读取RTC时间并填充到传入的结构体中
void RTC_GetDateTime(RTC_DateTime_t *datetime)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    datetime->year   = sDate.Year;
    datetime->month  = sDate.Month;
    datetime->day    = sDate.Date;
    datetime->hour   = sTime.Hours;
    datetime->minute = sTime.Minutes;
    datetime->second = sTime.Seconds;

    RTC_year=(datetime->year)+2000;
    RTC_month=datetime->month;
    RTC_day=datetime->day;
    RTC_hour=datetime->hour;
    RTC_min=datetime->minute;
    RTC_second= datetime->second;
}

// 辅助函数：将年月日时分秒转换为从 2000-01-01 00:00:00 开始的秒数
uint32_t DateTimeToSeconds(uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute, uint8_t second)
{
    const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t seconds = 0;

    // 年份累计（从 2000 年开始）
    for (uint16_t y = 2025; y < year; y++) {
        seconds += 365 * 24 * 3600;
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
            seconds += 24 * 3600;          // 闰年加一天
    }

    // 月份累计
    for (uint8_t m = 1; m < month; m++) {
        seconds += days_in_month[m-1] * 24 * 3600;
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
            seconds += 24 * 3600;          // 闰年二月加一天
    }

    // 日累计
    seconds += (day - 1) * 24 * 3600;
    // 时分秒累计
    seconds += hour * 3600 + minute * 60 + second;

    return seconds;
}

// 校验日期时间合法性
int is_valid_datetime(uint16_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t minute, uint8_t second)
{
    static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (year < 2026 || year > 2035) return 0;
    if (month < 1 || month > 12) return 0;
    if (day < 1) return 0;
    if (hour > 23) return 0;
    if (minute > 59) return 0;
    if (second > 59) return 0;

    uint8_t max_day = days_in_month[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
        max_day = 29;
    if (day > max_day) return 0;

    return 1;
}
