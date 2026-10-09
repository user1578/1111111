#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include"freertos_demo.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include "IAP.h"
#include "rec_dealprint.h"
#include "Ble_driver.h"


uint8_t ble_rx_buffer[300];
uint8_t ble_rx_complete = 0;
uint8_t ble_send_tm=0;
/* 蓝牙接收环形缓冲区 */

uint8_t  ble_ring_buf[BLE_RING_SIZE];
uint16_t ble_ring_wr = 0;   // 写指针（中断中修改）
uint16_t ble_ring_rd = 0;   // 读指针（主循环中修改）


uint8_t  ble_frame[300];    // 存放当前正在组装的帧
uint16_t ble_index = 0;     // 当前已写入 ble_frame 的字节数
uint8_t  ble_exp_len = 0;   // 期望的剩余数据长度（从长度字节解析）

/* 帧解析状态机 */
typedef enum {
    BLE_STATE_SYNC1 = 0,    // 等待 0xAA
    BLE_STATE_SYNC2,        // 等待 0x55
    BLE_STATE_LEN,          // 读取长度字节
    BLE_STATE_DATA,         // 读取剩余数据 + 校验
} BleFrameState_t;

BleFrameState_t ble_state = BLE_STATE_SYNC1;
/* 最终完整帧存放区，与你的 deal_data_u1 用法一致 */
uint8_t ble_deal_data[300];
volatile uint8_t uart6_frame_ready = 0;   // 完整帧就绪标志
// 发送函数封装
HAL_StatusTypeDef BLE_UART_Transmit(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    return HAL_UART_Transmit(&huart6, pData, Size, Timeout);
}

