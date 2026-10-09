#include "main.h"
#include "IAP.h"
#include "usart.h"
#include "data_send.h"
#include "data_rec.h"


/* IAP 升级相关变量 */
static FIL     iap_file;               // 升级文件对象
static uint8_t iap_file_opened = 0;    // 文件是否已打开
static uint16_t iap_total_packets = 0; // 总包数
static uint16_t iap_current_packet = 0;// 期望的下一个包序号（1开始）
/**
 * @brief 发送 IAP 升级回复包
 * @param status 升级状态 (0x01:文件创建失败/不一致; 0x02:当前包接收成功; 0x03:包错误; 0x04:升级完成)
 * @param packet_num 当前包序号（低字节在前）
 */
void IAP_SendResponse(uint8_t status, uint16_t packet_num)
{
    uint8_t buff[30];
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x8C;
    buff[i++] = status;
    buff[i++] = (uint8_t)(packet_num & 0xFF);
    buff[i++] = (uint8_t)((packet_num >> 8) & 0xFF);

    uint16_t len_index = i;
    write_pkt_checksum(buff, len_index, &i);

    EC800Send_HexData(buff, i);
}
/**
 * @brief 处理 IAP 升级数据包
 * @param total_packets  总包数
 * @param current_packet 当前包序号（从1开始）
 * @param data           本包数据（200字节，最后一包可能包含无效补0）
 * @param data_len       实际数据长度（固定200，保留参数便于扩展）
 * @return 0:成功; -1:失败（需要上层处理）
 */
int IAP_ProcessUpgrade(uint16_t total_packets, uint16_t current_packet, uint8_t *data, uint16_t data_len)
{
    FRESULT res;
    UINT bw;

    // 第一包：创建/覆盖文件
    if (current_packet == 1) {
        if (iap_file_opened) {
            f_close(&iap_file);
            iap_file_opened = 0;
        }
        res = f_open(&iap_file, "firmware.bin", FA_CREATE_ALWAYS | FA_WRITE);
        if (res != FR_OK) {
            IAP_SendResponse(0x01, current_packet);
            return -1;
        }
        iap_file_opened = 1;
        iap_total_packets = total_packets;
        iap_current_packet = 1;
    }

    // 检查包序号是否连续
    if (current_packet != iap_current_packet) {
        IAP_SendResponse(0x03, iap_current_packet);
        return -1;
    }

    // 定位到文件偏移量（每个包200字节）
    uint32_t offset = (current_packet - 1) * 220;
    res = f_lseek(&iap_file, offset);
    if (res != FR_OK) {
        IAP_SendResponse(0x03, current_packet);
        return -1;
    }

    // 写入数据
    res = f_write(&iap_file, data, data_len, &bw);
    if (res != FR_OK || bw != data_len) {
        IAP_SendResponse(0x03, current_packet);
        return -1;
    }
    f_sync(&iap_file); // 立即写入物理存储

    iap_current_packet++;
    IAP_SendResponse(0x02, current_packet); // 回复成功

    // 最后一包处理
    if (current_packet == total_packets) {
        f_close(&iap_file);
        iap_file_opened = 0;

        FIL ready_file;
        res = f_open(&ready_file, "OK.DAT", FA_CREATE_ALWAYS | FA_WRITE);
        if (res == FR_OK) {
            f_close(&ready_file);
        }

        IAP_SendResponse(0x04, total_packets);
        HAL_Delay(500);
        HAL_NVIC_SystemReset();
    }

    return 0;
}
