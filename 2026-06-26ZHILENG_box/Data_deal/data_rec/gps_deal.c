/**
  * @file    gps_parser.c
  * @brief   GPS数据解析模块
  * @author  STM32开发者
  * @date    2024
  */

#include "gps_deal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"

// GPS解析器实例
GPS_Parser_t gps_parser;

// GPS接收缓冲区
uint8_t gps_rx_buffer[GPS_RX_BUFFER_SIZE];
 bool gps_data_ready = false;
 uint16_t gps_data_length = 0;

// 打印缓冲区
char print_buffer[300];

// 内部函数声明
static bool parse_utc_time(const char* time_str, const char* date_str);
static bool parse_location(const char* lat_str, const char* lon_str);
static float dm_to_degree(const char* dm_format);
static void clean_string(char* str);
static bool is_valid_gps_data(void);



/**
  * @brief  解析逗号分隔的字符串，处理空字段
  * @param  str: 输入字符串
  * @param  tokens: 输出token数组
  * @param  max_tokens: 最大token数量
  * @param  delimiter: 分隔符（默认为逗号）
  * @retval 找到的token数量
  */
int parse_csv(char* str, char* tokens[], int max_tokens, char delimiter) {
    if (str == NULL || tokens == NULL || max_tokens <= 0) {
        return 0;
    }

    int count = 0;
    char* start = str;
    char* end = NULL;

    while (count < max_tokens && *start != '\0') {
        // 查找下一个分隔符
        end = strchr(start, delimiter);

        if (end == NULL) {
            // 最后一个字段
            tokens[count++] = start;
            break;
        } else {
            // 保存当前字段
            *end = '\0';  // 临时截断
            tokens[count++] = start;

            // 移动到下一个字段
            start = end + 1;
        }
    }

    // 如果还有剩余字段但已达到最大数量，确保字符串恢复
    if (end != NULL && count < max_tokens) {
        *end = delimiter;  // 恢复分隔符
    }

    return count;
}
/**
  * @brief  初始化GPS解析器
  */
void GPS_Parser_Init(void) {
    memset(&gps_parser, 0, sizeof(GPS_Parser_t));
    gps_parser.data_valid = false;
    gps_parser.parse_count = 0;
    gps_parser.error_count = 0;
    gps_parser.last_update = 0;
}

/**
  * @brief  清理字符串，移除换行符和回车符
  * @param  str: 要清理的字符串
  */
