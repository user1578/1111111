/**
 ****************************************************************************************************

 ****************************************************************************************************
 */
 
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include"freertos_demo.h"
#include "sht30_1.h"
#include "usart.h"
#include "4g_con.h"
#include "data_send.h"
#include "motor_speed.h"
#include "data_rec.h"
#include "tim.h"
#include "lock_state.h"
#include "string.h"
#include "stdio.h"
#include "printer_driver.h"
#include "adc.h"
#include "ff.h"
#include "get_bat.h"
#include "battery_soc.h"
#include "sd_store_send.h"
#include "offline_sender.h"
#include "temperature_control.h"
#include "iwdg.h"
#include "start_ping.h"
#include "my_RTC.h"
#include "key_ping.h"
#include "Ble_driver.h"
/******************************************************************************************************/
/*FreeRTOS����*/

/* START_TASK ���� ����
 * ����: ������ �������ȼ� ��ջ��С ��������
 */
//�������ȼ�
#define START_TASK_PRIO		1
//�����ջ��С
#define START_STK_SIZE 		680
//������
TaskHandle_t StartTask_Handler;
//������
void start_task(void *pvParameters);

//�������ȼ�
#define LED0_TASK_PRIO		2
//�����ջ��С
#define LED0_STK_SIZE 		50
//������
TaskHandle_t LED0Task_Handler;
//������
void led0_task(void *pvParameters);

//�������ȼ�
#define LED1_TASK_PRIO		17
//�����ջ��С
#define LED1_STK_SIZE 		1024
//������
TaskHandle_t LED1Task_Handler;
//������
void led1_task(void *pvParameters);

//�������ȼ�
#define FLOAT_TASK_PRIO		4
//�����ջ��С
#define FLOAT_STK_SIZE 		128
//������
TaskHandle_t FLOATTask_Handler;
//������
void float_task(void *pvParameters);


//�������ȼ�
#define CAPTURE_TASK_PRIO		14
//�����ջ��С
#define CAPTURE_STK_SIZE 		228
//������
TaskHandle_t CAPTURETask_Handler;
//������
void capture_task(void *pvParameters);

//��ȡGPS��Ϣ
//�������ȼ�
#define READGPS_TASK_PRIO		15
//�����ջ��С
#define READGPS_STK_SIZE 		512
//������
TaskHandle_t READGPSTask_Handler;
//������
void readgps_task(void *pvParameters);

//AD���ݵĻ���ͱȽ�����
//�������ȼ�
#define DATADEAL_TASK_PRIO		18
//�����ջ��С
#define DATADEAL_STK_SIZE 		1024
//������
TaskHandle_t DATADEALTask_Handler;
//������
void datadeal_task(void *pvParameters);

//�쳣���ݵ���������
//�������ȼ�
#define DATALATCH_TASK_PRIO		13
//�����ջ��С
#define DATALATCH_STK_SIZE 		1024
//������
TaskHandle_t DATALATCHTask_Handler;
//������
void datalatch_task(void *pvParameters);

//ʱ���ȡ�߳�
//�������ȼ�
#define TIMEGET_TASK_PRIO		12
//�����ջ��С
#define TIMEGET_STK_SIZE 		628
//������
TaskHandle_t TIMEGETTask_Handler;
//������
void timeget_task(void *pvParameters);




/* LWIP_DEMO ���� ����
 * ����: ������ �������ȼ� ��ջ��С ��������
 */
#define LWIP_DMEO_TASK_PRIO     19       /* �������ȼ� */
#define LWIP_DMEO_STK_SIZE      1024*4       /* �����ջ��С */
TaskHandle_t LWIP_Task_Handler;             /* ������ */
void lwip_demo_task(void *pvParameters);    /* ������ */
/******************************************************************************************************/


