/**
  * @file    gps_parser.h
  * @brief   GPS数据解析模块头文件
  * @author  STM32开发者
  * @date    2024
  */

#ifndef __GPS_PARSER_H
#define __GPS_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define GPS_RX_BUFFER_SIZE 600
// GPS数据结构定义
typedef struct {
    // 时间信息
    struct {
        uint16_t year;          // 年份，如2024
        uint8_t month;          // 月份，1-12
        uint8_t day;            // 日，1-31
        uint8_t utc_hour;       // UTC小时，0-23
        uint8_t utc_minute;     // UTC分钟，0-59
        float utc_second;       // UTC秒，0-59.999
        uint8_t local_hour;     // 本地时间（北京）小时，0-23
        uint8_t local_minute;   // 本地时间分钟，0-59
        float local_second;     // 本地时间秒，0-59.999
    } time;
    
    // 位置信息
    struct {
        float latitude;         // 纬度，十进制度
        float longitude;        // 经度，十进制度
        char latitude_direction; // 纬度方向，N/S
        char longitude_direction; // 经度方向，E/W
        char lat_raw[15];       // 原始纬度字符串
        char lon_raw[15];       // 原始经度字符串
    } location;
    
    // 其他信息
    float altitude;         // 海拔高度，米
    float speed;            // 速度，千米/小时
    float course;           // 航向，度
    float hdop;             // 水平精度因子
    uint8_t satellites;     // 卫星数量，0-12
    uint8_t fix_status;     // 定位状态，0=无效，1=有效
    
    // 原始数据
    char raw_data[256];     // 原始GPS字符串
    uint32_t timestamp;     // 数据接收时间戳（系统tick）
    
} GPS_Data_t;

// GPS解析器状态
typedef struct {
    GPS_Data_t data;        // 当前GPS数据
    bool data_valid;        // 数据是否有效
    uint32_t parse_count;   // 解析次数
    uint32_t error_count;   // 错误次数
    uint32_t last_update;   // 最后更新时间
} GPS_Parser_t;

// 函数声明
void GPS_Parser_Init(void);
bool GPS_Parse_String(char* gps_string);
bool GPS_Get_Data(GPS_Data_t* data);
bool GPS_Is_Data_Valid(void);
void GPS_Reset_Data(void);
uint8_t GPS_Get_Satellite_Count(void);
float GPS_Get_Latitude(void);
float GPS_Get_Longitude(void);
float GPS_Get_Altitude(void);
void GPS_Get_Time(uint8_t* hour, uint8_t* minute, float* second);
void GPS_Get_Date(uint16_t* year, uint8_t* month, uint8_t* day);
void GPS_Data_To_String(char* buffer, uint32_t size);
const char* GPS_Get_Raw_Data(void);
uint32_t GPS_Get_Timestamp(void);
float GPS_Get_Speed(void);
float GPS_Get_Course(void);
void upload_gps_data(void);
void process_gps_data(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPS_PARSER_H */
