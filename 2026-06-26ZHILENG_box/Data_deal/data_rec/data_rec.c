#include "main.h"
#include "usart.h"
#include "string.h"
#include "Auxiliary.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lock_state.h"
#include "data_send.h"
#include "gps_deal.h"
#include <stdbool.h>
#include "stdio.h"
#include "string.h"
#include "4g_con.h"
#include "data_rec.h"
#include "temperature_control.h"
#include "SD_save_init.h"
#include "rec_dealprint.h"
#include "get_sig.h"
#include "my_RTC.h"
#include "IAP.h"
#include "Ble_driver.h"
#include "temperature_control.h"
#include "time_parser.h"


//TempControlParams *params;
extern GPS_Parser_t gps_parser;
extern uint8_t str_rec[600];
extern uint8_t str5_rec[300];
uint8_t deal_data_u1[600];
uint8_t deal_deta_findhead[600];
uint8_t deal_data_u5[300];
uint8_t uart2_state=0;
uint8_t uart4_state=0;
uint8_t uart5_state=0;
uint8_t ask_gps_state=0;
uint8_t deal_remote_state=0;
int remote_received_length = 0;  // 实际接收到的数据长度
extern uint8_t gps_rx_buffer[GPS_RX_BUFFER_SIZE];
extern uint16_t dis_connect;
extern bool gps_data_ready;
extern float Set_Temp;
extern float set_proportion;
extern TaskHandle_t DATADEALTask_Handler;
extern uint8_t gps_ask_state;
extern uint16_t gps_data_length;
extern uint8_t lock1_state;
uint8_t admin_code_data[6]="123456";
uint8_t lock1_code_data[6]="111111";
uint8_t lock2_code_data[6]="222222";
uint8_t lock3_code_data[6]="333333";
uint8_t lock4_code_data[6]="444444";
uint8_t admin_send[100];
uint8_t lock_send[100];
uint8_t ping_send_gps[100];
uint8_t ping_send_rssi[100];
uint8_t lock1_flag=0;
uint8_t lock2_flag=0;
uint8_t lock3_flag=0;
uint8_t lock4_flag=0;
uint16_t simple_time=0;
extern uint8_t ping_end_data[3];
extern uint8_t at_response_ready;
extern _4G_ConnectState_t _4g_state ;
extern uint32_t _4g_last_operation_time;
extern uint8_t _4g_retry_count ;
extern uint8_t _4g_is_connecting;  // 连接中标志
extern char at_response_buffer[AT_RESPONSE_BUF_SIZE];
// 参数设置（命令 0x03）
int32_t temp_high=8;      // 温度上限
int32_t temp_low=2;       // 温度下限
int32_t warn_high=7;      // 预警上限
int32_t warn_low=3;       // 预警下限
int32_t alarm_temp=8;     // 报警温度
int32_t alarm_low=2;      // 报警下限

// 密码设置（命令 0x02）
char admin_pwd[7];       // 管理员密码（6字符+'\0'）
char door1_pwd[7];       // 1号门密码
char door2_pwd[7];       // 2号门密码
char door3_pwd[7];       // 3号门密码
char door4_pwd[7];       // 4号门密码

uint8_t temp_alarm_flag=0;
uint8_t time_alarm_flag=0;
uint8_t start_state=0;
uint8_t detaL=0;

