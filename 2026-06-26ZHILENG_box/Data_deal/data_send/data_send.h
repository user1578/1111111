#include "main.h"




void music_play(void);
void heart_break_send(void);
void wind_temp_send(void);
void alarm_data(uint8_t alarm_type);
void Fault_data(uint8_t Fault_type);
void lock_data(uint8_t lock_add);
void temp_set(float set_temp_data);
void code_OK(void);
void code_admin_ask(void);
void code_lock_ask(void);
void EC800Send_HexData(uint8_t *bufferdata, uint16_t len);
void music2_play(void);
void music_time(void);
void music_bat(void);
void music_temp(void);
void temp_set_OK(void);
void alarm_set_OK(void);
void music_temp_OK(void);
void music_time_OK(void);
void time_updata(void);
void print_from_sd(void);
void print_OK(uint8_t print_cnt);
void door_open(uint8_t door_data,uint8_t door_state);
void shebei_open(uint8_t load_dir,uint8_t shebei_state);
void xiangti_data(uint8_t PCM_data,uint8_t cheti_data,uint8_t lengye_data);
void start_OK(void);
void zhileng_OK(void);
void init_send(void);
void vision_send(void);
void music3_play(void);
void print_send(void);