static void clean_string(char* str) {
    if (str == NULL) return;
    
    // 移除所有 \r 和 \n 字符，保留逗号和其他字符
    char* src = str;
    char* dst = str;
    
    while (*src) {
        if (*src != '\r' && *src != '\n') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';

    // 去掉首尾空格
    // 去掉开头的空格
    while (*str == ' ') {
        str++;
    }

    // 去掉结尾的空格
    char* end = str + strlen(str) - 1;
    while (end > str && *end == ' ') {
        *end = '\0';
        end--;
    }
}

/**
  * @brief  将度分格式转换为十进制度
  * @param  dm_format: 度分格式字符串，如"2232.0787"
  * @retval 十进制度数
  */
static float dm_to_degree(const char* dm_format) {
    if (dm_format == NULL || strlen(dm_format) == 0) {
        return 0.0f;
    }
    
    char temp[20];
    strncpy(temp, dm_format, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    float value = atof(temp);
    
    // 区分纬度和经度
    if (strlen(dm_format) <= 9) { // 纬度格式 ddmm.mmmm
        int degrees = (int)(value / 100);
        float minutes = value - degrees * 100.0f;
        return degrees + minutes / 60.0f;
    } else { // 经度格式 dddmm.mmmm
        int degrees = (int)(value / 100);
        float minutes = value - degrees * 100.0f;
        return degrees + minutes / 60.0f;
    }
}

/**
  * @brief  解析UTC时间和日期，转换为北京时间（UTC+8）
  * @param  time_str: 时间字符串，如"061753.0"
  * @param  date_str: 日期字符串，如"170518"
  * @retval 解析是否成功
  */
static bool parse_utc_time(const char* time_str, const char* date_str) {
    if (time_str == NULL || date_str == NULL) {
        return false;
    }
    
    if (strlen(time_str) < 6 || strlen(date_str) != 6) {
        return false;
    }
    
    char hour_str[3] = {0};
    char min_str[3] = {0};
    char sec_str[10] = {0};
    char day_str[3] = {0};
    char month_str[3] = {0};
    char year_str[3] = {0};
    
    // 解析UTC时间 HHMMSS.ss
    strncpy(hour_str, time_str, 2);
    strncpy(min_str, time_str + 2, 2);
    strncpy(sec_str, time_str + 4, sizeof(sec_str) - 1);
    
    // 解析UTC日期 DDMMYY
    strncpy(day_str, date_str, 2);
    strncpy(month_str, date_str + 2, 2);
    strncpy(year_str, date_str + 4, 2);
    
    // 转换为数值（UTC）
    int utc_hour = atoi(hour_str);
    int utc_minute = atoi(min_str);
    double utc_second = atof(sec_str);
    
    int day = atoi(day_str);
    int month = atoi(month_str);
    int year = 2000 + atoi(year_str);
    
    // 计算北京时间（UTC+8）
    int local_hour = utc_hour + 8;
    int local_minute = utc_minute;
    double local_second = utc_second;
    
    // 处理日期跨日
    if (local_hour >= 24) {
        local_hour -= 24;

        // 日期加一天
        day++;

        // 获取当前月份的天数（考虑闰年）
        int days_in_month;
        switch (month) {
            case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                days_in_month = 31;
                break;
            case 4: case 6: case 9: case 11:
                days_in_month = 30;
                break;
            case 2:
                // 闰年判断
                if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
                    days_in_month = 29;
                } else {
                    days_in_month = 28;
                }
                break;
            default:
                days_in_month = 30; // 不会发生
                break;
        }

        if (day > days_in_month) {
            day = 1;
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
        }
    }
    
    // 存储结果到全局结构体（北京时间）
    gps_parser.data.time.year = year;
    gps_parser.data.time.month = month;
    gps_parser.data.time.day = day;
    gps_parser.data.time.local_hour = local_hour;
    gps_parser.data.time.local_minute = local_minute;
    gps_parser.data.time.local_second = local_second;

    // 可选：如果需要保留UTC时间，可以额外存储，但现有结构未提供字段
    // 根据实际需求，也可以将UTC时间存入其他字段

    return true;
}

/**
  * @brief  解析经纬度
  * @param  lat_str: 纬度字符串，如"2232.0787N"
  * @param  lon_str: 经度字符串，如"11355.5482E"
  * @retval 解析是否成功
  */
static bool parse_location(const char* lat_str, const char* lon_str) {
    if (lat_str == NULL || lon_str == NULL) {
        return false;
    }
    
    if (strlen(lat_str) < 8 || strlen(lon_str) < 9) {
        return false;
    }
    
    // 保存原始字符串
    strncpy(gps_parser.data.location.lat_raw, lat_str, 
            sizeof(gps_parser.data.location.lat_raw) - 1);
    strncpy(gps_parser.data.location.lon_raw, lon_str, 
            sizeof(gps_parser.data.location.lon_raw) - 1);
    
    // 提取纬度和经度的数值部分
    char lat_value[15] = {0};
    char lon_value[15] = {0};
    
    strncpy(lat_value, lat_str, strlen(lat_str) - 1);
    strncpy(lon_value, lon_str, strlen(lon_str) - 1);
    
    // 提取方向
    gps_parser.data.location.latitude_direction = lat_str[strlen(lat_str) - 1];
    gps_parser.data.location.longitude_direction = lon_str[strlen(lon_str) - 1];
    
    // 转换为十进制度
    gps_parser.data.location.latitude = dm_to_degree(lat_value);
    gps_parser.data.location.longitude = dm_to_degree(lon_value);
    
    // 如果方向是南纬或西经，取负值
    if (gps_parser.data.location.latitude_direction == 'S') {
        gps_parser.data.location.latitude = -gps_parser.data.location.latitude;
    }
    
    if (gps_parser.data.location.longitude_direction == 'W') {
        gps_parser.data.location.longitude = -gps_parser.data.location.longitude;
    }
    
    return true;
}

/**
  * @brief  检查GPS数据是否有效
  * @retval 数据是否有效
  */
static bool is_valid_gps_data(void) {
    // 检查卫星数量
    if (gps_parser.data.satellites < 3) {
        printf("Invalid: satellites < 3 (%d)\r\n", gps_parser.data.satellites);
        return false;
    }
    
    // 检查经纬度是否在合理范围内
    if (fabs(gps_parser.data.location.latitude) > 90.0f ||
        fabs(gps_parser.data.location.longitude) > 180.0f) {
        printf("Invalid: lat/lon out of range (%.6f, %.6f)\r\n",
               gps_parser.data.location.latitude, gps_parser.data.location.longitude);
        return false;
    }
    
    // 检查时间是否合理
    if (gps_parser.data.time.utc_hour > 23 ||
        gps_parser.data.time.utc_minute > 59 ||
        gps_parser.data.time.utc_second >= 60.0f) {
        printf("Invalid: time out of range (%02d:%02d:%05.2f)\r\n",
               gps_parser.data.time.utc_hour, gps_parser.data.time.utc_minute,
               gps_parser.data.time.utc_second);
        return false;
    }
    
    // 检查日期是否合理
    if (gps_parser.data.time.day < 1 || gps_parser.data.time.day > 31 ||
        gps_parser.data.time.month < 1 || gps_parser.data.time.month > 12) {
        printf("Invalid: date out of range (%04d-%02d-%02d)\r\n",
               gps_parser.data.time.year, gps_parser.data.time.month, gps_parser.data.time.day);
        return false;
    }

    return true;
}

/**
  * @brief  解析GPS数据字符串
  * @param  gps_string: GPS原始字符串
  * @retval 解析是否成功
  */
bool GPS_Parse_String(char* gps_string) {
    if (gps_string == NULL) {
        gps_parser.error_count++;
        return false;
    }
    
    // 先打印原始数据用于调试
    printf("Parsing: '%s'\r\n", gps_string);

    // 检查是否为错误信息
    if (strstr(gps_string, "+CME ERROR") != NULL) {
        gps_parser.data_valid = false;
        gps_parser.data.fix_status = 0;
        gps_parser.error_count++;
        printf("GPS Error received\r\n");
        return false;
    }
    
    // 检查是否为GPS数据
    char* gps_start = strstr(gps_string, "+QGPSLOC:");
    if (gps_start == NULL) {
        // 尝试其他可能的格式
        gps_start = strstr(gps_string, "QGPSLOC:");
        if (gps_start == NULL) {
            gps_parser.error_count++;
            printf("No GPS data found\r\n");
            return false;
        }
    }
    
    // 保存原始数据
    strncpy(gps_parser.data.raw_data, gps_string, 
            sizeof(gps_parser.data.raw_data) - 1);
    gps_parser.data.raw_data[sizeof(gps_parser.data.raw_data) - 1] = '\0';

    // 清理原始数据（移除换行符）
    clean_string(gps_parser.data.raw_data);
    
    // 设置时间戳
    gps_parser.data.timestamp = HAL_GetTick();
    
    // 跳过"+QGPSLOC:"
    gps_start += 9;

    // 跳过可能的空格
    while (*gps_start == ' ') gps_start++;
    
    // 复制到临时缓冲区
    char buffer[200];
    strncpy(buffer, gps_start, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    printf("Data to parse: '%s'\r\n", buffer);

    // 使用新的函数解析逗号分隔的字符串
    char* tokens[12];  // 最多12个字段
    int token_count = parse_csv(buffer, tokens, 12, ',');

    printf("Found %d tokens\r\n", token_count);
    
    // 打印所有token用于调试
    for (int i = 0; i < token_count; i++) {
        if (tokens[i] && tokens[i][0] != '\0') {
            printf("  [%d] = '%s'\r\n", i, tokens[i]);
        } else {
            printf("  [%d] = (empty)\r\n", i);
        }
    }
    
    // 检查是否有足够的token（至少需要11个，但第6个可能为空）
    if (token_count < 11) {
        printf("Not enough tokens: %d\r\n", token_count);
        gps_parser.error_count++;
        return false;
    }
    
    // 解析各个字段
    // 根据你的数据：+QGPSLOC: 061312.00,3029.7595N,11432.0940E,0.98,84.5,3,,0.128,0.070,050126,23
    // 索引：
    // 0: 时间 (061312.00)
    // 1: 纬度 (3029.7595N)
    // 2: 经度 (11432.0940E)
    // 3: 高度 (0.98)
    // 4: 速度 (84.5)
    // 5: 航向 (3)
    // 6: 精度因子 (可能是空)
    // 7: 垂直速度 (0.128)
    // 8: 水平速度 (0.070)
    // 9: 日期 (050126)
    // 10: 卫星数量 (23)
    
    // 解析时间和日期
    if (tokens[0] && tokens[9]) {
        if (!parse_utc_time(tokens[0], tokens[9])) {
            printf("Failed to parse time/date: %s, %s\r\n", tokens[0], tokens[9]);
            gps_parser.error_count++;
            return false;
        }
    } else {
        printf("Missing time or date field\r\n");
        gps_parser.error_count++;
        return false;
    }
    
    // 解析经纬度
    if (tokens[1] && tokens[2]) {
        if (!parse_location(tokens[1], tokens[2])) {
            printf("Failed to parse location: %s, %s\r\n", tokens[1], tokens[2]);
            gps_parser.error_count++;
            return false;
        }
    } else {
        printf("Missing latitude or longitude field\r\n");
        gps_parser.error_count++;
        return false;
    }
    
    // 解析其他字段（处理可能为空的字段）
    gps_parser.data.altitude = (tokens[3] && tokens[3][0] != '\0') ? atof(tokens[3]) : 0.0f;
    gps_parser.data.speed = (tokens[4] && tokens[4][0] != '\0') ? atof(tokens[4]) : 0.0f;
    gps_parser.data.course = (tokens[5] && tokens[5][0] != '\0') ? atof(tokens[5]) : 0.0f;
    gps_parser.data.hdop = (tokens[6] && tokens[6][0] != '\0') ? atof(tokens[6]) : 99.99f;  // 空值时设为很大
    gps_parser.data.satellites = (tokens[10] && tokens[10][0] != '\0') ? atoi(tokens[10]) : 0;

    // 根据数据中的第6个字段判断定位状态
    if (gps_parser.data.satellites >= 3 && gps_parser.data.hdop < 10.0f) {
        gps_parser.data.fix_status = 1;  // 定位有效
    } else {
        gps_parser.data.fix_status = 0;  // 定位无效
    }
    
    // 检查数据有效性
    if (is_valid_gps_data()) {
        gps_parser.data_valid = true;
        gps_parser.parse_count++;
        gps_parser.last_update = HAL_GetTick();
        printf("GPS parse successful!\r\n");
        return true;
    } else {
        gps_parser.data_valid = false;
        gps_parser.error_count++;
        printf("GPS data invalid\r\n");
        return false;
    }
}

/**
  * @brief  获取GPS数据
  * @param  data: 输出参数，存储GPS数据
  * @retval 数据是否有效
  */
bool GPS_Get_Data(GPS_Data_t* data) {
    if (data == NULL) {
        return false;
    }
    
    if (gps_parser.data_valid) {
        memcpy(data, &gps_parser.data, sizeof(GPS_Data_t));
        return true;
    }
    
    return false;
}

/**
  * @brief  检查GPS数据是否有效
  * @retval 数据是否有效
  */
bool GPS_Is_Data_Valid(void) {
    return gps_parser.data_valid;
}

/**
  * @brief  重置GPS数据
  */
void GPS_Reset_Data(void) {
    memset(&gps_parser.data, 0, sizeof(GPS_Data_t));
    gps_parser.data_valid = false;
}

/**
  * @brief  获取卫星数量
  * @retval 卫星数量
  */
uint8_t GPS_Get_Satellite_Count(void) {
    return gps_parser.data.satellites;
}

/**
  * @brief  获取纬度
  * @retval 纬度（十进制度）
  */
float GPS_Get_Latitude(void) {
    return gps_parser.data.location.latitude;
}

/**
  * @brief  获取经度
  * @retval 经度（十进制度）
  */
float GPS_Get_Longitude(void) {
    return gps_parser.data.location.longitude;
}

/**
  * @brief  获取海拔高度
  * @retval 海拔高度（米）
  */
float GPS_Get_Altitude(void) {
    return gps_parser.data.altitude;
}

/**
  * @brief  获取本地时间
  * @param  hour: 输出小时
  * @param  minute: 输出分钟
  * @param  second: 输出秒
  */
void GPS_Get_Time(uint8_t* hour, uint8_t* minute, float* second) {
    if (hour != NULL) *hour = gps_parser.data.time.local_hour;
    if (minute != NULL) *minute = gps_parser.data.time.local_minute;
    if (second != NULL) *second = gps_parser.data.time.local_second;
}

/**
  * @brief  获取日期
  * @param  year: 输出年份
  * @param  month: 输出月份
  * @param  day: 输出日
  */
void GPS_Get_Date(uint16_t* year, uint8_t* month, uint8_t* day) {
    if (year != NULL) *year = gps_parser.data.time.year;
    if (month != NULL) *month = gps_parser.data.time.month;
    if (day != NULL) *day = gps_parser.data.time.day;
}

/**
  * @brief  将GPS数据格式化为字符串
  * @param  buffer: 输出缓冲区
  * @param  size: 缓冲区大小
  */
void GPS_Data_To_String(char* buffer, uint32_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }
    
    if (!gps_parser.data_valid) {
        snprintf(buffer, size, "GPS Data: Not Available");
        return;
    }
    
    snprintf(buffer, size, 
        "Time: %04d-%02d-%02d %02d:%02d:%05.2f\r\n"
        "Location: %.6f°%c, %.6f°%c\r\n"
        "Altitude: %.1fm  Speed: %.1fkm/h  Course: %.1f°\r\n"
        "Satellites: %d  HDOP: %.2f  Status: %d\r\n"
        "Parse Count: %lu  Errors: %lu",
        gps_parser.data.time.year, gps_parser.data.time.month, gps_parser.data.time.day,
        gps_parser.data.time.local_hour, gps_parser.data.time.local_minute, 
        gps_parser.data.time.local_second,
        fabs(gps_parser.data.location.latitude), gps_parser.data.location.latitude_direction,
        fabs(gps_parser.data.location.longitude), gps_parser.data.location.longitude_direction,
        gps_parser.data.altitude, gps_parser.data.speed, gps_parser.data.course,
        gps_parser.data.satellites, gps_parser.data.hdop, gps_parser.data.fix_status,
        gps_parser.parse_count, gps_parser.error_count
    );
}

/**
  * @brief  获取原始GPS数据
  * @retval 原始GPS数据字符串
  */
const char* GPS_Get_Raw_Data(void) {
    return gps_parser.data.raw_data;
}

/**
  * @brief  获取数据时间戳
  * @retval 时间戳（系统tick）
  */
uint32_t GPS_Get_Timestamp(void) {
    return gps_parser.data.timestamp;
}

/**
  * @brief  获取速度
  * @retval 速度（千米/小时）
  */
float GPS_Get_Speed(void) {
    return gps_parser.data.speed;
}

/**
  * @brief  获取航向
  * @retval 航向（度）
  */
float GPS_Get_Course(void) {
    return gps_parser.data.course;
}

/**
  * @brief  处理接收到的GPS数据
  */
void process_gps_data(void) {
    if (gps_data_ready) {
        // 确保字符串以空字符结尾
        uint16_t data_len = gps_data_length;
        if (data_len >= GPS_RX_BUFFER_SIZE) {
            data_len = GPS_RX_BUFFER_SIZE - 1;
        }
        gps_rx_buffer[data_len] = '\0';

        // 转换为字符串
        char* gps_string = (char*)gps_rx_buffer;

        // 重要：不要在这里清理字符串，直接传递给解析函数
        // 解析函数会处理清理工作

        // 如果字符串不为空，尝试解析
        if (strlen(gps_string) > 0) {
            printf("\r\n=== Processing GPS Data ===\r\n");

            // 显示原始数据（用于调试）
            printf("Raw data (%d bytes): ", data_len);
            for (int i = 0; i < data_len && i < 80; i++) {
                if (gps_string[i] >= 32 && gps_string[i] <= 126) {
                    printf("%c", gps_string[i]);
                } else if (gps_string[i] == '\r') {
                    printf("\\r");
                } else if (gps_string[i] == '\n') {
                    printf("\\n");
                } else {
                    printf(".");
                }
            }
            printf("\r\n");

            // 检查是否包含GPS数据
            if (strstr(gps_string, "+QGPSLOC:") == NULL &&
                strstr(gps_string, "QGPSLOC:") == NULL) {
                printf("No GPS data in this message\r\n");

                // 检查是否是其他响应
                if (strstr(gps_string, "OK") != NULL) {
                    printf("OK response received\r\n");
                } else if (strstr(gps_string, "ERROR") != NULL) {
                    printf("ERROR response received\r\n");
                }
            } else {
                // 解析GPS数据
                if (GPS_Parse_String(gps_string)) {
                    printf("GPS parse successful\r\n");

                    // 获取并显示数据
                    GPS_Data_t data;
                    if (GPS_Get_Data(&data)) {
                        printf("Latitude: %.6f°%c\r\n",
                               fabs(data.location.latitude), data.location.latitude_direction);
                        printf("Longitude: %.6f°%c\r\n",
                               fabs(data.location.longitude), data.location.longitude_direction);
                        printf("Time: %02d:%02d:%05.2f (UTC+8)\r\n",
                               data.time.local_hour, data.time.local_minute, data.time.local_second);
                        printf("Date: %04d-%02d-%02d\r\n",
                               data.time.year, data.time.month, data.time.day);
                        printf("Altitude: %.1fm, Speed: %.1fkm/h\r\n",
                               data.altitude, data.speed);
                        printf("Satellites: %d, HDOP: %.2f\r\n",
                               data.satellites, data.hdop);
                    }
                } else {
                    printf("GPS parse failed\r\n");
                }
            }
            printf("=== End of Processing ===\r\n\r\n");
        }

        // 重置标志
        gps_data_ready = false;
        gps_data_length = 0;

        // 清空缓冲区
        memset(gps_rx_buffer, 0, GPS_RX_BUFFER_SIZE);
    }
}
/**
  * @brief  上传GPS数据（示例函数）
  */
void upload_gps_data(void) {
    if (GPS_Is_Data_Valid()) {
        GPS_Data_t data;
        if (GPS_Get_Data(&data)) {
            // 这里可以添加上传代码，例如：
            // 1. 通过4G模块上传到服务器
            // 2. 通过LoRa发送到网关
            // 3. 存储到SD卡

            printf("Uploading GPS Data...\r\n");
            printf("  Location: %.6f, %.6f\r\n",
                   data.location.latitude, data.location.longitude);
            printf("  Time: %02d:%02d:%05.2f\r\n",
                   data.time.local_hour, data.time.local_minute, data.time.local_second);
            printf("  Satellites: %d\r\n\r\n", data.satellites);

            // 示例：通过串口模拟上传
            snprintf(print_buffer, sizeof(print_buffer),
                    "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,\"time\":\"%02d:%02d:%05.2f\"}",
                    data.location.latitude, data.location.longitude,
                    data.altitude,
                    data.time.local_hour, data.time.local_minute, data.time.local_second);

            printf("JSON: %s\r\n\r\n", print_buffer);
        }
    }
}
