#include "main.h"



#define read_lock1  HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_14)
#define read_lock2  HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_15)
#define read_lock3  HAL_GPIO_ReadPin(GPIOG,GPIO_PIN_11)
#define read_lock4  HAL_GPIO_ReadPin(GPIOG,GPIO_PIN_12)



void lock1_on(void);
void lock1_off(void);
void lock2_on(void);
void lock2_off(void);
void lock3_on(void);
void lock3_off(void);
void lock4_on(void);
void lock4_off(void);
