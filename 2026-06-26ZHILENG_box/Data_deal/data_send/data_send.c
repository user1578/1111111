#include "main.h"
#include "usart.h"
#include "string.h"
#include "Auxiliary.h"
#include "sys.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gps_deal.h"
#include "4g_con.h"
#include "string.h"
#include <stdio.h>
#include "offline_sender.h"
#include "sd_store_send.h"
#include "data_rec.h"
#include "lock_state.h"
#include "my_RTC.h"
#include "printer_driver.h"

//timestruction_for_wave    time_for_wave,time_for_data;
uint16_t length_index=0;
extern uint8_t bat_quantity;
extern float temp[6];
extern float humi[6];
extern uint16_t moter_speed;
extern GPS_Parser_t gps_parser;
extern uint8_t lock1_state;
extern uint8_t lock2_state;
extern uint8_t admin_code_data[6];
extern uint8_t lock1_code_data[6];
extern char *strx; 	//·µ»ØÖµÖ¸ÕëÅÐ¶Ï
extern char at_response_buffer[AT_RESPONSE_BUF_SIZE];
extern uint16_t fengshan1_speed;
extern uint16_t fengshan2_speed;
extern uint8_t _4g_is_connecting;  // 连接中标志
extern float battery_soc;
extern uint16_t last_hour;
extern uint16_t last_min;
extern uint16_t heat_data;
//打印时间指令
extern uint32_t start_year;
extern uint32_t start_month;
extern uint32_t start_day;
extern uint32_t start_hour;
extern uint32_t start_min;
extern uint32_t end_year;
extern uint32_t end_month;
extern uint32_t end_day;
extern uint32_t end_hour;
extern uint32_t end_min;

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

extern uint32_t cold_data;
extern int16_t signal_rsrp;      // RSRP 值（dBm）
extern uint16_t signal_mcc;      // MCC
extern uint8_t  signal_mnc;      // MNC
extern uint32_t signal_tac;      // TAC（十六进制）
extern uint32_t signal_cellid;   // Cell ID（十六进制）
extern uint8_t print_KO;         // 打印完成标志（用于自动关打印机）
extern uint8_t print_state;      // 打印状态标志（用于触发自动关机）
extern uint32_t PCM_state;
extern uint32_t Car_data;
extern uint8_t start_state;
extern uint16_t coda_value;

extern uint16_t RTC_year;
extern uint8_t RTC_month;
extern uint8_t RTC_day;
extern uint8_t RTC_hour;
extern uint8_t RTC_min;
extern uint8_t RTC_second;

// 字节数组转换为十六进制字符串函数
void BytesToHexString(uint8_t *bytes, uint16_t len, char *hexstr)
{
    const char hex_chars[] = "0123456789ABCDEF";

    for(uint16_t i = 0; i < len; i++)
    {
        hexstr[i*2] = hex_chars[(bytes[i] >> 4) & 0x0F];
        hexstr[i*2 + 1] = hex_chars[bytes[i] & 0x0F];
    }
    hexstr[len*2] = '\0';  // 字符串结束符
}
// 直吐模式发送十六进制数据函数
void EC800Send_HexData(uint8_t *bufferdata, uint16_t len)
{
    char hexstr[512];  // 足够大的缓冲区存储十六进制字符串
    char at_cmd[600];  // AT命令缓冲区

    // 将字节数组转换为十六进制字符串
    BytesToHexString(bufferdata, len, hexstr);

    // 构建AT命令
    snprintf(at_cmd, sizeof(at_cmd), "AT+QISENDEX=0,\"%s\"\r\n", hexstr);

    // 发送AT命令
    AT_SendCommand(at_cmd);
    vTaskDelay(100);

    // 等待OK响应
    strx = strstr((char*)at_response_buffer, "OK");
    while(strx == NULL)
    {
        strx = strstr((char*)at_response_buffer, "OK");
        vTaskDelay(10);
    }

    vTaskDelay(100);
    Clear_Buffer();
}

