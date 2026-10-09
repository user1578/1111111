// offline_sender.c
#include "offline_sender.h"
#include "sd_store_send.h"
#include "data_send.h"
#include "main.h"
#include <stdio.h>

// 全局变量
static bool is_sending_offline = false;
static uint32_t current_packet_index = 0;
static uint32_t send_timer = 0;
static uint32_t sent_count = 0;
extern uint8_t _4g_is_connecting;  // 连接中标志

// 检查并上传离线数据
void Offline_Sender_Process(void)
{
    static uint32_t last_check_time = 0;
    uint32_t current_time = HAL_GetTick();

    // 每5秒检查一次
    if(current_time - last_check_time < 5000) {
        return;
    }
    last_check_time = current_time;

    // 状态机处理
    if(!sd_has_offline_data) {
        // 没有离线数据，无需处理
        return;
    }

    if(_4g_is_connecting == 1) {
        // 网络未连接，不能发送
        is_sending_offline = false;
        return;
    }

    if(!is_sending_offline) {
        // 开始发送离线数据
        is_sending_offline = true;
        current_packet_index = 0;
        sent_count = 0;
        send_timer = current_time;
        printf("[离线发送] 开始发送离线数据，共 %lu 个数据包\n", sd_offline_count);
    }

    // 检查是否到达发送间隔（5秒）
    if(current_time - send_timer >= 5000) {
        send_timer = current_time;

        // 读取当前数据包
        WindDataPacket_t packet;
        if(SD_Storage_ReadPacket(current_packet_index, &packet) == SD_STORAGE_OK) {
            // 发送数据包
            EC800Send_HexData(packet.data, packet.length);
            printf("[离线发送] 发送第 %lu 个数据包 (时间: %d-%d-%d %d:%d:%d)\n",
                   packet.index, packet.year, packet.month, packet.day,
                   packet.hour, packet.minute, packet.second);

            current_packet_index++;
            sent_count++;

            // 检查是否发送完毕
            if(current_packet_index >= sd_offline_count) {
                // 所有数据包发送完毕
                printf("[离线发送] 离线数据发送完成，共 %lu 个数据包\n", sent_count);

                // 删除已发送的数据包
                SD_Storage_RemoveSentPackets(sent_count);

                // 重置状态
                is_sending_offline = false;
                current_packet_index = 0;
                sent_count = 0;
            }
        } else {
            // 读取失败，停止发送
            printf("[离线发送] 读取数据包失败，停止发送\n");
            is_sending_offline = false;
        }
    }
}

// 获取发送状态
bool Offline_Sender_IsActive(void)
{
    return is_sending_offline;
}

// 获取已发送数量
uint32_t Offline_Sender_GetSentCount(void)
{
    return sent_count;
}

// 获取剩余数量
uint32_t Offline_Sender_GetRemainingCount(void)
{
    if(current_packet_index < sd_offline_count) {
        return sd_offline_count - current_packet_index;
    }
    return 0;
}

// 停止发送
void Offline_Sender_Stop(void)
{
    is_sending_offline = false;
    current_packet_index = 0;
    sent_count = 0;
    printf("[离线发送] 停止发送\n");
}
