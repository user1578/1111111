#include "time_parser.h"
#include <string.h>
#include <stdio.h>

/* 平年各月天数 */
static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/**
 * @brief 判断是否为闰年
 */
static int is_leap_year(uint16_t year)
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

/**
 * @brief 获取指定年月的实际天数
 */
static uint8_t month_days(uint16_t year, uint8_t month)
{
    if (month == 2 && is_leap_year(year))
        return 29;
    return days_in_month[month - 1];
}

/**
 * @brief 解析 +CCLK 时间并转换为北京时间
 * @note  输入格式示例：+CCLK: "26/06/18,14:08:07+32"
 *        年份为两位，默认为2000~2099年
 *        时区偏移单位：15分钟（+32 表示 UTC+8）
 *        本函数直接将小时+8得到北京时间，同时正确处理跨日、跨月、跨年
 */
int parse_cck_time(const char *time_data_buffer, beijing_time_t *bj_time)
{
    if (time_data_buffer == NULL || bj_time == NULL)
        return -1;

    /* 1. 定位 "+CCLK:" */
    char *p = strstr((char *)time_data_buffer, "+CCLK:");
    if (p == NULL)
        return -2;

    /* 2. 跳过至第一个双引号之后 */
    p = strchr(p, '\"');
    if (p == NULL)
        return -3;
    p++;

    int yy, mon, day, hour, min, sec;
    int tz_offset;  /* 时区偏移，单位15分钟，例如+32 */

    /* 3. 按格式提取：yy/MM/dd,hh:mm:ss±zz */
    if (sscanf(p, "%2d/%2d/%2d,%2d:%2d:%2d%3d", &yy, &mon, &day, &hour, &min, &sec, &tz_offset) != 7)
        return -4;

    /* 4. 年份转换（仅支持2000~2099） */
    uint16_t year = 2000 + (uint16_t)(yy);
    if (year > 2099) year -= 100;   /* 保险处理 */

    /* 5. 基础范围检查 */
    if (mon < 1 || mon > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
        return -5;

    uint8_t max_d = month_days(year, (uint8_t)mon);
    if (day > max_d)
        return -6;

    /* 6. UTC + 8 小时 → 北京时间 */
    hour += 8;

    /* 7. 处理小时进位导致的日期/月份/年份变化 */
    while (hour >= 24) {
        hour -= 24;
        day++;
        if (day > month_days(year, (uint8_t)mon)) {
            day = 1;
            mon++;
            if (mon > 12) {
                mon = 1;
                year++;
            }
        }
    }

    /* 8. 输出结果 */
    bj_time->year   = year;
    bj_time->month  = (uint8_t)mon;
    bj_time->day    = (uint8_t)day;
    bj_time->hour   = (uint8_t)hour;
    bj_time->minute = (uint8_t)min;
    bj_time->second = (uint8_t)sec;

    return 0;
}
