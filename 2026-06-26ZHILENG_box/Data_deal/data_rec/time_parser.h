#ifndef __TIME_PARSER_H
#define __TIME_PARSER_H

#include <stdint.h>

/**
 * @brief 北京时间结构体
 */
typedef struct {
    uint16_t year;   /* 年份，如 2026 */
    uint8_t  month;  /* 月，1~12 */
    uint8_t  day;    /* 日，1~31 */
    uint8_t  hour;   /* 时，0~23 */
    uint8_t  minute; /* 分，0~59 */
    uint8_t  second; /* 秒，0~59 */
} beijing_time_t;

/**
 * @brief 解析 +CCLK 返回的 UTC 时间，并转换为北京时间（UTC+8）
 * @param time_data_buffer 指向保存 AT 响应字符串的缓冲区（即 time_data 数组）
 * @param bj_time          输出北京时间结构体指针
 * @retval  0  解析成功
 * @retval -1  参数为空
 * @retval -2  未找到 "+CCLK:" 头
 * @retval -3  未找到时间字段的起始引号
 * @retval -4  格式解析失败（格式不符）
 * @retval -5  解析出的原始数值超出合法范围
 * @retval -6  日期无效（例如 2月30日）
 */
int parse_cck_time(const char *time_data_buffer, beijing_time_t *bj_time);

#endif /* __TIME_PARSER_H */