// 报警设置（命令 0x04）
uint32_t battery_warn_low=30;      // 电量预警下限（%）
uint32_t battery_alarm_low=20;     // 电量报警下限（%）
uint32_t keepwarn_low=6;          // 保温时长不足下限（小时）
uint32_t keepwarn_serious_low=4;  // 保温时长严重不足下限（小时）
//打印时间指令
uint32_t start_year=2026;
uint32_t start_month=3;
uint32_t start_day=4;
uint32_t start_hour=5;
uint32_t start_min=2;
uint32_t end_year=2026;
uint32_t end_month=3;
uint32_t end_day=5;
uint32_t end_hour=10;
uint32_t end_min=12;
uint32_t cold_data=14;
uint32_t PCM_state=PCM_NORMAL;
uint32_t Car_data=BOX_91L;
int16_t signal_rsrp = 0;      // RSRP 值（dBm）
int16_t signal_sinr = 0;      // SINR 值（dB）
uint16_t signal_mcc = 0;      // MCC（十进制）
uint8_t  signal_mnc = 0;      // MNC（十进制）
uint32_t signal_tac = 0;      // TAC（十六进制）
uint32_t signal_cellid = 0;   // Cell ID（十六进制）
uint8_t signal_ready = 0;     // 数据就绪标志
uint8_t data_ble_lenth=0;
// 新增全局变量
char signal_rx_buffer[300]; // 根据实际情况调整大小
extern uint8_t ble_rx_buffer[300];
extern uint8_t ble_rx_complete;

uint8_t auto_state=1;
extern uint8_t ble_deal_data[300];
uint8_t IAP_flag=0;
uint8_t print_KO=0;
uint8_t print_state=0;
uint32_t ping_fengshan12_speed=0;
uint32_t ping_fengshan34_speed=0;
uint8_t uart6_state=0;
extern uint8_t key_ping_send[100];
extern uint8_t ble_send_tm;
extern uint8_t print_rec[10];
extern TempControlParams control_params;
uint8_t time_data[200];
uint8_t time_ready=0;


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	   if(huart->Instance == USART1)
	   {
		   if(print_state==1)
		   {
			   print_state=0;
			   print_KO=1;
		   }
		   HAL_UARTEx_ReceiveToIdle_IT(&huart1,print_rec,10);
	   }
	    if(huart->Instance == USART2)                          //4G的数据
	    {
	      // 新增：检查是否为 +QENG:"servingcell" 响应
	    	if(strstr((char*)str_rec, "+QENG: \"servingcell\"") != NULL)
	    	{
	    	    if(Size < sizeof(signal_rx_buffer))
	    	    {
	    	    	uart2_state=1;
	    	        memcpy(signal_rx_buffer, str_rec, Size);
	    	        signal_rx_buffer[Size] = '\0';   // 确保以 '\0' 结尾
	    	        signal_ready = 1;
	    	    }

	    	}
	        // 如果正在连接过程中，收集AT响应
	    	else if(strstr(str_rec, "+QIURC:")!=NULL)                         //远端下行数据
	        {
	        	if(strstr(str_rec, "close")!=NULL)                       //主动断联
	        	{
	        		 _4g_is_connecting = 1;
	        	}
	        	else                                                    //数据包
	        	{
	        	 uart2_state=1;
	        	 remote_received_length=Size;
	        	 deal_remote_state=1;                                   //处理远端数据标志
	        	 memcpy(deal_deta_findhead, str_rec, sizeof(str_rec));
	        	}
	        }
	        else if(strstr(str_rec, "+QGPSLOC:")!=NULL)                 //GPS数据
	        {
	        	uart2_state=1;
	        	ask_gps_state=1;//GPS数据
	            // 正常的数据处理流程（保持你原有的逻辑）
	            gps_data_length = Size;
	            gps_data_ready = true;
	            memcpy(gps_rx_buffer, str_rec, sizeof(str_rec));
	        }
	    	else if(strstr((char*)str_rec, "CCLK") != NULL)
	    	{
	    	    if(Size < sizeof(time_data))
	    	    {
	    	    	uart2_state=1;
	    	        memcpy(time_data, str_rec, Size);
	    	        time_data[Size] = '\0';   // 确锟斤拷锟斤拷 '\0' 锟斤拷尾
	    	        time_ready = 1;
	    	    }

	    	}
	        else                                                                 //AT其他指令处理
	        {
	            if(Size < AT_RESPONSE_BUF_SIZE - 1)
	            {
	                memcpy(at_response_buffer + strlen(at_response_buffer), str_rec, Size);
	                at_response_buffer[strlen(at_response_buffer) + Size] = '\0';
	                at_response_ready = 1;
	            }
	        }
	        memset(str_rec, 0, sizeof(str_rec));
	        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, str_rec, sizeof(str_rec));
	    }
	if(huart->Instance == UART5)    //屏的数据
	{
	uart5_state=1;
    memcpy(deal_data_u5,str5_rec,sizeof(str5_rec));
    memset(str5_rec,0,sizeof(str5_rec));
	HAL_UARTEx_ReceiveToIdle_DMA(&huart5,str5_rec,sizeof(str5_rec));
	}
	if (huart->Instance == USART6) {
	    ble_rx_complete = 1;

	    // 将本次收到的所有字节压入环形缓冲
	    if((ble_rx_buffer[0]==0xA5)&&(ble_rx_buffer[1]==0x5A))
	    {
	    	uart6_state=1;
	    	memcpy(ble_deal_data,ble_rx_buffer,300);
	    }
	    // 注意：不要在这里调用 ble_parse_frame()，以免中断执行时间过长
	    // 而是在主循环中反复调用解析函数

	    // 重新启动 DMA 接收
	    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, ble_rx_buffer, 300);
	}

}

