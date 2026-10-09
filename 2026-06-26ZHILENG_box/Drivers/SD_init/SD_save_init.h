// config_sd.h
#ifndef CONFIG_SD_H
#define CONFIG_SD_H

#include <stdint.h>

// 保存当前配置到SD卡
void SaveConfigToSD(void);

// 从SD卡加载配置，成功返回1，失败返回0
uint8_t LoadConfigFromSD(void);

#endif
