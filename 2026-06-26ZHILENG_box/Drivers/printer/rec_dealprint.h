#include "main.h"

#define MAX_RECORDS 864  // 最多3天数据，5分钟一条
// 定义温湿度记录结构体
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    int16_t temp1;  // 温度1，实际值*10
    uint8_t humi1;  // 湿度1
    int16_t temp2;  // 温度2，实际值*10
    uint8_t humi2;  // 湿度2
} TempRecord;

void sort_records(void);
void handle_print_command(uint8_t *pkt);