u8 dat_rec=0;
u8 data_lenth=0;
u8 test_xor=0;
u8 test_sum=0;

/**
 * @brief 在接收数据中寻找包头并复制数据
 * @param src 源数据数组（接收到的数据）
 * @param src_len 源数据长度
 * @param dest 目标数据数组
 * @param max_dest_len 目标数组最大长度
 * @return 成功复制的数据长度，-1表示未找到包头
 */
int find_header_and_copy(uint8_t *src, int src_len, uint8_t *dest, int max_dest_len)
{
    int found_pos = -1;

    for(int i = 0; i < src_len - 5; i++)
    {
        if(src[i] == HEADER_BYTE1 && src[i + 1] == HEADER_BYTE2 &&
           src[i + 2] == HEADER_BYTE3 && src[i + 3] == HEADER_BYTE4 &&
           src[i + 4] == HEADER_BYTE5 && src[i + 5] == HEADER_BYTE6)
        {
            found_pos = i;
            break;
        }
    }

    if(found_pos == -1)
    {
        return -1;
    }

    int copy_len = src_len - found_pos;

    if(copy_len > max_dest_len)
    {
        copy_len = max_dest_len;
    }

    memcpy(dest, &src[found_pos], copy_len);

    return copy_len;
}

void data_deal(void)
{

  if(uart2_state==1)   //4G收到数据
  {
	  if(deal_remote_state==1)//不是问讯GPS，数据正常收发
	  {
		    // 寻找包头并复制数据
         int copied_len = find_header_and_copy(deal_deta_findhead, remote_received_length, deal_data_u1, sizeof(deal_data_u1));
         if(copied_len>0)
         {
	       if(check_pkt_header(deal_data_u1) && check_pkt_address(deal_data_u1))
	       {
		      data_lenth=deal_data_u1[PKT_LEN_POS]+7;
		     if((deal_data_u1[data_lenth-2]==XOR(deal_data_u1,data_lenth-9,7))&&(deal_data_u1[data_lenth-1]==CheckSum8(deal_data_u1,data_lenth-9,7)))
		      {
              switch (deal_data_u1[PKT_CMD_POS])
              {
                  case 0x80:
                	  dis_connect=0;
                      break;

                  case 0x81:
                	  dat_rec=0;
                      break;

                  case 0x82:

                      break;

                  case 0x83:

                      break;
                  case 0x84:
                       if(deal_data_u1[PKT_DATA_POS]==0x01)
                       {
                    	   if(deal_data_u1[PKT_DATA_POS+1]==0x01)
                    	   {
                  			  lock1_flag=1;
                  			  door_open(0x01,0x01);
                           }
                    	}
                       if(deal_data_u1[PKT_DATA_POS]==0x02)
                       {
                    	   if(deal_data_u1[PKT_DATA_POS+1]==0x01)
                    	   {
                  			  lock2_flag=1;
                  			  door_open(0x02,0x01);
                           }
                       }
                       if(deal_data_u1[PKT_DATA_POS]==0x03)
                       {
                    	   if(deal_data_u1[PKT_DATA_POS+1]==0x01)
                    	   {
                  			  lock3_flag=1;
                  			  door_open(0x03,0x01);
                           }
                       }
                       if(deal_data_u1[PKT_DATA_POS]==0x04)
                            {
                         	   if(deal_data_u1[PKT_DATA_POS+1]==0x01)
                         	   {
                         		   lock4_on();
                                }
                            }
                	  break;
                  case 0x85:
                	      wind_temp_send();
                	  break;
                  case 0x86:

                	  break;
                  case 0x87:
                	  temp_high=deal_data_u1[PKT_DATA_POS];
            	      sprintf(admin_send,"set_temp.n0.val=%ld",(uint32_t)(temp_high));
            	      memcpy(admin_send+strlen(admin_send),ping_end_data,3);
            	      HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
            	      memset(admin_send,0,sizeof(admin_send));
            	      temp_low=deal_data_u1[PKT_DATA_POS+1];
            	      sprintf(admin_send,"set_temp.n1.val=%ld",(uint32_t)(temp_low));
            	      memcpy(admin_send+strlen(admin_send),ping_end_data,3);
            	      HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
            	      memset(admin_send,0,sizeof(admin_send));
            	      warn_high=deal_data_u1[PKT_DATA_POS+2];
            	      sprintf(admin_send,"set_temp.n2.val=%ld",(uint32_t)(warn_high));
            	      memcpy(admin_send+strlen(admin_send),ping_end_data,3);
            	      HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
            	      memset(admin_send,0,sizeof(admin_send));
            	      warn_low=deal_data_u1[PKT_DATA_POS+3];
            	      sprintf(admin_send,"set_temp.n3.val=%ld",(uint32_t)(warn_low));
            	      memcpy(admin_send+strlen(admin_send),ping_end_data,3);
            	      HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
            	      memset(admin_send,0,sizeof(admin_send));
            	      TempControl_SetTemperature(&control_params, (float)(temp_low), (float)(temp_high));
            	      SaveConfigToSD();
            	      temp_set_OK();
                	  break;
                  case 0x88:
                	  if(deal_data_u1[PKT_DATA_POS]==0x00)
                	  {
                       memcpy(admin_code_data,&deal_data_u1[PKT_DATA_POS+1],6);
                	  }
                	  if(deal_data_u1[PKT_DATA_POS]==0x01)
                	  {
                       memcpy(lock1_code_data,&deal_data_u1[PKT_DATA_POS+1],6);
                	  }
                	  if(deal_data_u1[PKT_DATA_POS]==0x02)
                	  {
                       memcpy(lock2_code_data,&deal_data_u1[PKT_DATA_POS+1],6);
                	  }
                	  if(deal_data_u1[PKT_DATA_POS]==0x03)
                	  {
                       memcpy(lock3_code_data,&deal_data_u1[PKT_DATA_POS+1],6);
                	  }
                	  if(deal_data_u1[PKT_DATA_POS]==0x04)
                	  {
                       memcpy(lock4_code_data,&deal_data_u1[PKT_DATA_POS+1],6);
                	  }
                	  SaveConfigToSD();
                       code_OK();
                	  break;
                  case 0x89:
                	  vision_send();
                	  break;
                  case 0x8A:

                	  break;
                  case 0x8B:
                	  if(deal_data_u1[PKT_DATA_POS]==0x00)
                	  {
                		  code_admin_ask();

                	  }
                	  if(deal_data_u1[PKT_DATA_POS]==0x01)
                	  {
                		  code_lock_ask();
                	  }
                	  break;
                  case 0x8C:
              	    uint16_t total_packets = (deal_data_u1[PKT_DATA_POS] << 8) | deal_data_u1[PKT_DATA_POS+1];
              	    uint16_t current_packet = (deal_data_u1[PKT_DATA_POS+2] << 8) | deal_data_u1[PKT_DATA_POS+3];
              	    uint8_t *data_ptr = &deal_data_u1[PKT_DATA_POS+4];
              	    IAP_flag=1;
              	    dis_connect=0;
              	    IAP_ProcessUpgrade(total_packets, current_packet, data_ptr, 220);
                	 break;
                  case 0x8D:
            	      memcpy(&battery_warn_low, &deal_data_u1[PKT_DATA_POS], 4);
            	      sprintf(admin_send,"dianliang.n0.val=%ld",(uint32_t)(battery_warn_low));
            	      memcpy(admin_send+strlen(admin_send),ping_end_data,3);
            	      HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
            	      memset(admin_send,0,sizeof(admin_send));
            	      memcpy(&battery_alarm_low, &deal_data_u1[PKT_DATA_POS+4], 4);
            	      sprintf(admin_send,"dianliang.n1.val=%ld",(uint32_t)(battery_alarm_low));
            	      memcpy(admin_send+strlen(admin_send),ping_end_data,3);
            	      HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
            	      memset(admin_send,0,sizeof(admin_send));
            	      memcpy(&keepwarn_low, &deal_data_u1[PKT_DATA_POS+8], 4);
            	      sprintf(admin_send,"dianliang.n2.val=%ld",(uint32_t)(keepwarn_low));
            	      memcpy(admin_send+strlen(admin_send),ping_end_data,3);
            	      HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
            	      memset(admin_send,0,sizeof(admin_send));
            	      memcpy(&keepwarn_serious_low, &deal_data_u1[PKT_DATA_POS+12], 4);
            	      sprintf(admin_send,"dianliang.n3.val=%ld",(uint32_t)(keepwarn_serious_low));
            	      memcpy(admin_send+strlen(admin_send),ping_end_data,3);
            	      HAL_UART_Transmit(&huart5,(uint8_t*)admin_send,strlen(admin_send),1000);
            	      memset(admin_send,0,sizeof(admin_send));
            	      SaveConfigToSD();
            	      alarm_set_OK();
                	  break;
                  case 0x8E:
                	  if(deal_data_u1[PKT_DATA_POS]==0x01)
                	  {
                		  temp_alarm_flag=1;
                	  }
                	  else if(deal_data_u1[PKT_DATA_POS]==0x00)
                	  {
                		  temp_alarm_flag=0;
                	  }
                	  music_temp_OK();
                	  break;
                  case 0x8F:
                	  if(deal_data_u1[PKT_DATA_POS]==0x01)
                	  {
                		  time_alarm_flag=1;
                	  }
                	  else if(deal_data_u1[PKT_DATA_POS]==0x00)
                	  {
                		  time_alarm_flag=0;
                	  }
                	  music_time_OK();
                	  break;
                  case 0x90:

                	  break;
                  case 0x94:
                	  detaL=deal_data_u1[PKT_DATA_POS];
                	  TempControl_UpdateDeltaL(&control_params, (float)detaL);
				      break;
                  case 0x95:
                      HAL_NVIC_SystemReset();
                	  break;
               }
		     }
	      }
        }
	  deal_remote_state=0;
	  memset(deal_data_u1,0,sizeof(deal_data_u1));
    }
//	  if(ask_gps_state==1) //问讯GPS，定位经纬度
//	  {
//		if(gps_ask_state==1)           //解析GPS数据
//		  {
//
//			char* gps1_string = (char*)gps_rx_buffer;
//			 if(strstr(gps1_string,"+QGPSLOC:")!=NULL)
//			  {
//				 ask_gps_state=0;
//				process_gps_data();
//				sprintf(ping_send_gps,"rtc0=%d",(int32_t)(gps_parser.data.time.year));
//				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
//				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
//				memset(ping_send_gps,0,100);
//				sprintf(ping_send_gps,"rtc1=%d",(int32_t)(gps_parser.data.time.month));
//				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
//				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
//				memset(ping_send_gps,0,100);
//				sprintf(ping_send_gps,"rtc2=%d",(int32_t)(gps_parser.data.time.day));
//				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
//				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
//				memset(ping_send_gps,0,100);
//				sprintf(ping_send_gps,"rtc3=%d",(int32_t)(gps_parser.data.time.local_hour));
//				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
//				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
//				memset(ping_send_gps,0,100);
//				sprintf(ping_send_gps,"rtc4=%d",(int32_t)(gps_parser.data.time.local_minute));
//				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
//				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
//				memset(ping_send_gps,0,100);
//				sprintf(ping_send_gps,"rtc5=%d",(int32_t)(gps_parser.data.time.local_second));
//				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
//				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
//				memset(ping_send_gps,0,100);
//			    RTC_SetTimeFromGPS(gps_parser.data.time.year,gps_parser.data.time.month,gps_parser.data.time.day,gps_parser.data.time.local_hour,gps_parser.data.time.local_minute,gps_parser.data.time.local_second);
//				gps_ask_state++;
//			  }
//		  }
//
//	  }
		if(time_ready==1)
		{
			  beijing_time_t bj_time;
			  parse_cck_time((char*)time_data, &bj_time);
			  sprintf(ping_send_gps,"rtc0=%d",(int32_t)(bj_time.year));
			  				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
			  				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
			  				memset(ping_send_gps,0,100);
			  				sprintf(ping_send_gps,"rtc1=%d",(int32_t)(bj_time.month));
			  				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
			  				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
			  				memset(ping_send_gps,0,100);
			  				sprintf(ping_send_gps,"rtc2=%d",(int32_t)(bj_time.day));
			  				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
			  				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
			  				memset(ping_send_gps,0,100);
			  				sprintf(ping_send_gps,"rtc3=%d",(int32_t)(bj_time.hour));
			  				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
			  				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
			  				memset(ping_send_gps,0,100);
			  				sprintf(ping_send_gps,"rtc4=%d",(int32_t)(bj_time.minute));
			  				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
			  				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
			  				memset(ping_send_gps,0,100);
			  				sprintf(ping_send_gps,"rtc5=%d",(int32_t)(bj_time.second));
			  				memcpy(ping_send_gps+strlen(ping_send_gps),ping_end_data,3);
			  				HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_gps,strlen(ping_send_gps),1000);
			  				memset(ping_send_gps,0,100);
			  			    RTC_SetTimeFromGPS(bj_time.year,bj_time.month,bj_time.day,bj_time.hour,bj_time.minute,bj_time.second);
			  time_ready=0;
		}
	  if(signal_ready==1)
	  {
		  process_signal();
		  sprintf(ping_send_rssi,"mon_temp_hum.n8.val=%d",(int32_t)(signal_rsrp));
		  memcpy(ping_send_rssi+strlen(ping_send_rssi),ping_end_data,3);
		  HAL_UART_Transmit(&huart5,(uint8_t*)ping_send_rssi,strlen(ping_send_rssi),1000);
		  memset(ping_send_rssi,0,100);
		  signal_ready=0;
	  }
	  uart2_state=0;
  }
  if(uart5_state==1)
  {
	  if(deal_data_u5[0]==0x68)
	  {
  		sprintf(key_ping_send,"sleep=0");
  		memcpy(key_ping_send+strlen(key_ping_send),ping_end_data,3);
  		HAL_UART_Transmit(&huart5,(uint8_t*)key_ping_send,strlen(key_ping_send),1000);
  		memset(key_ping_send,0,100);
	  }
	  else
	  {
	  if(deal_data_u5[2]==0x05)
	  {
	      // 打印起始时间-年
	      memcpy(&start_year, &deal_data_u5[4], 4);
	      // 打印起始时间-月
	      memcpy(&start_month, &deal_data_u5[9], 4);
	      // 打印起始时间-日
	      memcpy(&start_day, &deal_data_u5[14], 4);
	      // 打印起始时间-时
	      memcpy(&start_hour, &deal_data_u5[19], 4);
	      // 打印起始时间-分
	      memcpy(&start_min, &deal_data_u5[24], 4);
	      // 打印结束时间-年
	      memcpy(&end_year, &deal_data_u5[29], 4);
	      // 打印结束时间-月
	      memcpy(&end_month, &deal_data_u5[34], 4);
	      // 打印结束时间-日
	      memcpy(&end_day, &deal_data_u5[39], 4);
	      // 打印结束时间-时
	      memcpy(&end_hour, &deal_data_u5[44], 4);
	      // 打印结束时间-分
	      memcpy(&end_min, &deal_data_u5[49], 4);

      print_from_sd();
   }
      else if(deal_data_u5[2]==0x04) // 报警设置
	  {
	      // 电量预警下限：F0 之后 4 字节，偏移 4
	      memcpy(&battery_warn_low, &deal_data_u5[4], 4);
	      // 电量报警下限：F1 之后 4 字节，偏移 9
	      memcpy(&battery_alarm_low, &deal_data_u5[9], 4);
	      // 保温时长不足下限：F2 之后 4 字节，偏移 14
	      memcpy(&keepwarn_low, &deal_data_u5[14], 4);
	      // 保温时长严重不足下限：F3 之后 4 字节，偏移 19
	      memcpy(&keepwarn_serious_low, &deal_data_u5[19], 4);
	      SaveConfigToSD();
	  }

      else if(deal_data_u5[2]==0x03) // 参数设置
	  {
	      // 温度上限：F0 之后 4 字节，偏移 4
	      memcpy(&temp_high, &deal_data_u5[4], 4);
	      // 温度下限：F1 之后 4 字节，偏移 9
	      memcpy(&temp_low, &deal_data_u5[9], 4);
	      // 预警上限：F2 之后 4 字节，偏移 14
	      memcpy(&warn_high, &deal_data_u5[14], 4);
	      // 预警下限：F3 之后 4 字节，偏移 19
	      memcpy(&warn_low, &deal_data_u5[19], 4);
	      // 报警温度：F4 之后 4 字节，偏移 24
	      memcpy(&alarm_temp, &deal_data_u5[24], 4);
	      // 报警下限：F5 之后 4 字节，偏移 29
	      memcpy(&alarm_low, &deal_data_u5[29], 4);

	      temp_high-=100;
	      temp_low-=100;
	      warn_high-=100;
	      warn_low-=100;
	      alarm_temp-=100;
	      alarm_low-=100;

	      SaveConfigToSD();

	      TempControl_SetTemperature(&control_params,(float)(temp_low),(float)(temp_high));
	  }

      else if(deal_data_u5[2]==0x02) // 密码设置
	  {
	      // 管理员密码：F0 之后 6 字节，偏移 4
	      memcpy(admin_pwd, &deal_data_u5[4], 6);
	      admin_pwd[6] = '\0';  // 添加字符串结束符
	      // 1号门密码：F1 之后 6 字节，偏移 11
	      memcpy(door1_pwd, &deal_data_u5[11], 6);
	      door1_pwd[6] = '\0';
	      // 2号门密码：F2 之后 6 字节，偏移 18
	      memcpy(door2_pwd, &deal_data_u5[18], 6);
	      door2_pwd[6] = '\0';
	      // 3号门密码：F3 之后 6 字节，偏移 25
	      memcpy(door3_pwd, &deal_data_u5[25], 6);
	      door3_pwd[6] = '\0';
	      // 4号门密码：F4 之后 6 字节，偏移 32
	      memcpy(door4_pwd, &deal_data_u5[32], 6);
	      door4_pwd[6] = '\0';
	  }
	  else if(deal_data_u5[2]==0x01)                  //开关锁标志
	  {
		  if(deal_data_u5[3]==0x01)             //1开锁
		  {
			  lock1_flag=1;
			  door_open(0x01,0x01);
		  }
		  if(deal_data_u5[3]==0x03)             //2开锁
		  {
			  lock2_flag=1;
			  door_open(0x02,0x01);
		  }
		  if(deal_data_u5[3]==0x05)             //3开锁
		  {
			  lock3_flag=1;
			  door_open(0x03,0x01);
		  }
		  if(deal_data_u5[3]==0x07)             //4开锁
		  {
			  lock4_flag=1;
		  }
	  }
	  else if(deal_data_u5[2]==0x06)        //开关机
	  {
		  if(deal_data_u5[3]==0x01)
		  {
		     start_state=1;
		  }
		  if(deal_data_u5[3]==0x00)
		  {
			  start_state=0;
		  }
		  SaveConfigToSD();
	  }
	  else if(deal_data_u5[2]==0x07)      //箱体参数
	  {
		  memcpy(&cold_data,&deal_data_u5[3],4);
		  // PCM类型: 0x01=常规PCM, 0x02=低温PCM
		  if(deal_data_u5[7]==0x01)
		  {
			  PCM_state=PCM_NORMAL;
		  }
		  else if(deal_data_u5[7]==0x02)
		  {
			  PCM_state=PCM_LOW;
		  }
		  // 箱体类型: 0x0A=91L, 0x07=91L加厚, 0x08=51L, 0x09=51L加厚
		  if(deal_data_u5[8]==0x0A)
		  {
			  Car_data=BOX_91L;
		  }
		  else if(deal_data_u5[8]==0x07)
		  {
			  Car_data=BOX_91L_THICK;
		  }
		  else if(deal_data_u5[8]==0x08)
		  {
			  Car_data=BOX_51L;
		  }
		  else if(deal_data_u5[8]==0x09)
		  {
			  Car_data=BOX_51L_THICK;
		  }
		  SaveConfigToSD();
		  // 设置箱体参数
		  TempControl_SetBoxType(&control_params, Car_data);
		  // 设置PCM参数
		  TempControl_SetPCMType(&control_params, PCM_state, cold_data);
	  }
	  else if(deal_data_u5[2]==0x08)
	  {
          if(deal_data_u5[3]==0x01)   //自动模式
          {
        	  auto_state=1;
          }
          else if(deal_data_u5[3]==0x02) //手动模式
          {
        	  auto_state=0;
        	  memcpy(&ping_fengshan12_speed,&deal_data_u5[4],4);
        	  memcpy(&ping_fengshan34_speed,&deal_data_u5[8],4);
          }
	  }
	  }
	  uart5_state=0;
	  memset(deal_data_u5,0,sizeof(deal_data_u5));
  }
  if(uart6_state==1)              //蓝牙打印数据
  {
      if((ble_deal_data[0]==0xA5)&&(ble_deal_data[1]==0x5A)) //包头且地址正确
      {
    	  data_ble_lenth=ble_deal_data[2]+3;//数据长度，用于计算校验
	     if((ble_deal_data[data_ble_lenth-2]==XOR(ble_deal_data,data_ble_lenth-5,3))&&(ble_deal_data[data_ble_lenth-1]==CheckSum8(ble_deal_data,data_ble_lenth-5,3)))  //数据校验
	     {
	    	 switch (ble_deal_data[4])
	    	 {
	    	 case 0x90:
	    	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);    //打开打印机
           	  handle_print_command(ble_deal_data);
           	  break;
	    	 }
	     }
      }
      memset(ble_deal_data,0,300);
      uart6_state=0;
  }

}