uint8_t first_time=1;
float temp[8], humi[8];
uint8_t status[6];
uint16_t moter_speed=0;
float Set_Temp=20.0;
uint8_t test_data[3]={0x01,0x02,0x03};
uint16_t speed_data=0;
float set_proportion=20.0;
uint8_t bat_quantity=100;
int16_t cha_zhi=0;
extern uint16_t dis_connect;
uint8_t lock1_state=0;
uint8_t lock2_state=0;
uint8_t lock3_state=0;
uint8_t lock4_state=0;
uint8_t ping_end_data[3]={0xFF,0xFF,0xFF};
char ping_send_tem[100];
char ping_send_hum[100];
char ping_send_hide[100];
char ping_send_bat[100];
char ping_send_time[100];
extern uint8_t str_rec[300];
extern uint8_t str5_rec[300];
extern uint8_t lock_send[100];
extern uint8_t admin_send[100];
extern uint8_t admin_code_data[6];
extern uint8_t lock1_code_data[6];
extern uint8_t lock2_code_data[6];
extern uint8_t lock3_code_data[6];
extern uint8_t lock4_code_data[6];
extern uint8_t ask_gps_state;
extern uint8_t uart2_state;
extern uint8_t uart4_state;
extern uint8_t uart5_state;
extern uint8_t lock1_flag;
extern uint8_t lock2_flag;
extern uint8_t lock3_flag;
extern uint8_t lock4_flag;
uint16_t heart_break_con=0;
uint16_t data_send_con=0;
float remaining_time = 0.0f;
extern TempControlParams control_params;
extern uint8_t _4g_is_connecting;  // 连接中标志
uint8_t read_gps_cnt;              //读GPS的时序
uint8_t firsttime_4g=1;
extern FATFS fs;
extern FIL fsrc, fdst,file; // file objects
extern FRESULT res; // FatFs function common result code
extern UINT br, bw; // File R/W count
extern int16_t channel_value[4];
float bat_voltage=0;
float battery_soc=100.0;
extern uint16_t fengshan1_speed;
extern uint16_t fengshan2_speed;
extern uint16_t heat_data;
extern float last_time;
extern uint16_t last_hour;
extern uint16_t last_min;
uint32_t lock1_cnt=0;
uint32_t lock2_cnt=0;
uint32_t lock3_cnt=0;
uint32_t lock4_cnt=0;
uint8_t door_open_state=1;
uint8_t print_first_time=1;
extern uint8_t temp_alarm_flag;
extern uint8_t time_alarm_flag;
extern uint8_t start_state;
uint8_t get_sig_state=0;
extern uint8_t auto_state;
extern float see_detaL;
extern float see_L1;
extern float see_L2;
uint16_t RTC_year=0;
uint8_t RTC_month=0;
uint8_t RTC_day=0;
uint8_t RTC_hour=0;
uint8_t RTC_min=0;
uint8_t RTC_second=0;
uint32_t current_time=0;
uint32_t last_RTC_time=0;
uint8_t first_RTC_state=1;
uint8_t data_send_flag=0;
uint8_t print_off_cnt=0;
extern uint16_t coda_value;
uint8_t test_state=0;
extern uint8_t key_ping_send[100];
extern uint8_t uart6_state;
extern uint8_t print_KO;
extern uint8_t IAP_flag;
extern uint32_t cold_data;
extern uint32_t PCM_state;
extern uint32_t Car_data;

/**
 * @breif       freertos_demo
 * @param       ��
 * @retval      ��
 */
void freertos_demo(void)
{
    /* start_task */
    xTaskCreate((TaskFunction_t )start_task,
                (const char *   )"start_task",
                (uint16_t       )START_STK_SIZE,
                (void *         )NULL,
                (UBaseType_t    )START_TASK_PRIO,
                (TaskHandle_t * )&StartTask_Handler);

    vTaskStartScheduler(); /* ����������� */
}

/**
 * @brief       start_task
 * @param       pvParameters : �������(δ�õ�)
 * @retval      ��
 */
