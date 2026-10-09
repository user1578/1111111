// sd_store_send.c
#include "sd_store_send.h"
#include "msd.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>
#include "my_RTC.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "iwdg.h"

// 全局变量
bool sd_has_offline_data = false;
uint32_t sd_offline_count = 0;
uint32_t print_log_count = 0;  // 打印日志数据包数量

// 存储文件名
#define OFFLINE_DATA_FILE "offline.dat"
#define PRINT_LOG_FILE   "plog.dat"
#define MAX_PACKETS 1000  // 最大存储1000个数据包
#define SD_LOCK_TIMEOUT_MS  5000  // SD卡锁超时5秒，超时放弃本次存储/读取（清理类仍用portMAX_DELAY）

// 私有变量
static FATFS fs;
static bool sd_initialized = false;
static SemaphoreHandle_t sd_mutex = NULL;  // SD卡递归互斥锁

// 初始化SD卡存储
SD_Storage_Status_t SD_Storage_Init(void)
{
    FRESULT res;

    // 创建递归互斥锁（允许同任务内嵌套加锁）
    if(sd_mutex == NULL) {
        sd_mutex = xSemaphoreCreateRecursiveMutex();
        if(sd_mutex == NULL) {
            return SD_STORAGE_ERROR;
        }
    }

    // 硬件初始化
    if(MSD_Init() != 0) {
        return SD_STORAGE_NOT_READY;
    }

    // 挂载文件系统 - 根据你的FatFs版本使用正确的方法
    // 方法1：旧版FatFs（两个参数）
    res = f_mount(0, &fs);  // 第一个参数是驱动器号，第二个是FATFS指针

    // 或者方法2：新版FatFs（三个参数）
    // res = f_mount(&fs, "0:", 1);  // 这个可能不兼容你的版本

    if(res != FR_OK) {
        return SD_STORAGE_ERROR;
    }

    // 检查是否有离线数据
    FIL file;
    res = f_open(&file, OFFLINE_DATA_FILE, FA_READ);
    if(res == FR_OK) {
        // 获取文件大小
        DWORD file_size = f_size(&file);
        sd_offline_count = file_size / sizeof(WindDataPacket_t);
        sd_has_offline_data = (sd_offline_count > 0);
        f_close(&file);

        if(sd_has_offline_data) {
            printf("[SD] 发现 %lu 个离线数据包\n", sd_offline_count);
        }
    }

    // 检查打印日志文件
    res = f_open(&file, PRINT_LOG_FILE, FA_READ);
    if(res == FR_OK) {
        DWORD file_size = f_size(&file);
        print_log_count = file_size / sizeof(WindDataPacket_t);
        f_close(&file);
        printf("[SD] 发现 %lu 个打印日志数据包\n", print_log_count);
    }

    sd_initialized = true;
    return SD_STORAGE_OK;
}

