#include "main.h"

#define power_key               HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_5)
#define menu_key                HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_6)
#define change_left_key         HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_7)
#define change_right_key        HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8)
#define enter_key               HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9)
void PowerKey_Hand(void);