/* 发送 AT 指令并等待期望的应答 */
uint8_t BLE_SendCmd(char *cmd, char *expected_response, uint16_t timeout_ms)
{
    uint8_t ret = 1; // 默认失败
    char tx_buffer[64];

    // 发送指令，末尾加上 \r\n
    sprintf(tx_buffer, "%s\r\n", cmd);
    HAL_UART_Transmit(&huart6, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

    // 等待响应（简单超时轮询，实际建议用状态机）
    uint32_t tickstart = HAL_GetTick();
    while ((HAL_GetTick() - tickstart) < timeout_ms)
    {
        if (ble_rx_complete)
        {
            ble_rx_complete = 0;
            // 在接收缓冲区中查找期望应答
            if (strstr((char*)ble_rx_buffer, expected_response) != NULL)
            {
                ret = 0; // 成功
                break;
            }
            memset(ble_rx_buffer, 0, BLE_RX_BUF_SIZE);
        }
        vTaskDelay(1);
    }
    return ret;
}

/* 进入 AT 配置模式（兼容已处于配置模式的情况） */
uint8_t BLE_EnterConfigMode(void)
{
    uint8_t retry = 0;
    char tx_buffer[20];

    for (retry = 0; retry < 2; retry++)
    {
        // 发送 "+++a"
        HAL_UART_Transmit(&huart6, (uint8_t*)"+++a", 4, 100);

        // 等待响应（最多 300ms）
        uint32_t tickstart = HAL_GetTick();
        while ((HAL_GetTick() - tickstart) < 300)
        {
            if (ble_rx_complete)
            {
                ble_rx_complete = 0;

                // 情况1：收到 "a+ok" —— 进入成功
                if (strstr((char*)ble_rx_buffer, "OK") != NULL)
                {
                    memset(ble_rx_buffer, 0, BLE_RX_BUF_SIZE);
                    return 0;   // 成功进入配置模式
                }

                // 情况2：收到 "ERROR" 且是第二次尝试，说明已在配置模式
                if (strstr((char*)ble_rx_buffer, "ERROR") != NULL)
                {
                    if (retry == 1)
                    {
                        memset(ble_rx_buffer, 0, BLE_RX_BUF_SIZE);
                        return 0;   // 已在配置模式，视为成功
                    }
                }

                memset(ble_rx_buffer, 0, BLE_RX_BUF_SIZE);
                break;  // 退出内层 while，进行下一次重试
            }
            vTaskDelay(1);
        }
    }

    return 1;   // 两次尝试均失败
}


void BLE_ConfigAsSlave(void)
{
    char cmd_buf[64];
    uint8_t retry;

    // 1. 进入配置模式（处理已处于配置模式的情况）
    if (BLE_EnterConfigMode() != 0)
    {
        // 进入配置模式失败，直接退出 也要是从机模式才能退出
        // printf_via_uart1("Enter config mode failed\r\n");
        sprintf(cmd_buf, "AT+MODE=S");
        BLE_SendCmd(cmd_buf, "OK", 200);
        return;
    }
    // printf_via_uart1("Enter config mode OK\r\n");

    // 2. AT 测试
    BLE_SendCmd("AT", "OK", 100);

    // 3. 设置蓝牙名称
    sprintf(cmd_buf, "AT+NAME=%s", BLE_NAME);
    BLE_SendCmd(cmd_buf, "OK", 200);

    // 4. 设置开机欢迎语（可选）
    sprintf(cmd_buf, "AT+HELLO=%s", BLE_HELLO);
    BLE_SendCmd(cmd_buf, "OK", 200);

    // 5. 设置发射功率（0dBm）
    sprintf(cmd_buf, "AT+TPL=%d", BLE_TPL);
    BLE_SendCmd(cmd_buf, "OK", 200);

    // 6. 设置串口参数（115200,8,N,1）
    sprintf(cmd_buf, "AT+UART=%d,%d,%d,%d", BLE_UART_BAUD, BLE_UART_DATA, BLE_UART_PARITY, BLE_UART_STOP);
    BLE_SendCmd(cmd_buf, "OK", 200);

    // 7. 设置广播速度（50ms）
    sprintf(cmd_buf, "AT+ADPTIM=%d", BLE_ADPTIM);
    BLE_SendCmd(cmd_buf, "OK", 200);

    // 8. 关闭链路匹配连接密码（可选）
    BLE_SendCmd("AT+LINKPASSEN=OFF", "OK", 200);

    // 9. 板载LED（可开可关，按需）
    BLE_SendCmd("AT+LEDEN=ON", "OK", 200);

    // 10. 关闭从机断连自动睡眠
    BLE_SendCmd("AT+SLAVESLEEPEN=OFF", "OK", 200);

    // 11. 配置最大输出功率（避免限速），重试3次
    for (retry = 0; retry < 3; retry++)
    {
        if (BLE_SendCmd("AT+MAXPUT=ON", "OK", 200) == 0)
        {
            break;  // 成功，退出重试循环
        }
        vTaskDelay(50);  // 短暂延时后重试
    }
    if (retry == 3)
    {
        // 3次均失败，退出配置流程
        // printf_via_uart1("AT+MAXPUT=ON failed after 3 retries\r\n");
        return;
    }

    // 12. 设置工作模式为从机（设置后模块复位并进入透传模式）
    sprintf(cmd_buf, "AT+MODE=S");
    BLE_SendCmd(cmd_buf, "OK", 200);

    // 清空接收缓冲，标记配置完成，进入透传
    memset(ble_rx_buffer, 0, 300);
    ble_send_tm = 1;
}
///* 向环形缓冲写入一字节（中断中调用） */
//void ble_ring_put(uint8_t byte)
//{
//    uint16_t next = (ble_ring_wr + 1) % BLE_RING_SIZE;
//    if (next != ble_ring_rd) {          // 未满
//        ble_ring_buf[ble_ring_wr] = byte;
//        ble_ring_wr = next;
//    }
//    // 若满则丢弃（可扩展溢出处理）
//}
//
///* 从环形缓冲读取一字节（主循环调用） */
//static inline uint8_t ble_ring_get(uint8_t *byte)
//{
//    if (ble_ring_wr == ble_ring_rd)
//        return 0;   // 空
//    *byte = ble_ring_buf[ble_ring_rd];
//    ble_ring_rd = (ble_ring_rd + 1) % BLE_RING_SIZE;
//    return 1;
//}
//
///* 尝试从环形缓冲中解析出一个完整帧，返回 1 表示已得到完整帧 */
//uint8_t ble_parse_frame(void)
//{
//    uint8_t byte;
//    while (ble_ring_get(&byte)) {
//        switch (ble_state) {
//            case BLE_STATE_SYNC1:
//                if (byte == 0xA5) {
//                    ble_frame[0] = 0xA5;
//                    ble_index = 1;
//                    ble_state = BLE_STATE_SYNC2;
//                }
//                break;
//
//            case BLE_STATE_SYNC2:
//                if (byte == 0x5A) {
//                    ble_frame[1] = 0x5A;
//                    ble_index = 2;
//                    ble_state = BLE_STATE_LEN;
//                } else {
//                    ble_state = BLE_STATE_SYNC1;   // 同步失败，重新找头
//                }
//                break;
//
//            case BLE_STATE_LEN:
//                ble_frame[2] = byte;
//                ble_exp_len = byte;                 // 长度字节含义：后续数据总长（包含地址、命令、数据、校验）
//                if (ble_exp_len > sizeof(ble_frame) - 3) {
//                    ble_state = BLE_STATE_SYNC1;    // 长度异常，丢弃
//                } else {
//                    ble_index = 3;
//                    ble_state = BLE_STATE_DATA;
//                }
//                break;
//
//            case BLE_STATE_DATA:
//                ble_frame[ble_index++] = byte;
//                if (ble_index >= (uint16_t)(3 + ble_exp_len)) {
//                    // 收齐一帧，验证校验和（可选，建议加上）
//                    uint8_t xor_calc = 0, add_calc = 0;
//                    for (uint16_t i = 3; i < 3 + ble_exp_len - 2; i++) {
//                        xor_calc ^= ble_frame[i];
//                        add_calc += ble_frame[i];
//                    }
//                    uint8_t xor_recv = ble_frame[3 + ble_exp_len - 2];
//                    uint8_t add_recv = ble_frame[3 + ble_exp_len - 1];
//                    if (xor_calc == xor_recv && add_calc == add_recv) {
//                        // 校验通过，复制到全局数组
//                        memcpy(ble_deal_data, ble_frame, ble_index);
//                        uart6_frame_ready = 1;
//                    }
//                    // 无论校验是否通过，都重置状态准备下一帧
//                    ble_state = BLE_STATE_SYNC1;
//                    ble_index = 0;
//                    return 1;   // 本次调用最多解析出一帧
//                }
//                break;
//        }
//    }
//    return 0;
//}
//
//void Ble_Process(void)
//{
//    if (ble_send_tm == 1) {     // 已退出配置模式，进入透传
//        // 不断解析，直到环形缓冲中无完整帧
//        while (ble_parse_frame()) {
//            // 当解析出一帧后，uart6_frame_ready 会被置 1
//            if (uart6_frame_ready) {
//                uart6_frame_ready = 0;
//
//                // 完全复用你原有 deal_data_u1 的处理逻辑，只需把数组名换为 ble_deal_data
//                uint8_t cmd = ble_deal_data[4];   // 命令字位置
//                switch (cmd) {
//                    case 0x8C: // 升级
////                        {
////                            uint16_t total   = (ble_deal_data[5] << 8) | ble_deal_data[6];
////                            uint16_t current = (ble_deal_data[7] << 8) | ble_deal_data[8];
////                            uint8_t *data    = &ble_deal_data[9];
////                            IAP_ProcessUpgrade(total, current, data, 200);
////                        }
//                        break;
//
//                    case 0x90: // 打印温湿度
//                        handle_print_command(ble_deal_data);  // 直接复用原函数
//                        break;
//
//                }
//            }
//        }
//    }
//}
