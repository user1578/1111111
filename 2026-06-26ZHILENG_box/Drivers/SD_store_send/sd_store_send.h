// sd_store_send.h
#ifndef SD_STORE_SEND_H
#define SD_STORE_SEND_H

#include <stdint.h>
#include <stdbool.h>

// SD卡存储状态
typedef enum {
    SD_STORAGE_OK = 0,
    SD_STORAGE_NOT_READY,
    SD_STORAGE_FULL,
    SD_STORAGE_ERROR
} SD_Storage_Status_t;

// 数据包存储结构
typedef struct {
    uint32_t index;         // 数据包序号
    uint16_t year;          // 存储时的年份（如 2026）
    uint8_t  month;         // 月
    uint8_t  day;           // 日
    uint8_t  hour;          // 时
    uint8_t  minute;        // 分
    uint8_t  second;        // 秒
    uint16_t length;        // 数据包长度
    uint8_t  data[200];     // 完整的数据包（与wind_temp_send生成的一致）
} WindDataPacket_t;

// 函数声明
SD_Storage_Status_t SD_Storage_Init(void);
SD_Storage_Status_t SD_Storage_SavePacket(uint8_t *data, uint16_t length);
SD_Storage_Status_t SD_Storage_SavePacketWithTime(uint8_t *data, uint16_t length,
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second);
SD_Storage_Status_t SD_Storage_ReadPacket(uint32_t packet_index, WindDataPacket_t *packet);
SD_Storage_Status_t SD_Storage_RemoveSentPackets(uint32_t sent_count);
SD_Storage_Status_t SD_Storage_ClearAll(void);
SD_Storage_Status_t SD_Storage_Cleanup7Days(void);
uint32_t SD_Storage_GetCount(void);

// 打印日志功能（独立于离线补传，使用print_log.dat文件，保留7天）
SD_Storage_Status_t SD_Storage_SavePrintLog(uint8_t *data, uint16_t length,
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second);
SD_Storage_Status_t SD_Storage_ReadPrintLog(uint32_t packet_index, WindDataPacket_t *packet);
SD_Storage_Status_t SD_Storage_Cleanup7DaysPrintLog(void);
uint32_t SD_Storage_GetPrintLogCount(void);

// 全局变量声明
extern bool sd_has_offline_data;
extern uint32_t sd_offline_count;
extern uint32_t print_log_count;

#endif
