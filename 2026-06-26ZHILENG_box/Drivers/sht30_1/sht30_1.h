#ifndef __SHT30_H
#define __SHT30_H

#include "main.h"
#include "delay.h"

/***************** 传感器数量 *****************/
#define SHT30_DEVICE_COUNT 8   // 支持8个探头

/***************** 结构体定义 *****************/
typedef struct {
    GPIO_TypeDef *IIC_PORT;
    uint16_t SDA_PIN;
    uint16_t SCL_PIN;
    uint8_t addr;               // I2C 地址（一般为 0x44<<1）
} SHT30_Device;

// 声明外部数组，在 .c 文件中定义
extern SHT30_Device sht30_devices[SHT30_DEVICE_COUNT];

/***************** IO 操作宏（需要传入设备指针） *****************/
#define SHT30_IIC_SDA_H(dev)    HAL_GPIO_WritePin((dev)->IIC_PORT, (dev)->SDA_PIN, GPIO_PIN_SET)
#define SHT30_IIC_SDA_L(dev)    HAL_GPIO_WritePin((dev)->IIC_PORT, (dev)->SDA_PIN, GPIO_PIN_RESET)
#define SHT30_IIC_SCL_H(dev)    HAL_GPIO_WritePin((dev)->IIC_PORT, (dev)->SCL_PIN, GPIO_PIN_SET)
#define SHT30_IIC_SCL_L(dev)    HAL_GPIO_WritePin((dev)->IIC_PORT, (dev)->SCL_PIN, GPIO_PIN_RESET)
#define SHT30_READ_SDA(dev)     HAL_GPIO_ReadPin((dev)->IIC_PORT, (dev)->SDA_PIN)

/***************** I2C 地址与命令 *****************/
#define SHT30_ADDR      (uint8_t)(0x44 << 1)   // 默认地址
#define SHT30_READ_HUMITURE  (uint16_t)0x2C06  // 读取温湿度命令

/***************** CRC 多项式 *****************/
#define POLYNOMIAL_CXDZ 0x31   // X^8 + X^5 + X^4 + 1

/***************** 函数声明 *****************/
void SHT30_Init_All(void);
void SHT30_Device_Init(SHT30_Device *dev);
uint8_t SHT30_Read_Humiture_Device(SHT30_Device *dev, float *temp, float *humi);

// 以下函数为内部使用，也可声明
void SHT30_IIC_SDA_OUT_Device(SHT30_Device *dev);
void SHT30_IIC_SDA_IN_Device(SHT30_Device *dev);
void SHT30_IIC_start_Device(SHT30_Device *dev);
void SHT30_IIC_stop_Device(SHT30_Device *dev);
uint8_t SHT30_IIC_Get_ack_Device(SHT30_Device *dev);
void SHT30_IIC_ACK_Device(SHT30_Device *dev);
void SHT30_IIC_NACK_Device(SHT30_Device *dev);
void SHT30_IIC_write_byte_Device(SHT30_Device *dev, uint8_t Data);
uint8_t SHT30_IIC_read_byte_Device(SHT30_Device *dev, uint8_t ack);
void SHT30_CMD_Device(SHT30_Device *dev, uint16_t cmd);
unsigned char SHT3X_CRC(uint8_t *data, uint8_t len);

// 兼容旧代码（使用第一个传感器）
void SHT30_Init(void);
u8 SHT30_Read_Humiture(float *temp, float *humi);

#endif /* __SHT30_H */
