#include "main.h"


#define BLE_HELLO "Welcome"          // 开机欢迎语，可自定义
#define BLE_TPL 5                    // 发射功率 0dBm
#define BLE_UART_BAUD 115200
#define BLE_UART_DATA 8
#define BLE_UART_PARITY 0
#define BLE_UART_STOP 1
#define BLE_ADPTIM 5                 // 广播间隔 50ms
#define BLE_RING_SIZE   1024
// 接收缓冲区
#define BLE_RX_BUF_SIZE 300




uint8_t BLE_SendCmd(char *cmd, char *expected_response, uint16_t timeout_ms);
void BLE_ConfigAsSlave(void);
void Ble_Process(void);
uint8_t ble_parse_frame(void);
void ble_ring_put(uint8_t byte);