// 修改后的心跳包发送函数
void heart_break_send(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x80;

    buff[i++] = (uint8_t)(temp_high+100);
    buff[i++] = (uint8_t)(temp_low+100);
    buff[i++] = (uint8_t)(warn_high+100);
    buff[i++] = (uint8_t)(warn_low+100);
    buff[i++] = (uint8_t)(PCM_state);
    buff[i++] = (uint8_t)(Car_data);
    buff[i++] = (uint8_t)(cold_data);
    buff[i++] = (uint8_t)(battery_warn_low);
    buff[i++] = (uint8_t)(battery_alarm_low);
    buff[i++] = (uint8_t)(keepwarn_low);
    buff[i++] = (uint8_t)(keepwarn_serious_low);
    buff[i++] = (uint8_t)(start_state);

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}


// 修改后的wind_temp_send函数
static uint32_t last_wind_temp_tick = 0;
static uint32_t print_log_save_counter = 0;  // 打印日志清理计数器

void wind_temp_send(void)
{
    uint32_t now = xTaskGetTickCount();
    // 防止短时间内重复发送（3秒内只允许发一次）
    if (now - last_wind_temp_tick < pdMS_TO_TICKS(3000)) {
        return;
    }
    last_wind_temp_tick = now;

    uint8_t buff[200];
    uint8_t changebuf[4];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x81;

    buff[i++] = (uint8_t)(RTC_year >> 8);
    buff[i++] = (uint8_t)RTC_year;
    buff[i++] = RTC_month;
    buff[i++] = RTC_day;
    buff[i++] = RTC_hour;
    buff[i++] = RTC_min;
    buff[i++] = RTC_second;

    buff[i++] = (uint8_t)battery_soc;

    for(int j = 0; j < 6; j++) {
        memcpy(changebuf, &temp[j], 4);
        buff[i++] = changebuf[3];
        buff[i++] = changebuf[2];
        buff[i++] = changebuf[1];
        buff[i++] = changebuf[0];
    }

    for(int j = 0; j < 6; j++) {
        memcpy(changebuf, &humi[j], 4);
        buff[i++] = changebuf[3];
        buff[i++] = changebuf[2];
        buff[i++] = changebuf[1];
        buff[i++] = changebuf[0];
    }

    buff[i++] = (uint8_t)(fengshan1_speed>>8);
    buff[i++] = (uint8_t)fengshan1_speed;
    buff[i++] = (uint8_t)(fengshan2_speed>>8);
    buff[i++] = (uint8_t)fengshan2_speed;

    // MCC（uint16, 2字节, 大端）
    buff[i++] = (uint8_t)(signal_mcc >> 8);
    buff[i++] = (uint8_t)signal_mcc;
    // MNC（uint8, 1字节）
    buff[i++] = (uint8_t)signal_mnc;
    // TAC（uint32, 4字节, 大端）
    buff[i++] = (uint8_t)(signal_tac >> 24);
    buff[i++] = (uint8_t)(signal_tac >> 16);
    buff[i++] = (uint8_t)(signal_tac >> 8);
    buff[i++] = (uint8_t)signal_tac;
    // Cell ID（uint32, 4字节, 大端）
    buff[i++] = (uint8_t)(signal_cellid >> 24);
    buff[i++] = (uint8_t)(signal_cellid >> 16);
    buff[i++] = (uint8_t)(signal_cellid >> 8);
    buff[i++] = (uint8_t)signal_cellid;

    buff[i++]=0x01;//数据续传标志位，后续会进行更改

    buff[i++] = read_lock1;   // 门1实际状态：read_lock=0(关闭)→上报1，read_lock=1(打开)→上报0
    buff[i++] = read_lock2;   // 门2实际状态

    if(last_hour>=255)
    {
    	last_hour=255;
    }
    buff[i++]=(uint8_t)last_hour;
    buff[i++]=last_min;
   buff[i++]=(uint8_t)((coda_value)>>8);
   buff[i++]=(uint8_t)coda_value;
   buff[i++]=(uint8_t)(heat_data>>8);
   buff[i++]=(uint8_t)heat_data;
   buff[i++]=(uint8_t)((signal_rsrp+200)>>8);
   buff[i++]=(uint8_t)(signal_rsrp+200);

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    // 时间合法时存储
    if(is_valid_datetime(RTC_year, RTC_month, RTC_day, RTC_hour, RTC_min, RTC_second)) {
        // 1. 始终存打印日志（plog.dat，保留7天，用于打印）
        if(SD_Storage_SavePrintLog(buff, length_index,
                RTC_year, RTC_month, RTC_day,
                RTC_hour, RTC_min, RTC_second) == SD_STORAGE_OK) {
            // 每100次存储才清理一次，避免频繁SD卡操作阻塞其他任务
            print_log_save_counter++;
            if(print_log_save_counter >= 100) {
                print_log_save_counter = 0;
                SD_Storage_Cleanup7DaysPrintLog();
            }
        }
        // 2. 4G未连接时存离线补传数据（offline.dat，连接后自动发送并删除）
        if(_4g_is_connecting != 0) {
            SD_Storage_SavePacketWithTime(buff, length_index,
                RTC_year, RTC_month, RTC_day,
                RTC_hour, RTC_min, RTC_second);
        }
    } else {
        printf("[存储] RTC时间非法，跳过存储\n");
    }

    // 4G连接正常时实时上报平台
    if(_4g_is_connecting == 0) {
        EC800Send_HexData(buff, length_index);
        printf("[发送] 正常发送数据包\n");
    }
}

