#include "main.h"
#include "adc.h"

float get_bat_voltage(void)
{
	int value=0;
	float bat_data;
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,HAL_MAX_DELAY);
	value=HAL_ADC_GetValue(&hadc1);//读电池电压
	bat_data=(((float)value)/4095)*3.3;
	return bat_data;
}