// 存储数据包到SD卡（带时间戳）
SD_Storage_Status_t SD_Storage_SavePacketWithTime(uint8_t *data, uint16_t length,
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second)
{
    if(!sd_initialized) {
        return SD_STORAGE_NOT_READY;
    }

    // 检查长度
    if(length > 200) {
        return SD_STORAGE_ERROR;
    }

    // 带超时获取锁，超时放弃本次存储（避免清理卡死时阻塞存储任务）
    if(xSemaphoreTakeRecursive(sd_mutex, pdMS_TO_TICKS(SD_LOCK_TIMEOUT_MS)) != pdTRUE) {
        printf("[SD] 存储获取锁超时，放弃本次存储\n");
        return SD_STORAGE_ERROR;
    }

    FIL file;
    FRESULT res;
    UINT bw;

    // 先尝试打开现有文件，不存在则创建
    res = f_open(&file, OFFLINE_DATA_FILE, FA_WRITE | FA_READ);
    if(res == FR_OK) {
        // 文件存在，移动到末尾
        f_lseek(&file, f_size(&file));
    } else if(res == FR_NO_FILE) {
        // 文件不存在，创建新文件
        res = f_open(&file, OFFLINE_DATA_FILE, FA_CREATE_NEW | FA_WRITE);
    }

    if(res != FR_OK) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    // 准备存储的数据包
    WindDataPacket_t packet;
    packet.index = sd_offline_count + 1;
    packet.year = year;
    packet.month = month;
    packet.day = day;
    packet.hour = hour;
    packet.minute = minute;
    packet.second = second;
    packet.length = length;
    memcpy(packet.data, data, length);

    // 写入文件
    res = f_write(&file, &packet, sizeof(WindDataPacket_t), &bw);

    // 确保数据写入
    f_sync(&file);
    f_close(&file);

    if(res == FR_OK && bw == sizeof(WindDataPacket_t)) {
        sd_offline_count++;
        sd_has_offline_data = true;
        printf("[SD] 存储数据包 #%lu 成功\n", packet.index);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_OK;
    }

    printf("[SD] 存储失败: %d\n", res);
    xSemaphoreGiveRecursive(sd_mutex);
    return SD_STORAGE_ERROR;
}

// 兼容旧接口：不带时间，时间字段填 0
SD_Storage_Status_t SD_Storage_SavePacket(uint8_t *data, uint16_t length)
{
    return SD_Storage_SavePacketWithTime(data, length, 0, 0, 0, 0, 0, 0);
}

// 获取当前存储的数据包数量
uint32_t SD_Storage_GetCount(void)
{
    return sd_offline_count;
}