//��ʼ����������
//_user_heap_stack
void start_task(void *pvParameters)
{
	//////////////////////////////////////////////////////////////////////

    taskENTER_CRITICAL();           //进入临界区
     vTaskDelay(500);
     //IWDG_Init(IWDG_PRESCALER_64,10000); //��Ƶ��Ϊ 64,����ֵΪ 500,���ʱ��Ϊ 1s


    //����LED0����
    xTaskCreate((TaskFunction_t )led0_task,
                (const char*    )"led0_task",
                (uint16_t       )LED0_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LED0_TASK_PRIO,
                (TaskHandle_t*  )&LED0Task_Handler);
    //����LED1����
    xTaskCreate((TaskFunction_t )led1_task,
                (const char*    )"led1_task",
                (uint16_t       )LED1_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LED1_TASK_PRIO,
                (TaskHandle_t*  )&LED1Task_Handler);
    //�����������
    xTaskCreate((TaskFunction_t )float_task,
                (const char*    )"float_task",
                (uint16_t       )FLOAT_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )FLOAT_TASK_PRIO,
                (TaskHandle_t*  )&FLOATTask_Handler);
    //��B���ʱ����
    xTaskCreate((TaskFunction_t )capture_task,
                (const char*    )"capture_task",
                (uint16_t       )CAPTURE_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )CAPTURE_TASK_PRIO,
                (TaskHandle_t*  )&CAPTURETask_Handler);
    //��ȡGPSʱ����Ϣ����
    xTaskCreate((TaskFunction_t )readgps_task,
                (const char*    )"readgps_task",
                (uint16_t       )READGPS_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )READGPS_TASK_PRIO,
                (TaskHandle_t*  )&READGPSTask_Handler);
    //����AD���ݻ���ͱȽ�����
    xTaskCreate((TaskFunction_t )datadeal_task,
                (const char*    )"datadeal_task",
                (uint16_t       )DATADEAL_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )DATADEAL_TASK_PRIO,
                (TaskHandle_t*  )&DATADEALTask_Handler);
    //�����쳣������������
    xTaskCreate((TaskFunction_t )datalatch_task,
                (const char*    )"datalatch_task",
                (uint16_t       )DATALATCH_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )DATALATCH_TASK_PRIO,
                (TaskHandle_t*  )&DATALATCHTask_Handler);
    //����Ӳ��ʱ��ÿһ���ʱ���ȡ�߳�
    xTaskCreate((TaskFunction_t )timeget_task,
                (const char*    )"timeget_task",
                (uint16_t       )TIMEGET_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TIMEGET_TASK_PRIO,
                (TaskHandle_t*  )&TIMEGETTask_Handler);

    /* ����lwIP���� */
    xTaskCreate((TaskFunction_t )lwip_demo_task,
                (const char*    )"lwip_demo_task",
                (uint16_t       )LWIP_DMEO_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LWIP_DMEO_TASK_PRIO,
                (TaskHandle_t*  )&LWIP_Task_Handler);


    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���

//     vTaskSuspend(DATADEALTask_Handler);
//	vTaskSuspend(TIMEGETTask_Handler);
//    vTaskSuspend(DATADEALTask_Handler);
//    vTaskSuspend(DATALATCHTask_Handler);
}

