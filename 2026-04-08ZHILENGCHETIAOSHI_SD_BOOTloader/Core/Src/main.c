/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Bootloader for STM32F407 (SPI SD Card & FatFs)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "gpio.h"
#include "msd.h"
#include "ff.h"
#include "usart.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);

/* Private define ------------------------------------------------------------*/
#define BOOT_SIZE_KB          (32)
#define APP_SIZE_KB           (480)

#define FLASH_BASE_ADDR       (0x08000000UL)
#define APP_START_ADDR        (FLASH_BASE_ADDR + BOOT_SIZE_KB * 1024)

#define FLASH_PAGE_SIZE       (2 * 1024)

#define FIRMWARE_FILE         "firmware.bin"
#define FIRMWARE_READY_FILE   "OK.DAT"

typedef struct {
    uint32_t start_addr;
    uint32_t sector_num;
} SectorMapEntry;

static const SectorMapEntry sector_map[] = {
    {0x08000000, 0}, {0x08004000, 1}, {0x08008000, 2}, {0x0800C000, 3},
    {0x08010000, 4}, {0x08020000, 5}, {0x08040000, 6}, {0x08060000, 7},
};
#define SECTOR_MAP_SIZE (sizeof(sector_map) / sizeof(sector_map[0]))

static uint32_t GetSector(uint32_t addr)
{
    uint32_t i;
    for (i = SECTOR_MAP_SIZE - 1; i > 0; i--) {
        if (addr >= sector_map[i].start_addr) {
            return sector_map[i].sector_num;
        }
    }
    return 0;
}

static bool TF_Card_Detect(void);
static int  Update_App_Code(FIL* fp);
static void Jump_To_App(void);
static bool IsAppValid(void);
static void UART_Print(const char *str);

FATFS g_fs;
FIL   g_file;

void SystemClock_Config(void);

static void UART_Print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 200);
}

static bool TF_Card_Detect(void)
{
    return (MSD_Init() == 0);
}

static bool IsAppValid(void)
{
    uint32_t app_stack = *(volatile uint32_t*)APP_START_ADDR;
    return (app_stack >= 0x20000000 && app_stack <= 0x20020000);
}

static int Update_App_Code(FIL* fp)
{
    FRESULT res;
    UINT br;
    uint8_t buffer[FLASH_PAGE_SIZE];
    uint32_t flash_addr;
    uint32_t error;
    FLASH_EraseInitTypeDef erase;
    uint32_t start_sector, end_sector, sector;

    if (fp == NULL) return -1;

    UART_Print("Start Erasing APP Area\r\n");

    HAL_FLASH_Unlock();

    if (FLASH->CR & FLASH_CR_LOCK) {
        UART_Print("ERROR: Flash still locked!\r\n");
        return -1;
    }

    start_sector = GetSector(APP_START_ADDR);
    end_sector = GetSector(APP_START_ADDR + APP_SIZE_KB * 1024 - 1);

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_1;

    for (sector = start_sector; sector <= end_sector; sector++) {
        erase.Sector    = sector;
        erase.NbSectors = 1;

        uint8_t retry = 0;
        while (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
            retry++;
            HAL_Delay(10);
            if (retry > 200) {
                char err_buf[48];
                sprintf(err_buf, "Erase sector %lu failed\r\n", (unsigned long)sector);
                UART_Print(err_buf);
                HAL_FLASH_Lock();
                return -1;
            }
        }
        char sec_buf[48];
        sprintf(sec_buf, "Sector %lu erased OK\r\n", (unsigned long)sector);
        UART_Print(sec_buf);
    }

    UART_Print("Erase done, start programming\r\n");

    flash_addr = APP_START_ADDR;
    while (1) {
        res = f_read(fp, buffer, FLASH_PAGE_SIZE, &br);
        if (res != FR_OK) {
            UART_Print("Read firmware failed!\r\n");
            HAL_FLASH_Lock();
            return -1;
        }
        if (br == 0) break;

        if (flash_addr == APP_START_ADDR) {
            char dbg[64];
            UART_Print("First 16 bytes from file: ");
            for (int k = 0; k < 16 && k < (int)br; k++) {
                sprintf(dbg + k*3, "%02X ", buffer[k]);
            }
            UART_Print(dbg);
            UART_Print("\r\n");
            sprintf(dbg, "br=%u flash_addr=0x%08lX\r\n", (unsigned)br, (unsigned long)flash_addr);
            UART_Print(dbg);
        }

        for (uint32_t i = 0; i < br; i += 4) {
            uint32_t word;
            if (i + 3 < br) {
                word = ((uint32_t)buffer[i+3] << 24) | ((uint32_t)buffer[i+2] << 16) |
                       ((uint32_t)buffer[i+1] << 8)  | (uint32_t)buffer[i];
            } else {
                word = 0xFFFFFFFF;
                for (uint32_t j = 0; i + j < br; j++) {
                    word |= (uint32_t)buffer[i+j] << (j * 8);
                }
            }
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_addr + i, word) != HAL_OK) {
                UART_Print("Program failed!\r\n");
                HAL_FLASH_Lock();
                return -1;
            }
        }
        flash_addr += br;
    }

    HAL_FLASH_Lock();
    UART_Print("Programming done\r\n");

    {
        uint32_t verify_stack = *(volatile uint32_t*)APP_START_ADDR;
        uint32_t verify_reset = *(volatile uint32_t*)(APP_START_ADDR + 4);
        char vbuf[64];
        sprintf(vbuf, "Verify: stack=0x%08lX reset=0x%08lX\r\n",
                (unsigned long)verify_stack, (unsigned long)verify_reset);
        UART_Print(vbuf);
    }

    return 0;
}