// 清理超过7天的数据包
SD_Storage_Status_t SD_Storage_Cleanup7Days(void)
{
    if(!sd_initialized || sd_offline_count <= 1) {
        return SD_STORAGE_OK;
    }

    xSemaphoreTakeRecursive(sd_mutex, portMAX_DELAY);

    FIL file;
    FRESULT res;
    UINT br;

    // 单次打开文件，读取最新包（文件末尾）
    res = f_open(&file, OFFLINE_DATA_FILE, FA_READ);
    if(res != FR_OK) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    f_lseek(&file, (sd_offline_count - 1) * sizeof(WindDataPacket_t));
    WindDataPacket_t newest;
    res = f_read(&file, &newest, sizeof(WindDataPacket_t), &br);
    if(res != FR_OK || br != sizeof(WindDataPacket_t)) {
        f_close(&file);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    uint32_t newest_secs = DateTimeToSeconds(newest.year, newest.month, newest.day,
                                              newest.hour, newest.minute, newest.second);

    // 7天 = 7*24*3600 = 604800 秒
    if(newest_secs < 604800) {
        f_close(&file);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_OK;  // 时间不够长，不需要清理
    }
    uint32_t cutoff = newest_secs - 604800;

    // 从头顺序扫描，统计需要删除的包数量（单次打开，不再逐包开关文件）
    uint32_t remove_count = 0;
    WindDataPacket_t pkt;
    f_lseek(&file, 0);
    for(uint32_t i = 0; i < sd_offline_count; i++) {
        res = f_read(&file, &pkt, sizeof(WindDataPacket_t), &br);
        if(res != FR_OK || br != sizeof(WindDataPacket_t)) break;
        uint32_t pkt_secs = DateTimeToSeconds(pkt.year, pkt.month, pkt.day,
                                               pkt.hour, pkt.minute, pkt.second);
        if(pkt_secs < cutoff) {
            remove_count++;
        } else {
            break;  // 数据按时间顺序存储，遇到足够的就停止
        }
        if((i % 10) == 0) IWDG_Feed();  // 定期喂狗
    }
    f_close(&file);

    if(remove_count > 0) {
        printf("[SD] 清理 %lu 个超过7天的数据包\n", remove_count);
        SD_Storage_RemoveSentPackets(remove_count);
    }

    xSemaphoreGiveRecursive(sd_mutex);
    return SD_STORAGE_OK;
}

// 读取一个数据包（用于后续发送）
SD_Storage_Status_t SD_Storage_ReadPacket(uint32_t packet_index, WindDataPacket_t *packet)
{
    if(!sd_initialized) {
        return SD_STORAGE_NOT_READY;
    }

    if(packet_index >= sd_offline_count) {
        return SD_STORAGE_ERROR;
    }

    // 带超时获取锁，超时放弃本次读取
    if(xSemaphoreTakeRecursive(sd_mutex, pdMS_TO_TICKS(SD_LOCK_TIMEOUT_MS)) != pdTRUE) {
        printf("[SD] 读取获取锁超时，放弃本次读取\n");
        return SD_STORAGE_ERROR;
    }

    FIL file;
    FRESULT res;
    UINT br;

    res = f_open(&file, OFFLINE_DATA_FILE, FA_READ);
    if(res != FR_OK) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    // 定位到指定数据包
    res = f_lseek(&file, packet_index * sizeof(WindDataPacket_t));
    if(res != FR_OK) {
        f_close(&file);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    // 读取数据包
    res = f_read(&file, packet, sizeof(WindDataPacket_t), &br);
    f_close(&file);

    xSemaphoreGiveRecursive(sd_mutex);

    if(res == FR_OK && br == sizeof(WindDataPacket_t)) {
        return SD_STORAGE_OK;
    }

    return SD_STORAGE_ERROR;
}

// 删除已发送的数据包（重建文件）
SD_Storage_Status_t SD_Storage_RemoveSentPackets(uint32_t sent_count)
{
    if(!sd_initialized || sent_count == 0) {
        return SD_STORAGE_OK;
    }

    xSemaphoreTakeRecursive(sd_mutex, portMAX_DELAY);

    if(sent_count >= sd_offline_count) {
        // 所有数据包都已发送，删除文件
        FRESULT res = f_unlink(OFFLINE_DATA_FILE);
        if(res == FR_OK || res == FR_NO_FILE) {
            sd_offline_count = 0;
            sd_has_offline_data = false;
            xSemaphoreGiveRecursive(sd_mutex);
            return SD_STORAGE_OK;
        }
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    // 创建临时文件，复制未发送的数据
    FIL src_file, dst_file;
    FRESULT res;
    UINT br, bw;
    WindDataPacket_t packet;

    res = f_open(&src_file, OFFLINE_DATA_FILE, FA_READ);
    if(res != FR_OK) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    // 用 FA_CREATE_ALWAYS 避免残留临时文件导致失败
    res = f_open(&dst_file, "temp.dat", FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        f_close(&src_file);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    // 跳过已发送的数据包
    f_lseek(&src_file, sent_count * sizeof(WindDataPacket_t));

    // 复制剩余数据包
    uint32_t new_count = 0;
    while(1) {
        res = f_read(&src_file, &packet, sizeof(WindDataPacket_t), &br);
        if(res != FR_OK || br != sizeof(WindDataPacket_t)) {
            break;
        }

        // 更新索引
        packet.index = new_count + 1;
        res = f_write(&dst_file, &packet, sizeof(WindDataPacket_t), &bw);
        if(res != FR_OK || bw != sizeof(WindDataPacket_t)) {
            break;
        }
        new_count++;
        if((new_count % 10) == 0) IWDG_Feed();  // 定期喂狗
    }

    f_sync(&dst_file);
    f_close(&src_file);
    f_close(&dst_file);

    // 删除原文件，重命名临时文件
    f_unlink(OFFLINE_DATA_FILE);
    f_rename("temp.dat", OFFLINE_DATA_FILE);

    // 更新计数
    sd_offline_count = new_count;
    sd_has_offline_data = (sd_offline_count > 0);

    printf("[SD] 已删除 %lu 个已发送数据包，剩余 %lu 个\n", sent_count, new_count);

    xSemaphoreGiveRecursive(sd_mutex);
    return SD_STORAGE_OK;
}

// 清空所有离线数据
SD_Storage_Status_t SD_Storage_ClearAll(void)
{
    xSemaphoreTakeRecursive(sd_mutex, portMAX_DELAY);
    FRESULT res = f_unlink(OFFLINE_DATA_FILE);
    if(res == FR_OK || res == FR_NO_FILE) {
        sd_offline_count = 0;
        sd_has_offline_data = false;
        printf("[SD] 所有离线数据已清空\n");
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_OK;
    }
    xSemaphoreGiveRecursive(sd_mutex);
    return SD_STORAGE_ERROR;
}

// ===================== 打印日志功能（print_log.dat）=====================

// 存储打印日志数据包
SD_Storage_Status_t SD_Storage_SavePrintLog(uint8_t *data, uint16_t length,
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second)
{
    if(!sd_initialized) {
        return SD_STORAGE_NOT_READY;
    }
    if(length > 200) {
        return SD_STORAGE_ERROR;
    }

    // 带超时获取锁，超时放弃本次存储（避免清理卡死时阻塞存储任务）
    if(xSemaphoreTakeRecursive(sd_mutex, pdMS_TO_TICKS(SD_LOCK_TIMEOUT_MS)) != pdTRUE) {
        printf("[SD] 打印日志存储获取锁超时，放弃本次存储\n");
        return SD_STORAGE_ERROR;
    }

    FIL file;
    FRESULT res;
    UINT bw;

    res = f_open(&file, PRINT_LOG_FILE, FA_WRITE | FA_READ);
    if(res == FR_OK) {
        f_lseek(&file, f_size(&file));
    } else if(res == FR_NO_FILE) {
        res = f_open(&file, PRINT_LOG_FILE, FA_CREATE_NEW | FA_WRITE);
    }

    if(res != FR_OK) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    WindDataPacket_t packet;
    packet.index = print_log_count + 1;
    packet.year = year;
    packet.month = month;
    packet.day = day;
    packet.hour = hour;
    packet.minute = minute;
    packet.second = second;
    packet.length = length;
    memcpy(packet.data, data, length);

    res = f_write(&file, &packet, sizeof(WindDataPacket_t), &bw);
    f_sync(&file);
    f_close(&file);

    if(res == FR_OK && bw == sizeof(WindDataPacket_t)) {
        print_log_count++;
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_OK;
    }
    xSemaphoreGiveRecursive(sd_mutex);
    return SD_STORAGE_ERROR;
}

// 读取打印日志数据包
SD_Storage_Status_t SD_Storage_ReadPrintLog(uint32_t packet_index, WindDataPacket_t *packet)
{
    if(!sd_initialized) {
        return SD_STORAGE_NOT_READY;
    }
    if(packet_index >= print_log_count) {
        return SD_STORAGE_ERROR;
    }

    // 带超时获取锁，超时放弃本次读取
    if(xSemaphoreTakeRecursive(sd_mutex, pdMS_TO_TICKS(SD_LOCK_TIMEOUT_MS)) != pdTRUE) {
        printf("[SD] 打印日志读取获取锁超时，放弃本次读取\n");
        return SD_STORAGE_ERROR;
    }

    FIL file;
    FRESULT res;
    UINT br;

    res = f_open(&file, PRINT_LOG_FILE, FA_READ);
    if(res != FR_OK) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    res = f_lseek(&file, packet_index * sizeof(WindDataPacket_t));
    if(res != FR_OK) {
        f_close(&file);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    res = f_read(&file, packet, sizeof(WindDataPacket_t), &br);
    f_close(&file);

    xSemaphoreGiveRecursive(sd_mutex);

    if(res == FR_OK && br == sizeof(WindDataPacket_t)) {
        return SD_STORAGE_OK;
    }
    return SD_STORAGE_ERROR;
}

// 获取打印日志数量
uint32_t SD_Storage_GetPrintLogCount(void)
{
    return print_log_count;
}

// 清理超过7天的打印日志（重建文件，删除过期数据）
SD_Storage_Status_t SD_Storage_Cleanup7DaysPrintLog(void)
{
    if(!sd_initialized || print_log_count <= 1) {
        return SD_STORAGE_OK;
    }

    xSemaphoreTakeRecursive(sd_mutex, portMAX_DELAY);

    FIL file;
    FRESULT res;
    UINT br;

    // 单次打开文件，读取最新包（文件末尾）
    res = f_open(&file, PRINT_LOG_FILE, FA_READ);
    if(res != FR_OK) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    f_lseek(&file, (print_log_count - 1) * sizeof(WindDataPacket_t));
    WindDataPacket_t newest;
    res = f_read(&file, &newest, sizeof(WindDataPacket_t), &br);
    if(res != FR_OK || br != sizeof(WindDataPacket_t)) {
        f_close(&file);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    uint32_t newest_secs = DateTimeToSeconds(newest.year, newest.month, newest.day,
                                              newest.hour, newest.minute, newest.second);
    if(newest_secs < 604800) {
        f_close(&file);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_OK;
    }
    uint32_t cutoff = newest_secs - 604800;

    // 从头顺序扫描，统计需要删除的包数量（单次打开，不再逐包开关文件）
    uint32_t remove_count = 0;
    WindDataPacket_t pkt;
    f_lseek(&file, 0);
    for(uint32_t i = 0; i < print_log_count; i++) {
        res = f_read(&file, &pkt, sizeof(WindDataPacket_t), &br);
        if(res != FR_OK || br != sizeof(WindDataPacket_t)) break;
        uint32_t pkt_secs = DateTimeToSeconds(pkt.year, pkt.month, pkt.day,
                                               pkt.hour, pkt.minute, pkt.second);
        if(pkt_secs < cutoff) {
            remove_count++;
        } else {
            break;
        }
        if((i % 10) == 0) IWDG_Feed();  // 定期喂狗
    }
    f_close(&file);

    if(remove_count == 0) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_OK;
    }

    // 重建文件：跳过过期数据，保留剩余数据
    FIL src_file, dst_file;
    UINT bw;

    res = f_open(&src_file, PRINT_LOG_FILE, FA_READ);
    if(res != FR_OK) {
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    // 用 FA_CREATE_ALWAYS 避免残留临时文件导致失败
    res = f_open(&dst_file, "ptmp.dat", FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        f_close(&src_file);
        xSemaphoreGiveRecursive(sd_mutex);
        return SD_STORAGE_ERROR;
    }

    f_lseek(&src_file, remove_count * sizeof(WindDataPacket_t));

    uint32_t new_count = 0;
    while(1) {
        res = f_read(&src_file, &pkt, sizeof(WindDataPacket_t), &br);
        if(res != FR_OK || br != sizeof(WindDataPacket_t)) {
            break;
        }
        pkt.index = new_count + 1;
        res = f_write(&dst_file, &pkt, sizeof(WindDataPacket_t), &bw);
        if(res != FR_OK || bw != sizeof(WindDataPacket_t)) {
            break;
        }
        new_count++;
        if((new_count % 10) == 0) IWDG_Feed();  // 定期喂狗
    }

    f_sync(&dst_file);
    f_close(&src_file);
    f_close(&dst_file);

    f_unlink(PRINT_LOG_FILE);
    f_rename("ptmp.dat", PRINT_LOG_FILE);

    print_log_count = new_count;
    printf("[SD] 打印日志清理 %lu 个过期包，剩余 %lu 个\n", remove_count, new_count);

    xSemaphoreGiveRecursive(sd_mutex);
    return SD_STORAGE_OK;
}
