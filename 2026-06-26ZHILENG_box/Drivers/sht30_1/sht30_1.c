#include "sht30_1.h"

// 定义 8 个传感器引脚配置（按 T1 ~ T8 顺序）
SHT30_Device sht30_devices[SHT30_DEVICE_COUNT] = {
    // T1 ~ T6（原有）
    {GPIOE, GPIO_PIN_4,  GPIO_PIN_6,  SHT30_ADDR},   // T1
    {GPIOF, GPIO_PIN_0,  GPIO_PIN_1,  SHT30_ADDR},   // T2
    {GPIOF, GPIO_PIN_2,  GPIO_PIN_3,  SHT30_ADDR},   // T3
    {GPIOF, GPIO_PIN_4,  GPIO_PIN_5,  SHT30_ADDR},   // T4
    {GPIOF, GPIO_PIN_6,  GPIO_PIN_7,  SHT30_ADDR},   // T5
    {GPIOF, GPIO_PIN_8,  GPIO_PIN_9,  SHT30_ADDR},   // T6

    // T7、T8（新增）
    {GPIOF, GPIO_PIN_10, GPIO_PIN_11, SHT30_ADDR},   // T7  SDA=PF10, SCL=PF11
    {GPIOF, GPIO_PIN_12, GPIO_PIN_13, SHT30_ADDR},   // T8  SDA=PF12, SCL=PF13
};

// 初始化所有传感器
void SHT30_Init_All(void)
{
    for (int i = 0; i < SHT30_DEVICE_COUNT; i++) {
        SHT30_Device_Init(&sht30_devices[i]);
    }
}

// 初始化单个传感器
void SHT30_Device_Init(SHT30_Device *dev)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    // 使能 GPIO 时钟
    if (dev->IIC_PORT == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    } else if (dev->IIC_PORT == GPIOF) {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    }
    // 若还有其他端口，继续添加

    // 配置 SDA 和 SCL 为推挽输出
    GPIO_InitStructure.Pin = dev->SDA_PIN | dev->SCL_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(dev->IIC_PORT, &GPIO_InitStructure);

    // 释放总线
    SHT30_IIC_SDA_H(dev);
    SHT30_IIC_SCL_H(dev);
    delay_us(6);
}

// 切换 SDA 为输出模式
void SHT30_IIC_SDA_OUT_Device(SHT30_Device *dev)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.Pin = dev->SDA_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(dev->IIC_PORT, &GPIO_InitStructure);
}

// 切换 SDA 为输入模式
void SHT30_IIC_SDA_IN_Device(SHT30_Device *dev)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.Pin = dev->SDA_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(dev->IIC_PORT, &GPIO_InitStructure);
}

// IIC 起始信号
void SHT30_IIC_start_Device(SHT30_Device *dev)
{
    SHT30_IIC_SDA_H(dev);
    SHT30_IIC_SCL_H(dev);
    delay_us(6);
    SHT30_IIC_SDA_L(dev);
    SHT30_IIC_SCL_L(dev);
    delay_us(6);
}

// IIC 停止信号
void SHT30_IIC_stop_Device(SHT30_Device *dev)
{
    SHT30_IIC_SCL_H(dev);
    SHT30_IIC_SDA_L(dev);
    delay_us(6);
    SHT30_IIC_SDA_H(dev);
    SHT30_IIC_SCL_L(dev);
    delay_us(6);
}

// 等待应答
uint8_t SHT30_IIC_Get_ack_Device(SHT30_Device *dev)
{
    uint16_t CNT = 0;
    SHT30_IIC_SDA_IN_Device(dev);
    SHT30_IIC_SCL_L(dev);
    SHT30_IIC_SCL_H(dev);
    delay_us(6);

    while((SHT30_READ_SDA(dev)) && (CNT < 100))
    {
        CNT++;
        if(CNT == 100)
        {
            SHT30_IIC_SDA_OUT_Device(dev);
            return 0;
        }
    }

    SHT30_IIC_SCL_L(dev);
    delay_us(6);
    SHT30_IIC_SDA_OUT_Device(dev);
    return 1;
}

// 发送应答（ACK）
void SHT30_IIC_ACK_Device(SHT30_Device *dev)
{
    SHT30_IIC_SDA_L(dev);
    SHT30_IIC_SCL_H(dev);
    delay_us(6);
    SHT30_IIC_SCL_L(dev);
    delay_us(6);
}