//LED0������
void led0_task(void *pvParameters)                           //温控任务
{
    while(1)
    {
     if(first_time==0)
     {
   	 for(int i = 0; i < 8; i++)
   	        {
   	    status[i] = SHT30_Read_Humiture_Device(&sht30_devices[i], &temp[i], &humi[i]);
   	        }
		sprintf(ping_send_tem,"mon_temp_hum.x0.val=%ld",(int32_t)(temp[0]*10));
		memcpy(ping_send_tem+strlen(ping_send_tem),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_tem,strlen(ping_send_tem),1000);
		memset(ping_send_tem,0,100);
		sprintf(ping_send_hum,"mon_temp_hum.n0.val=%ld",(int32_t)(humi[0]));
		memcpy(ping_send_hum+strlen(ping_send_hum),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hum,strlen(ping_send_hum),1000);
		memset(ping_send_hum,0,100);
		sprintf(ping_send_tem,"mon_temp_hum.x1.val=%ld",(int32_t)(temp[1]*10));
		memcpy(ping_send_tem+strlen(ping_send_tem),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_tem,strlen(ping_send_tem),1000);
		memset(ping_send_tem,0,100);
		sprintf(ping_send_hum,"mon_temp_hum.n1.val=%ld",(int32_t)(humi[1]));
		memcpy(ping_send_hum+strlen(ping_send_hum),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hum,strlen(ping_send_hum),1000);
		memset(ping_send_hum,0,100);
		sprintf(ping_send_tem,"mon_temp_hum.x2.val=%ld",(int32_t)(temp[2]*10));
		memcpy(ping_send_tem+strlen(ping_send_tem),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_tem,strlen(ping_send_tem),1000);
		memset(ping_send_tem,0,100);
		sprintf(ping_send_hum,"mon_temp_hum.n2.val=%ld",(int32_t)(humi[2]));
		memcpy(ping_send_hum+strlen(ping_send_hum),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hum,strlen(ping_send_hum),1000);
		memset(ping_send_hum,0,100);
		sprintf(ping_send_tem,"mon_temp_hum.x3.val=%ld",(int32_t)(temp[3]*10));
		memcpy(ping_send_tem+strlen(ping_send_tem),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_tem,strlen(ping_send_tem),1000);
		memset(ping_send_tem,0,100);
		sprintf(ping_send_hum,"mon_temp_hum.n3.val=%ld",(int32_t)(humi[3]));
		memcpy(ping_send_hum+strlen(ping_send_hum),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hum,strlen(ping_send_hum),1000);
		memset(ping_send_hum,0,100);
		sprintf(ping_send_tem,"mon_temp_hum.x4.val=%ld",(int32_t)(temp[4]*10));
		memcpy(ping_send_tem+strlen(ping_send_tem),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_tem,strlen(ping_send_tem),1000);
		memset(ping_send_tem,0,100);
		sprintf(ping_send_hum,"mon_temp_hum.n4.val=%ld",(int32_t)(humi[4]));
		memcpy(ping_send_hum+strlen(ping_send_hum),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hum,strlen(ping_send_hum),1000);
		memset(ping_send_hum,0,100);
		sprintf(ping_send_tem,"mon_temp_hum.x5.val=%ld",(int32_t)(temp[5]*10));
		memcpy(ping_send_tem+strlen(ping_send_tem),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_tem,strlen(ping_send_tem),1000);
		memset(ping_send_tem,0,100);
		sprintf(ping_send_hum,"mon_temp_hum.n5.val=%ld",(int32_t)(humi[5]));
		memcpy(ping_send_hum+strlen(ping_send_hum),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hum,strlen(ping_send_hum),1000);
		memset(ping_send_hum,0,100);
        // 更新温度控制
        if((start_state==0)||(door_open_state==0))                        //关机状态，风扇不启动
        {
        moter_heat_Stop();
        coda_value=0;
        last_time=0;
        convert_hours_to_hhmm_float(last_time, &last_hour, &last_min);
        }
		else if((start_state==1)&&(door_open_state==1)&&(auto_state==1))                   //开机状态，风扇启动，自动模式
		{
        TempControl_Update(&control_params);      //温控任务
		}
		else if((start_state==1)&&(door_open_state==1)&&(auto_state==0))                   //开机状态，风扇启动，手动模式
		{
		 moter_hand();     //温控任务
		}
		sprintf(ping_send_hide,"mon_temp_hum.n6.val=%ld",(int32_t)(fengshan1_speed));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_hide,"mon_temp_hum.n7.val=%ld",(int32_t)(heat_data));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_time,"main.n6.val=%ld",(int32_t)(last_hour)); //剩余时
		memcpy(ping_send_time+strlen(ping_send_time),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_time,strlen(ping_send_time),1000);
		memset(ping_send_time,0,100);
		sprintf(ping_send_time,"main.n7.val=%ld",(int32_t)(last_min));  //剩余分
		memcpy(ping_send_time+strlen(ping_send_time),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_time,strlen(ping_send_time),1000);
		memset(ping_send_time,0,100);
		sprintf(ping_send_time,"monitor.x2.val=%ld",(int32_t)(last_time*10));  //剩余秒
		memcpy(ping_send_time+strlen(ping_send_time),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_time,strlen(ping_send_time),1000);
		memset(ping_send_time,0,100);
		sprintf(ping_send_hide,"mon_temp_hum.n9.val=%ld",(int32_t)(see_detaL));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_hide,"mon_temp_hum.n10.val=%ld",(int32_t)(see_L1));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_hide,"mon_temp_hum.n11.val=%ld",(int32_t)(see_L2));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_hide,"mon_temp_hum.n12.val=%ld",(int32_t)(!Ble_link));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_hide,"banben.x0.val=%ld",(int32_t)(Soft_version));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_hide,"main.n11.val=%ld",(int32_t)(cold_data));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_hide,"leixing=%ld",(int32_t)(PCM_state));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(ping_send_hide,"cheti=%ld",(int32_t)(Car_data));
		memcpy(ping_send_hide+strlen(ping_send_hide),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_hide,strlen(ping_send_hide),1000);
		memset(ping_send_hide,0,100);
		sprintf(admin_send,"guanliyuan.admin_code.txt=\"%c%c%c%c%c%c\"",(admin_code_data[0]),(admin_code_data[1]),(admin_code_data[2]),(admin_code_data[3]),(admin_code_data[4]),(admin_code_data[5]));
		memcpy(admin_send+strlen(admin_send),ping_end_data,3);
	    HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
		memset(admin_send,0,sizeof(admin_send));
		sprintf(lock_send,"guanliyuan.lock1_code.txt=\"%c%c%c%c%c%c\"",(lock1_code_data[0]),(lock1_code_data[1]),(lock1_code_data[2]),(lock1_code_data[3]),(lock1_code_data[4]),(lock1_code_data[5]));
		memcpy(lock_send+strlen(lock_send),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)lock_send,strlen(lock_send),1000);
		memset(lock_send,0,sizeof(lock_send));
		sprintf(lock_send,"guanliyuan.lock2_code.txt=\"%c%c%c%c%c%c\"",(lock2_code_data[0]),(lock2_code_data[1]),(lock2_code_data[2]),(lock2_code_data[3]),(lock2_code_data[4]),(lock2_code_data[5]));
		memcpy(lock_send+strlen(lock_send),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)lock_send,strlen(lock_send),1000);
		memset(lock_send,0,sizeof(lock_send));
		sprintf(lock_send,"guanliyuan.lock3_code.txt=\"%c%c%c%c%c%c\"",(lock3_code_data[0]),(lock3_code_data[1]),(lock3_code_data[2]),(lock3_code_data[3]),(lock3_code_data[4]),(lock3_code_data[5]));
		memcpy(lock_send+strlen(lock_send),ping_end_data,3);
	    HAL_UART_Transmit(&huart5,(uint8_t*)lock_send,strlen(lock_send),1000);
		memset(lock_send,0,sizeof(lock_send));
		sprintf(lock_send,"guanliyuan.lock4_code.txt=\"%c%c%c%c%c%c\"",(lock4_code_data[0]),(lock4_code_data[1]),(lock4_code_data[2]),(lock4_code_data[3]),(lock4_code_data[4]),(lock4_code_data[5]));
		memcpy(lock_send+strlen(lock_send),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)lock_send,strlen(lock_send),1000);
		memset(lock_send,0,sizeof(lock_send));
		sprintf(ping_send_bat,"main.n3.val=%ld",(int32_t)(battery_soc));
		memcpy(ping_send_bat+strlen(ping_send_bat),ping_end_data,3);
		HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_bat,strlen(ping_send_bat),1000);
		memset(ping_send_bat,0,100);
		IWDG_Feed();
     }
       vTaskDelay(200);
    }
}

