#include "main.h"
#include "data_send.h"

extern uint8_t lock1_state;
extern uint8_t lock2_state;
extern uint8_t lock3_state;
extern uint8_t lock4_state;

void lock1_on(void)
{
	lock1_state=1;
//	door_open(0x01,0x00);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_SET);
}

void lock1_off(void)
{
	lock1_state=0;
//	door_open(0x01,0x01);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_RESET);
}

void lock2_on(void)
{
	lock2_state=1;
//	door_open(0x02,0x00);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_SET);
}

void lock2_off(void)
{
	lock2_state=0;
//	door_open(0x02,0x01);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_RESET);
}

void lock3_on(void)
{
	lock3_state=1;
//	door_open(0x03,0x00);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_SET);
}

void lock3_off(void)
{
	lock3_state=0;
//	door_open(0x01,0x01);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_RESET);
}

void lock4_on(void)
{
	lock4_state=1;
//	door_open(0x04,0x00);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);
}

void lock4_off(void)
{
	lock4_state=0;
//	door_open(0x01,0x01);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET);
}