static void Jump_To_App(void)
{
    uint32_t app_stack = *(volatile uint32_t*)APP_START_ADDR;
    uint32_t app_reset = *(volatile uint32_t*)(APP_START_ADDR + 4);
    pFunction jump_to_app;
    char dbg[64];

    sprintf(dbg, "APP stack: 0x%08lX\r\n", (unsigned long)app_stack);
    UART_Print(dbg);
    sprintf(dbg, "APP reset: 0x%08lX\r\n", (unsigned long)app_reset);
    UART_Print(dbg);

    if (!IsAppValid()) {
        UART_Print("APP invalid, cannot jump.\r\n");
        return;
    }

    UART_Print("Jumping to APP...\r\n");

    HAL_SPI_DeInit(&hspi1);
    HAL_UART_DeInit(&huart1);
    HAL_DeInit();

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __set_MSP(app_stack);
    jump_to_app = (pFunction)app_reset;
    jump_to_app();

    while (1);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    HAL_Delay(500);

    UART_Print("\r\n=== Bootloader Starting ===\r\n");

    if (!TF_Card_Detect()) {
        UART_Print("No SD card, check APP...\r\n");
        if (IsAppValid()) {
            UART_Print("Valid APP, jumping...\r\n");
            Jump_To_App();
        } else {
            UART_Print("No valid APP, halt.\r\n");
            while (1);
        }
    }

    if (f_mount(0, &g_fs) != FR_OK) {
        UART_Print("Mount failed, try APP...\r\n");
        if (IsAppValid()) {
            Jump_To_App();
        } else {
            UART_Print("No valid APP, halt.\r\n");
            while (1);
        }
    }

    if (f_stat(FIRMWARE_FILE, NULL) != FR_OK) {
        UART_Print("No firmware.bin, jump APP...\r\n");
        if (IsAppValid()) {
            Jump_To_App();
        } else {
            UART_Print("No valid APP, halt.\r\n");
            while (1);
        }
    }

    if (f_stat(FIRMWARE_READY_FILE, NULL) != FR_OK) {
        UART_Print("No OK.DAT, upgrade incomplete, delete bin...\r\n");
        f_unlink(FIRMWARE_FILE);
        if (IsAppValid()) {
            Jump_To_App();
        } else {
            UART_Print("No valid APP, halt.\r\n");
            while (1);
        }
    }

    UART_Print("Found firmware.bin+OK.DAT, updating...\r\n");
    if (f_open(&g_file, FIRMWARE_FILE, FA_READ) != FR_OK) {
        UART_Print("Open firmware.bin failed!\r\n");
        if (IsAppValid()) Jump_To_App();
        else while (1);
    }

    {
        DWORD fsize = f_size(&g_file);
        char size_buf[32];
        UART_Print("firmware.bin size: ");
        sprintf(size_buf, "%lu bytes\r\n", (unsigned long)fsize);
        UART_Print(size_buf);
    }

    if (Update_App_Code(&g_file) != 0) {
        UART_Print("Update failed!\r\n");
        f_close(&g_file);
        if (IsAppValid()) Jump_To_App();
        else while (1);
    }

    f_close(&g_file);

    f_unlink(FIRMWARE_READY_FILE);
    f_unlink(FIRMWARE_FILE);

    UART_Print("Update OK, jumping to APP...\r\n");
    HAL_Delay(500);
    Jump_To_App();

    while (1);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
