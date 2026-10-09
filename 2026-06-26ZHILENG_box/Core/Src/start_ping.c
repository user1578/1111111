#include "main.h"
#include "string.h"
#include "usart.h"

uint8_t ping_start_send[100];
extern uint8_t ping_end_data[3];
extern int32_t temp_high;      // 温度上限
extern int32_t temp_low;       // 温度下限
extern int32_t warn_high;      // 预警上限
extern int32_t warn_low;       // 预警下限
extern int32_t alarm_temp;     // 报警温度
extern int32_t alarm_low;      // 报警下限
extern uint32_t battery_warn_low;      // 电量预警下限（%）
extern uint32_t battery_alarm_low;     // 电量报警下限（%）
extern uint32_t keepwarn_low;          // 保温时长不足下限（小时）
extern uint32_t keepwarn_serious_low;  // 保温时长严重不足下限（小时）
extern uint8_t start_state;

void ping_start(void)
{
    sprintf(ping_start_send,"state=%ld",(uint32_t)(start_state));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));

    sprintf(ping_start_send,"set_temp.n0.val=%ld",(uint32_t)(temp_high));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));
    // 温度下限：F1 之后 4 字节，偏移 9
    sprintf(ping_start_send,"set_temp.n1.val=%ld",(uint32_t)(temp_low));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));
    // 预警上限：F2 之后 4 字节，偏移 13
    sprintf(ping_start_send,"set_temp.n2.val=%ld",(uint32_t)(warn_high));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));
    // 预警下限：F3 之后 4 字节，偏移 17
    sprintf(ping_start_send,"set_temp.n3.val=%ld",(uint32_t)(warn_low));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));
    sprintf(ping_start_send,"dianliang.n0.val=%ld",(uint32_t)(battery_warn_low));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));
    // 电量报警下限：F1 之后 4 字节，偏移 9
    sprintf(ping_start_send,"dianliang.n1.val=%ld",(uint32_t)(battery_alarm_low));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));
    // 保温时长不足下限：F2 之后 4 字节，偏移 14
    sprintf(ping_start_send,"dianliang.x0.val=%ld",(uint32_t)(keepwarn_low));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));
    // 保温时长严重不足下限：F3 之后 4 字节，偏移 19
    sprintf(ping_start_send,"dianliang.x1.val=%ld",(uint32_t)(keepwarn_serious_low));
    memcpy(ping_start_send+strlen(ping_start_send),ping_end_data,3);
    HAL_UART_Transmit(&huart5,(uint8_t*)ping_start_send,strlen(ping_start_send),1000);
    memset(ping_start_send,0,sizeof(ping_start_send));

}