//LED1������
void led1_task(void *pvParameters)                           //4G任务
{

    while(1)
    {

    	if(first_time==1)
    	{
    	open_4G();
    	vTaskDelay(3000);
    	first_time=0;
    	HAL_UARTEx_ReceiveToIdle_DMA(&huart5,str5_rec,sizeof(str5_rec));
    	ping_start();
    	EC800_Init(); //联网
    	BLE_ConfigAsSlave(); //蓝牙配置
    	dis_connect=0;
    	firsttime_4g=0;
    	RTC_SetTimeFromGPS(2026,3,25,10,25,30);   //初始化一个RTC时间
    	}
    	if(first_time==0)
    	{
           if((temp_alarm_flag==1)&&(start_state==1))       //温度报警
           {
        	   music_temp();
        	   vTaskDelay(2500);
           }
           if((time_alarm_flag==1)&&(start_state==1))       //温度报警
           {
        	   music_time();
        	   vTaskDelay(2500);
           }
  		   if(read_lock1 == 0)	 // 检查门锁状态
  		      {
  			   vTaskDelay(100);
  	  		   if(read_lock1 == 0)	 // 延迟防抖
  	  		      {
  		         music_play();
  		         if(_4g_is_connecting == 0)
  		         {
  		            alarm_data(Door_alarm);
  		         }
  		           vTaskDelay(1000);
  	  		      }
  		      }
  		   if(read_lock2 == 0)   //检查门锁状态
  		     {
  			 vTaskDelay(100);
   		   if(read_lock2 == 0)    //延迟防抖
    		     {
  		        music2_play();
  		        if(_4g_is_connecting == 0)
  		         {
  		            alarm_data(Door_alarm);
  		         }
  		          vTaskDelay(1000);
  		       }
  		     }
//  		   if(lock3_state == 1 && read_lock3 == 1)   //检查门锁状态
//  		     {
//  			 vTaskDelay(100);
//   		   if(lock3_state == 1 && read_lock3 == 1)    //延迟防抖
//    		     {
//  		        music3_play();
//  		        if(_4g_is_connecting == 0)
//  		         {
//  		            alarm_data(Door_alarm);
//  		         }
//  		          vTaskDelay(2500);
//  		       }
//  		     }
    	}
		vTaskDelay(100);
    }
}

