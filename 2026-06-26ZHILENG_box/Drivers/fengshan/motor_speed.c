#include "main.h"
#include "tim.h"
#include "usart.h"
#include "string.h"



extern uint16_t fengshan1_speed;
extern uint16_t fengshan2_speed;
extern uint16_t heat_data;
extern char ping_send_hide[100];
extern uint8_t ping_end_data[3];
extern uint32_t ping_fengshan12_speed;
extern uint32_t ping_fengshan34_speed;
void moter1_speed(uint16_t speed) //电机1转速
{
	  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_SET);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,(300-speed));
}

void moter2_speed(uint16_t speed)  //电机2转速
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7, GPIO_PIN_SET);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_2,(300-speed));
}

void moter3_speed(uint16_t speed)  //电机3转速
{
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);
	__HAL_TIM_SET_COMPARE(&htim4,TIM_CHANNEL_1,(300-speed));
}

void moter4_speed(uint16_t speed)  //电机4转速设置
{
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET);
	__HAL_TIM_SET_COMPARE(&htim4,TIM_CHANNEL_2,(300-speed));
}

void heat_warm(uint16_t warm)   //加热片加热设置
{
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_3,300-warm);
}

void motor1_stop(void)   //电机1停
{
	  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_RESET);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,300);
}

void motor2_stop(void)  //电机2停
{
	  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7, GPIO_PIN_RESET);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_2,300);
}

void moter3_stop(void)  //电机3停
{
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);
	__HAL_TIM_SET_COMPARE(&htim4,TIM_CHANNEL_1,300);
}

void moter4_stop(void) //电机4停
{
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET);
	__HAL_TIM_SET_COMPARE(&htim4,TIM_CHANNEL_2,300);
}

void heat_stop(void) //加热片停
{
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_3,300);
}

void moter_heat_Stop(void)
{
    // 关闭所有风扇和加热
    motor1_stop();
    motor2_stop();
    moter3_stop();
    moter4_stop();
    fengshan1_speed=0;
    fengshan2_speed=0;
    heat_stop();
    heat_data=0;
}

void moter_hand(void)
{
    // 关闭所有风扇和加热
	moter1_speed((uint16_t)ping_fengshan12_speed);
	moter2_speed((uint16_t)ping_fengshan12_speed);
	moter3_speed((uint16_t)ping_fengshan34_speed);
	moter4_speed((uint16_t)ping_fengshan34_speed);
    fengshan1_speed=ping_fengshan12_speed;
    fengshan2_speed=ping_fengshan34_speed;
    heat_stop();
    heat_data=0;
}

