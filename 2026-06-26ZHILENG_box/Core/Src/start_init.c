#include "main.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "msd.h"
#include "ff.h"
#include "stdio.h"
#include "sys.h"
#include "adc.h"
#include "lock_state.h"
#include "battery_soc.h"
#include "sht30_1.h"
#include "sd_store_send.h"
#include "offline_sender.h"
#include "freertos_demo.h"
#include "temperature_control.h"
#include "iwdg.h"
#include "SD_save_init.h"

TempControlParams control_params;extern char file_num;//Ñ¡ÖÐµÄÎÄ¼þ±êºÅ

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

void Start_fuc(void)
{
	  delay_init(72); //初始化延时函数
	  SHT30_Init_All();

	  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
	  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2);
	  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);
	  HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_1);
	  HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_2);
//	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
	  change_bat_OFF();
	  SD_Storage_Init();
	    // 尝试从SD卡加载配置，如果成功则使用加载的值，否则保持默认
	    if (LoadConfigFromSD()) {
	        // 加载成功，用加载的值设置温度控制模块
	        TempControl_Init(&control_params);
	        TempControl_SetBoxType(&control_params, Car_data);
	        TempControl_SetPCMType(&control_params, PCM_state, cold_data);
	        TempControl_SetTemperature(&control_params,(float)temp_low,(float)temp_high);
	    } else {
	  	  // 温度控制初始化
  	  TempControl_Init(&control_params);
  	  // 设置为91L大箱箱体
  	  TempControl_SetBoxType(&control_params, BOX_91L);
  	   // 设置常规PCM
  	  TempControl_SetPCMType(&control_params, PCM_NORMAL, 14);
	  	   // 设置温度区间[2,8]℃
	  	  TempControl_SetTemperature(&control_params, 2.0f, 8.0f);
	    }
	  IWDG_Init(IWDG_PRESCALER_256, 3125);
//	  sd_card_init();
	  Open_RF();                         //开天线
	  Battery_SOC_Init();	            // 初始化
	  lock1_on();
	  lock2_on();
	  lock3_on();
//	  __HAL_TIM_SET_COMPARE(&htim4,TIM_CHANNEL_1,200);
//	  __HAL_TIM_SET_COMPARE(&htim4,TIM_CHANNEL_2,50);
//	  HAL_TIM_Base_Start(&htim2);
}