// 发送非应答（NACK）
void SHT30_IIC_NACK_Device(SHT30_Device *dev)
{
    SHT30_IIC_SDA_H(dev);
    SHT30_IIC_SCL_H(dev);
    delay_us(6);
    SHT30_IIC_SCL_L(dev);
}

// 写一个字节
void SHT30_IIC_write_byte_Device(SHT30_Device *dev, uint8_t Data)
{
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
        if((Data & 0x80) == 0x80)
            SHT30_IIC_SDA_H(dev);
        else
            SHT30_IIC_SDA_L(dev);

        SHT30_IIC_SCL_H(dev);
        delay_us(6);
        SHT30_IIC_SCL_L(dev);
        delay_us(6);
        Data = Data << 1;
    }
    SHT30_IIC_SDA_L(dev);
}

// 读一个字节
uint8_t SHT30_IIC_read_byte_Device(SHT30_Device *dev, uint8_t ack)
{
    uint8_t i;
    uint8_t Data = 0;

    SHT30_IIC_SDA_IN_Device(dev);
    SHT30_IIC_SCL_L(dev);
    SHT30_IIC_SDA_H(dev);
    delay_us(6);

    for(i = 0; i < 8; i++)
    {
        Data = Data << 1;
        SHT30_IIC_SCL_H(dev);
        delay_us(6);
        if(SHT30_READ_SDA(dev))
            Data = Data | 0x01;
        SHT30_IIC_SCL_L(dev);
        delay_us(6);
    }

    SHT30_IIC_SDA_OUT_Device(dev);

    if (!ack)
        SHT30_IIC_NACK_Device(dev);
    else
        SHT30_IIC_ACK_Device(dev);

    return Data;
}

// 发送命令
void SHT30_CMD_Device(SHT30_Device *dev, uint16_t cmd)
{
    SHT30_IIC_start_Device(dev);
    SHT30_IIC_write_byte_Device(dev, dev->addr + 0);   // 写地址
    SHT30_IIC_Get_ack_Device(dev);
    SHT30_IIC_write_byte_Device(dev, (cmd >> 8) & 0xFF);
    SHT30_IIC_Get_ack_Device(dev);
    SHT30_IIC_write_byte_Device(dev, cmd & 0xFF);
    SHT30_IIC_Get_ack_Device(dev);
    SHT30_IIC_stop_Device(dev);
    delay_ms(50);
}

// 读取温湿度
uint8_t SHT30_Read_Humiture_Device(SHT30_Device *dev, float *temp, float *humi)
{
    uint8_t buff[6];

    SHT30_CMD_Device(dev, SHT30_READ_HUMITURE);

    SHT30_IIC_start_Device(dev);
    SHT30_IIC_write_byte_Device(dev, dev->addr + 1);   // 读地址
    SHT30_IIC_Get_ack_Device(dev);

    buff[0] = SHT30_IIC_read_byte_Device(dev, 1);
    buff[1] = SHT30_IIC_read_byte_Device(dev, 1);
    buff[2] = SHT30_IIC_read_byte_Device(dev, 1);
    buff[3] = SHT30_IIC_read_byte_Device(dev, 1);
    buff[4] = SHT30_IIC_read_byte_Device(dev, 1);
    buff[5] = SHT30_IIC_read_byte_Device(dev, 0);

    SHT30_IIC_stop_Device(dev);

    // CRC 校验
    if(SHT3X_CRC(&buff[0], 2) == buff[2] && SHT3X_CRC(&buff[3], 2) == buff[5])
    {
        *temp = -45.0f + (175.0f * (((uint16_t)buff[0] << 8) + buff[1]) / 65535.0f);
        *humi = 100.0f * (((uint16_t)buff[3] << 8) + buff[4]) / 65535.0f;

        if(*temp > 125.0f) *temp = 125.0f;
        else if(*temp < -40.0f) *temp = -40.0f;

        return 0;
    }
    else
        return 1;
}

// CRC 校验函数
unsigned char SHT3X_CRC(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    uint8_t byteCtr, bit;

    for(byteCtr = 0; byteCtr < len; byteCtr++) {
        crc ^= data[byteCtr];
        for(bit = 8; bit > 0; --bit) {
            if(crc & 0x80)
                crc = (crc << 1) ^ POLYNOMIAL_CXDZ;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

// ----- 兼容旧代码（使用第一个传感器）-----
void SHT30_Init(void)
{
    SHT30_Device_Init(&sht30_devices[0]);
}

u8 SHT30_Read_Humiture(float *temp, float *humi)
{
    return SHT30_Read_Humiture_Device(&sht30_devices[0], temp, humi);
}