// 修改后的报警数据上报函数
void alarm_data(uint8_t alarm_type)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x82;
    buff[i++] = alarm_type;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的故障上报函数
void Fault_data(uint8_t Fault_type)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x83;
    buff[i++] = Fault_type;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的回复锁状态函数
void lock_data(uint8_t lock_add)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x84;
    buff[i++] = lock_add;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的温度设定函数
void temp_set(float set_temp_data)
{
    uint8_t buff[50];
    uint8_t changebuf[4];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x87;

    memcpy(changebuf, &set_temp_data, 4);
    buff[i++] = changebuf[3];
    buff[i++] = changebuf[2];
    buff[i++] = changebuf[1];
    buff[i++] = changebuf[0];

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的密码确认函数
void code_OK(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x88;

    buff[i++] = 0x01;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的温度确认函数
void temp_set_OK(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x87;

    buff[i++] = 0x01;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的温度确认函数
void alarm_set_OK(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x8D;

    buff[i++] = 0x01;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的管理员密码查询函数
void code_admin_ask(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x8B;

    buff[i++] = 0x00;
    memcpy(&buff[i], admin_code_data, 6);
    i += 6;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的锁密码查询函数
void code_lock_ask(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x8B;

    buff[i++] = 0x01;
    memcpy(&buff[i], lock1_code_data, 6);
    i += 6;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

void music_play(void)
{
    uint8_t buff[50];
    memset(buff,0,sizeof(50));
	uint16_t i=0;
	buff[i++]=0xaa;
	buff[i++]=0x07;
	buff[i++]=0x02;
	buff[i++]=0x00;     //数据包长度
	buff[i++]=0x01;
	buff[i++]=0xB4;
	HAL_UART_Transmit(&huart3,buff,6,1000);
}

void music2_play(void)
{
    uint8_t buff[50];
    memset(buff,0,sizeof(50));
	uint16_t i=0;
	buff[i++]=0xaa;
	buff[i++]=0x07;
	buff[i++]=0x02;
	buff[i++]=0x00;     //数据包长度
	buff[i++]=0x02;
	buff[i++]=0xB5;
	HAL_UART_Transmit(&huart3,buff,6,1000);
}

void music3_play(void)
{
    uint8_t buff[50];
    memset(buff,0,sizeof(50));
	uint16_t i=0;
	buff[i++]=0xaa;
	buff[i++]=0x07;
	buff[i++]=0x02;
	buff[i++]=0x00;     //数据包长度
	buff[i++]=0x06;
	buff[i++]=0xB9;
	HAL_UART_Transmit(&huart3,buff,6,1000);
}

void music_time(void)
{
    uint8_t buff[50];
    memset(buff,0,sizeof(50));
	uint16_t i=0;
	buff[i++]=0xaa;
	buff[i++]=0x07;
	buff[i++]=0x02;
	buff[i++]=0x00;     //数据包长度
	buff[i++]=0x03;
	buff[i++]=0xB6;
	HAL_UART_Transmit(&huart3,buff,6,1000);
}

void music_bat(void)
{
    uint8_t buff[50];
    memset(buff,0,sizeof(50));
	uint16_t i=0;
	buff[i++]=0xaa;
	buff[i++]=0x07;
	buff[i++]=0x02;
	buff[i++]=0x00;     //数据包长度
	buff[i++]=0x04;
	buff[i++]=0xB7;
	HAL_UART_Transmit(&huart3,buff,6,1000);
}

void music_temp(void)
{
    uint8_t buff[50];
    memset(buff,0,sizeof(50));
	uint16_t i=0;
	buff[i++]=0xaa;
	buff[i++]=0x07;
	buff[i++]=0x02;
	buff[i++]=0x00;     //数据包长度
	buff[i++]=0x05;
	buff[i++]=0xB8;
	HAL_UART_Transmit(&huart3,buff,6,1000);
}

// 修改后的温度确认函数
void music_temp_OK(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x8E;

    buff[i++] = 0x01;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 修改后的温度确认函数
void music_time_OK(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x8F;

    buff[i++] = 0x01;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

void time_updata(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x90;
    buff[i++] = (uint8_t)(start_year >> 8);
    buff[i++] = (uint8_t)start_year;
    buff[i++] = (uint8_t)start_month;
    buff[i++] = (uint8_t)start_day;
    buff[i++] = (uint8_t)start_hour;
    buff[i++] = (uint8_t)start_min;
    buff[i++] = (uint8_t)(end_year >> 8);
    buff[i++] = (uint8_t)end_year;
    buff[i++] = (uint8_t)end_month;
    buff[i++] = (uint8_t)end_day;
    buff[i++] = (uint8_t)end_hour;
    buff[i++] = (uint8_t)end_min;

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 从SD卡读取数据并打印
// 打印内容：时间 温度1 湿度1 温度2 湿度2
// 错误处理：时间非法、时间范围超过7天
void print_from_sd(void)
{
    uint8_t buf[120];
    uint32_t i;
    uint32_t total_count;
    WindDataPacket_t packet;
    uint16_t prev_year = 0;
    uint8_t  prev_month = 0, prev_day = 0;
    uint8_t  has_data = 0;
    uint32_t start_sec, end_sec, pkt_sec;

    // 1. 校验起始时间合法性
    if(!is_valid_datetime((uint16_t)start_year, (uint8_t)start_month, (uint8_t)start_day,
                          (uint8_t)start_hour, (uint8_t)start_min, 0)) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);  // 打开打印机
        vTaskDelay(500);
        InitializePrint();
        // "打印失败开始时间错误"
        Print_ASCII((uint8_t*)"\xB4\xF2\xD3\xA1\xCA\xA7\xB0\xDC\xBF\xAA\xCA\xBC\xCA\xB1\xBC\xE4\xB4\xED\xCE\xF3");
        select_lines(2);
        print_state = 1;
        print_send();
        return;
    }

    // 2. 校验结束时间合法性
    if(!is_valid_datetime((uint16_t)end_year, (uint8_t)end_month, (uint8_t)end_day,
                          (uint8_t)end_hour, (uint8_t)end_min, 0)) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
        vTaskDelay(500);
        InitializePrint();
        // "打印失败结束时间错误"
        Print_ASCII((uint8_t*)"\xB4\xF2\xD3\xA1\xCA\xA7\xB0\xDC\xBD\xE1\xCA\xF8\xCA\xB1\xBC\xE4\xB4\xEd\xCE\xF3");
        select_lines(2);
        print_state = 1;
        print_send();
        return;
    }

    // 3. 计算时间范围
    start_sec = DateTimeToSeconds((uint16_t)start_year, (uint8_t)start_month, (uint8_t)start_day,
                                   (uint8_t)start_hour, (uint8_t)start_min, 0);
    end_sec = DateTimeToSeconds((uint16_t)end_year, (uint8_t)end_month, (uint8_t)end_day,
                                 (uint8_t)end_hour, (uint8_t)end_min, 0);

    if(end_sec <= start_sec) {
        // 结束时间不晚于开始时间
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
        vTaskDelay(500);
        InitializePrint();
        // "打印失败结束时间错误"
        Print_ASCII((uint8_t*)"\xB4\xF2\xD3\xA1\xCA\xA7\xB0\xDC\xBD\xE1\xCA\xF8\xCA\xB1\xBC\xE4\xB4\xEd\xCE\xF3");
        select_lines(2);
        print_state = 1;
        print_send();
        return;
    }

    // 4. 检查时间范围是否超过7天（604800秒）
    if((end_sec - start_sec) > 604800) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
        vTaskDelay(500);
        InitializePrint();
        // "打印失败打印数据超过7天时间"
        Print_ASCII((uint8_t*)"\xB4\xF2\xD3\xA1\xCA\xA7\xB0\xDC\xB4\xF2\xD3\xA1\xCA\xFD\xBE\xDD\xB3\xAC\xB9\xFD\x37\xCC\xEC\xCA\xB1\xBC\xE4");
        select_lines(2);
        print_state = 1;
        print_send();
        return;
    }

    // 5. 打开打印机电源
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
    vTaskDelay(500);

    // 6. 打印标题信息
    InitializePrint();
    // "打印单号:60103101"
    Print_ASCII((uint8_t*)"\xB4\xF2\xD3\xA1\xB5\xA5\xBA\xC5\x3A\x36\x30\x31\x30\x33\x31\x30\x31");

    // "开始时间:YYYY-MM-DD HH:MM"
    sprintf((char*)buf, "\xBF\xAA\xCA\xBC\xCA\xB1\xBC\xE4\x3A%ld-%ld-%ld %ld:%ld",
            (long)start_year, (long)start_month, (long)start_day,
            (long)start_hour, (long)start_min);
    init_putstr(buf, 0);

    // "结束时间:YYYY-MM-DD HH:MM"
    sprintf((char*)buf, "\xBD\xE1\xCA\xF8\xCA\xB1\xBC\xE4\x3A%ld-%ld-%ld %ld:%ld",
            (long)end_year, (long)end_month, (long)end_day,
            (long)end_hour, (long)end_min);
    init_putstr(buf, 0);

    // "时间  温度1  湿度1  温度2  湿度2"
    init_putstr((uint8_t*)"\xCA\xB1\xBC\xE4\x20\x20\xCE\xC2\xB6\xC8\x31\x20\x20\xCA\xAA\xB6\xC8\x31\x20\x20\xCE\xC2\xB6\xC8\x32\x20\x20\xCA\xAA\xB6\xC8\x32", 1);

    // 7. 遍历打印日志数据包，筛选时间范围内的数据并打印
    total_count = SD_Storage_GetPrintLogCount();
    for(i = 0; i < total_count; i++) {
        if(SD_Storage_ReadPrintLog(i, &packet) != SD_STORAGE_OK) continue;

        // 跳过时间非法的数据包
        if(!is_valid_datetime(packet.year, packet.month, packet.day,
                              packet.hour, packet.minute, packet.second)) continue;

        // 检查是否在打印时间范围内
        pkt_sec = DateTimeToSeconds(packet.year, packet.month, packet.day,
                                     packet.hour, packet.minute, packet.second);
        if(pkt_sec < start_sec || pkt_sec > end_sec) continue;

        // 检查数据包长度是否足够（至少需要到humi[1]的末尾，即偏移52）
        if(packet.length < 52) continue;

        // 日期变化时打印日期行
        if(packet.year != prev_year || packet.month != prev_month || packet.day != prev_day) {
            sprintf((char*)buf, "%d-%d-%d", packet.year, packet.month, packet.day);
            init_putstr(buf, 0);
            select_lines(1);
            prev_year = packet.year;
            prev_month = packet.month;
            prev_day = packet.day;
        }

        // 从数据包解析温度1、温度2、湿度1、湿度2（大端float转小端）
        // data[20-23]: temp[0] 温度1
        // data[24-27]: temp[1] 温度2
        // data[44-47]: humi[0] 湿度1
        // data[48-51]: humi[1] 湿度2
        uint8_t changebuf[4];
        float temp1, temp2, humi1, humi2;

        changebuf[0] = packet.data[23];
        changebuf[1] = packet.data[22];
        changebuf[2] = packet.data[21];
        changebuf[3] = packet.data[20];
        memcpy(&temp1, changebuf, 4);

        changebuf[0] = packet.data[27];
        changebuf[1] = packet.data[26];
        changebuf[2] = packet.data[25];
        changebuf[3] = packet.data[24];
        memcpy(&temp2, changebuf, 4);

        changebuf[0] = packet.data[47];
        changebuf[1] = packet.data[46];
        changebuf[2] = packet.data[45];
        changebuf[3] = packet.data[44];
        memcpy(&humi1, changebuf, 4);

        changebuf[0] = packet.data[51];
        changebuf[1] = packet.data[50];
        changebuf[2] = packet.data[49];
        changebuf[3] = packet.data[48];
        memcpy(&humi2, changebuf, 4);

        // 转换为整数*10格式打印（避免使用%f）
        int16_t t1_10 = (int16_t)(temp1 * 10);
        int16_t t1_int = t1_10 / 10;
        int16_t t1_frac = t1_10 % 10;
        if(t1_frac < 0) t1_frac = -t1_frac;

        int16_t t2_10 = (int16_t)(temp2 * 10);
        int16_t t2_int = t2_10 / 10;
        int16_t t2_frac = t2_10 % 10;
        if(t2_frac < 0) t2_frac = -t2_frac;

        sprintf((char*)buf, "%02d:%02d   %d.%d   %d   %d.%d   %d",
                packet.hour, packet.minute,
                t1_int, t1_frac, (int)humi1,
                t2_int, t2_frac, (int)humi2);
        init_putstr(buf, 0);
        select_lines(1);
        vTaskDelay(2);
        has_data = 1;
    }

    // 8. 无数据时打印提示
    if(!has_data) {
        // "无打印数据"
        init_putstr((uint8_t*)"\xCE\xDE\xB4\xF2\xD3\xA1\xCA\xFD\xBE\xDD", 0);
        select_lines(1);
    }

    select_lines(2);  // 走纸

    // 9. 设置打印状态，触发18秒后自动关打印机
    print_state = 1;
    print_send();
}

// 打印数据确认函数
void print_OK(uint8_t print_cnt)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    buff[i++] = 0xA5;
    buff[i++] = 0x5A;
    buff[i++] = 0x00;
    buff[i++] = 0x01;
    buff[i++] = 0x90;
    buff[i++] = print_cnt;

    length_index = i;
    buff[2] = length_index - 1;
    buff[i++] = XOR(buff, length_index - 3, 3);
    buff[i++] = CheckSum8(buff, length_index - 3, 3);

    length_index = i;

    HAL_UART_Transmit(&huart6, (uint8_t*)buff, length_index, 100);
}

// 开门函数
void door_open(uint8_t door_data,uint8_t door_state)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x92;
    buff[i++] = door_data;
    buff[i++] = door_state;
    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 设备启动状态反馈
void shebei_open(uint8_t load_dir,uint8_t shebei_state)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x93;
    buff[i++] = load_dir;
    buff[i++] = shebei_state;
    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

void xiangti_data(uint8_t PCM_data,uint8_t cheti_data,uint8_t lengye_data)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x95;
    buff[i++] = PCM_data;
    buff[i++] = cheti_data;
    buff[i++] = lengye_data;
    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 设备参数设置上报
void start_OK(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x93;
    buff[i++] = 0x01;
    buff[i++] = 0xF0;
    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 设备参数设置上报
void vision_send(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x89;

    buff[i++] = (uint8_t)(Soft_version);

    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}
// 回复设置命令
void zhileng_OK(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x95;
    buff[i++] = 0x01;
    buff[i++] = 0xF0;
    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 回复设置命令
void init_send(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    write_pkt_header(buff, &i);
    buff[i++] = 0x91;
    length_index = i;
    write_pkt_checksum(buff, length_index, &i);
    length_index = i;

    EC800Send_HexData(buff, length_index);
}

// 回复打印命令
void print_send(void)
{
    uint8_t buff[50];
    memset(buff, 0, sizeof(buff));
    uint16_t i = 0;

    buff[i++] = 0x1C;
    buff[i++] = 0x76;

    length_index = i;

    // 使用直吐模式发送数据
	HAL_UART_Transmit(&huart1,buff,length_index,1000);
}