//extern channel_dateTypeDef channel;
//�����������
void float_task(void *pvParameters)  //锁任务
{

	while(1)
	{
          if(lock1_flag==1)
          {
        	  lock1_off();
        	  lock1_cnt++;
          }
          if((lock1_flag==1)&&(lock1_cnt>=200))
          {
        	  lock1_flag=0;
        	  lock1_cnt=0;
        	  lock1_on();
          }
          if(lock2_flag==1)
          {
        	  lock2_off();
        	  lock2_cnt++;
          }
          if((lock2_flag==1)&&(lock2_cnt>=200))
          {
        	  lock2_flag=0;
        	  lock2_cnt=0;
        	  lock2_on();
          }
//          if(lock3_flag==1)
//          {
//        	  lock3_off();
//        	  lock3_cnt++;
//          }
//          if((lock3_flag==1)&&(lock3_cnt>=200))
//          {
//        	  lock3_flag=0;
//        	  lock3_cnt=0;
//        	  lock3_on();
//          }
//          if(lock4_flag==1)
//          {
//        	  lock4_off();
//        	  lock4_cnt++;
//          }
//          if((lock4_flag==1)&&(lock4_cnt>=200))
//          {
//        	  lock4_flag=0;
//        	  lock4_cnt=0;
//        	  lock4_on();
//          }
         if((read_lock1 == 0)||(read_lock2 == 0))    //1 2一个打开
          {
        	 door_open_state=0;
          }
          if((read_lock1 == 1)&&(read_lock2 == 1))      //1  2都锁上
          {
        	  door_open_state=1;
          }
          if(print_KO==1)
          {
        	  print_off_cnt++;
        	  if(print_off_cnt>=180)
        	  {
        		  print_off_cnt=0;
        		  print_KO=0;
            	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);    //关闭打印机
        	  }
          }
        vTaskDelay(100);

	}
}

//��B���շ�����

void capture_task(void *pvParameters)                    //重连任务
{


	while(1)
	{


       if((firsttime_4g==0)&&(_4g_is_connecting==1))
       {
		EC800_Init();
       }
		vTaskDelay(5000);


	}
}
//��ȡGPS��Ϣ����

