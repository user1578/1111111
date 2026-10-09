#include "ff.h"
#include <string.h>
#include "main.h" // 包含全局变量声明

// 定义配置文件的魔数和版本，用于校验
#define CONFIG_MAGIC    0x5A5A5A5A
#define CONFIG_VERSION  1


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
extern uint32_t PCM_state;
extern uint32_t Car_data;
extern uint8_t start_state;
extern uint8_t admin_code_data[6];
extern uint8_t lock1_code_data[6];
extern uint8_t lock2_code_data[6];
extern uint8_t lock3_code_data[6];
extern uint8_t lock4_code_data[6];
// 保存当前配置到SD卡
void SaveConfigToSD(void)
{
    FIL file;
    UINT bw;
    FRESULT res;

    // 1. 准备数据缓冲区，按顺序存放所有变量
    //    先放魔数和版本（用于校验）
    uint32_t magic = CONFIG_MAGIC;
    uint32_t version = CONFIG_VERSION;

    // 2. 以写入方式打开配置文件（覆盖）
    res = f_open(&file, "config.dat", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) return;

    // 3. 写入魔数
    f_write(&file, &magic, sizeof(magic), &bw);
    // 4. 写入版本
    f_write(&file, &version, sizeof(version), &bw);

    // 5. 按顺序写入各个变量
    // 开关机状态（如果start_state已定义）
    f_write(&file, &start_state, sizeof(start_state), &bw);

    // 制冷车参数
    f_write(&file, &cold_data, sizeof(cold_data), &bw);
    f_write(&file, &PCM_state, sizeof(PCM_state), &bw);
    f_write(&file, &Car_data, sizeof(Car_data), &bw);

    // 报警设置
    f_write(&file, &battery_warn_low, sizeof(battery_warn_low), &bw);
    f_write(&file, &battery_alarm_low, sizeof(battery_alarm_low), &bw);
    f_write(&file, &keepwarn_low, sizeof(keepwarn_low), &bw);
    f_write(&file, &keepwarn_serious_low, sizeof(keepwarn_serious_low), &bw);

    // 参数设置
    f_write(&file, &temp_high, sizeof(temp_high), &bw);
    f_write(&file, &temp_low, sizeof(temp_low), &bw);
    f_write(&file, &warn_high, sizeof(warn_high), &bw);
    f_write(&file, &warn_low, sizeof(warn_low), &bw);
    f_write(&file, &alarm_temp, sizeof(alarm_temp), &bw);
    f_write(&file, &alarm_low, sizeof(alarm_low), &bw);

    // 新增：写入密码
    f_write(&file, admin_code_data, sizeof(admin_code_data), &bw);
    f_write(&file, lock1_code_data, sizeof(lock1_code_data), &bw);
    f_write(&file, lock2_code_data, sizeof(lock2_code_data), &bw);
    f_write(&file, lock3_code_data, sizeof(lock3_code_data), &bw);
    f_write(&file, lock4_code_data, sizeof(lock4_code_data), &bw);

    // 6. 关闭文件
    f_close(&file);
}

// 从SD卡加载配置，成功返回1，失败返回0
uint8_t LoadConfigFromSD(void)
{
    FIL file;
    UINT br;
    FRESULT res;

    // 1. 打开配置文件
    res = f_open(&file, "config.dat", FA_READ);
    if (res != FR_OK) return 0;

    // 2. 读取并验证魔数和版本
    uint32_t magic, version;
    f_read(&file, &magic, sizeof(magic), &br);
    f_read(&file, &version, sizeof(version), &br);
    if (magic != CONFIG_MAGIC || version != CONFIG_VERSION) {
        f_close(&file);
        return 0;
    }

    // 3. 按顺序读取各个变量
    f_read(&file, &start_state, sizeof(start_state), &br);

    f_read(&file, &cold_data, sizeof(cold_data), &br);
    f_read(&file, &PCM_state, sizeof(PCM_state), &br);
    f_read(&file, &Car_data, sizeof(Car_data), &br);

    f_read(&file, &battery_warn_low, sizeof(battery_warn_low), &br);
    f_read(&file, &battery_alarm_low, sizeof(battery_alarm_low), &br);
    f_read(&file, &keepwarn_low, sizeof(keepwarn_low), &br);
    f_read(&file, &keepwarn_serious_low, sizeof(keepwarn_serious_low), &br);

    f_read(&file, &temp_high, sizeof(temp_high), &br);
    f_read(&file, &temp_low, sizeof(temp_low), &br);
    f_read(&file, &warn_high, sizeof(warn_high), &br);
    f_read(&file, &warn_low, sizeof(warn_low), &br);
    f_read(&file, &alarm_temp, sizeof(alarm_temp), &br);
    f_read(&file, &alarm_low, sizeof(alarm_low), &br);

    // 读取密码
    f_read(&file, admin_code_data, sizeof(admin_code_data), &br);
    f_read(&file, lock1_code_data, sizeof(lock1_code_data), &br);
    f_read(&file, lock2_code_data, sizeof(lock2_code_data), &br);
    f_read(&file, lock3_code_data, sizeof(lock3_code_data), &br);
    f_read(&file, lock4_code_data, sizeof(lock4_code_data), &br);

    f_close(&file);
    return 1;
}