void readgps_task(void *pvParameters)                       //数据上报
{
//	u8 i=0;
	while(1)
	{
		        send_bat();
		        // 计数器累加
		        heart_break_con++;
//		        data_send_con++;
		        read_gps_cnt++;
		        get_sig_state++;
	            if((read_gps_cnt >= 2)&&(IAP_flag==0)) {

	                Get_time_data();
	            }

		        if((get_sig_state>=2)&&(IAP_flag==0))     //问讯信号强度
		        {
		        	ask_sig();
		        	get_sig_state=0;
		        }

		        if(_4g_is_connecting == 1) {
		            // 网络未连接时的处理
//		            if(read_gps_cnt >= 2) {
//		                ask_gps_state = 1;
////		                Get_Gps_data();
//		                read_gps_cnt = 0;
//		            }


		        }
		        else {
		            // 网络已连接时的处理

		            // 1. 处理离线数据发送
		            Offline_Sender_Process();

		            // 2. 正常的数据采集和发送

		            if((heart_break_con >= 24)&&(data_send_flag==0)&&(IAP_flag==0)) {
		                heart_break_send();
		                heart_break_con = 0;
		            }

		        }

		        vTaskDelay(5000);
		    }
}

//AD���ݵĻ���ͱȽ�
void datadeal_task(void *pvParameters)
{
	while(1)
	{
		if((uart2_state==1)||(uart5_state==1)||(uart6_state==1))
		{
		  data_deal();
		}
		IWDG_Feed();
	vTaskDelay(2);
	}

}
//�쳣���ݵ�����
void datalatch_task(void *pvParameters) //心跳重连判断任务
{
	while(1)
	{
		dis_connect++;
		vTaskDelay(1000);
		if(dis_connect>=300)   //10s一次，300s->5分钟没收到心跳回复 4G断电重连
		{
			dis_connect=0;
			_4g_is_connecting=1;
			vTaskDelay(100);
		}

	}
}

//Ӳ��ʱ��ÿһ���ʱ���ȡ�߳�
void timeget_task(void *pvParameters)        //实时读取时间进程的任务
{
    RTC_DateTime_t now;
    uint32_t current_time_abs = 0;
    uint32_t last_time_abs = 0;
    uint8_t first_flag = 1;          // 首次记录基准时间

    while (1)
    {
        // 获取当前 RTC 时间
        RTC_GetDateTime(&now);

        // 计算绝对时间戳
        current_time_abs = DateTimeToSeconds(RTC_year, RTC_month, RTC_day,
                                             RTC_hour, RTC_min, RTC_second);

        if (first_flag)
        {
            last_time_abs = current_time_abs;
            first_flag = 0;
        }
        else
        {
            if (firsttime_4g == 0)
            {
                // 判断是否已过 5 分钟（300 秒）
                if (((current_time_abs - last_time_abs) >= 300)&&(IAP_flag==0))
                {
                    last_time_abs = current_time_abs;
                    data_send_flag = 1;

                    if (_4g_is_connecting == 1)
                    {
                        wind_temp_send();   // 自动存储到 SD 卡
                    }
                    else if (_4g_is_connecting == 0)
                    {
                        if (!Offline_Sender_IsActive())
                        {
                            wind_temp_send();   // 直接发送
                        }
                    }
                    data_send_flag = 0;
                }
            }
        }

        vTaskDelay(100);   // 200ms 检查一次，不会影响 5 分钟精度
    }
}
/**
 * @brief       lwIP��������
 * @param       pvParameters : �������(δ�õ�)
 * @retval      ��
 */

void lwip_demo_task(void *pvParameters)
{

    while (1)
    {
//    	PowerKey_Hand();
    	if(power_key==0)
    	{
    		sprintf(key_ping_send,"sleep=0");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
    	}
        if(menu_key==0)                   //回到主菜单
        {
    		sprintf(key_ping_send,"sleep=0");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
    		 vTaskDelay(20);
    		sprintf(key_ping_send,"page main");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
        }
        if(change_left_key==0)            //页面向左切换
        {
    		sprintf(key_ping_send,"sleep=0");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
    		 vTaskDelay(20);
    		sprintf(key_ping_send,"page door_set");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
        }
        if(change_right_key==0)            //页面向右切换
        {
    		sprintf(key_ping_send,"sleep=0");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
    		 vTaskDelay(20);
    		sprintf(key_ping_send,"page print");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
        }
        if(enter_key==0)            //页面向右切换
        {
    		sprintf(key_ping_send,"sleep=0");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
    		 vTaskDelay(20);
    		sprintf(key_ping_send,"page banben");
    		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
    		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
    		memset(key_ping_send,0,100);
        }

        vTaskDelay(40);
    }
}
