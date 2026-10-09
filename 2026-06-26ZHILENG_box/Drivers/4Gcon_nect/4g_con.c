#include "main.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "4g_con.h"


// 超时时间定义（单位：毫秒）
#define AT_TIMEOUT_MS          5000    // AT命令超时5秒
#define CPIN_TIMEOUT_MS        10000   // SIM卡检测超时10秒
#define CREG_TIMEOUT_MS        30000   // 网络注册超时30秒
#define CGREG_TIMEOUT_MS       30000   // GPRS注册超时30秒
#define QIOPEN_TIMEOUT_MS      30000   // TCP连接超时30秒
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
char str1[]="AT+CPIN?\r\n";
char str2[]="AT+QURCCFG=\"urcport\",\"uart1\"\r\n";
char str3[]="AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n";
char str4[]="AT+QIOPEN=1,0,\"TCP\",\"36.137.239.126\",9003,0,2\r\n";
char str5[]="+++";
char str6[]="AT+QICLOSE=0\r\n";
char str7[]="AT+QGPS=1\r\n";   //打开GPS
char str8[]="AT+QGPSLOC?\r\n"; //问讯数据
char str9[]="AT+QGPSEND\r\n";  //关GPS
char str10[]="AT+QENG=\"servingcell\"\r\n";
char str11[]="AT+CCLK?\r\n";

// 超时检测宏
#define IS_TIMEOUT(start_time, timeout_ms) \
    ((xTaskGetTickCount() - (start_time)) >= pdMS_TO_TICKS(timeout_ms))


const char* at_commands[] = {
    "AT\r\n",                                      // 0: 测试模块就绪 - 期待"OK"
    "ATE0\r\n",                                    // 1: 关闭回显 - 期待"OK"
    "AT+CMEE=2\r\n",                              // 2: 开启详细错误报告 - 期待"OK"
    "AT+CPIN?\r\n",                               // 3: 查询SIM卡状态 - 期待"+CPIN:READY"
    "AT+CEREG?\r\n",                              // 4: 查询4G网络注册 - 期待"+CEREG:0,1"或"+CEREG:0,5"
    "AT+QURCCFG=\"urcport\",\"uart1\"\r\n",       // 5: 设置URC端口 - 期待"OK"（非常重要！）
    "AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n",    // 6: 配置APN - 期待"OK"
    "AT+QIACT=1\r\n",                             // 7: 激活上下文 - 期待"OK"
    "AT+QIACT?\r\n",                              // 8: 查询激活状态 - 期待"+QIACT:"
    "AT+QIOPEN=1,0,\"TCP\",\"36.137.239.126\",9003,0,2\r\n"  // 9: 打开TCP连接 - 期待"CONNECT"
};
uint8_t gps_ask_state=0;
extern uint8_t ask_gps_state;
extern uint16_t dis_connect;
char at_response_buffer[AT_RESPONSE_BUF_SIZE];
uint8_t at_response_ready = 0;
_4G_ConnectState_t _4g_state = _4G_IDLE;
uint32_t _4g_last_operation_time = 0;
uint8_t _4g_retry_count = 0;
uint8_t gps_lose_cnt=0;
uint8_t _4g_is_connecting = 1;  // 连接中标志
char *strx=0,*extstrx,*Readystrx,*Errstrx; 	//·µ»ØÖµÖ¸ÕëÅÐ¶Ï
// ============== AT响应检查函数 ==============
uint8_t check_at_response(const char* expected) {
    if(at_response_ready) {
        uint8_t result = (strstr(at_response_buffer, expected) != NULL);
        at_response_ready = 0;
        memset(at_response_buffer, 0, AT_RESPONSE_BUF_SIZE);
        return result;
    }
    return 0;
}

// ============== 正确的4G连接状态机 ==============
void _4G_Connect_StateMachine(void)
{
    static uint32_t last_check_time = 0;
    uint32_t current_time = HAL_GetTick();

    // 状态机执行间隔（100ms）
    if(current_time - last_check_time < 100) return;
    last_check_time = current_time;

    switch(_4g_state) {
        case _4G_IDLE:
            // 等待启动连接
            break;

        case _4G_INIT_CHECK_READY:
            if(_4g_retry_count == 0) {
                // 第一次发送AT命令
                HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[0], strlen(at_commands[0]), 1000);
                _4g_last_operation_time = current_time;
                _4g_retry_count++;
//                printf("[4G] Checking module ready...\n");
            }
            else if(current_time - _4g_last_operation_time > 1000) {
                // 检查响应
                if(check_at_response("OK")) {
                    _4g_state = _4G_INIT_SET_ECHO;
                    _4g_retry_count = 0;
//                    printf("[4G] Module ready\n");
                }
                else if(_4g_retry_count < 3) {
                    // 重试
                    HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[0], strlen(at_commands[0]), 1000);
                    _4g_last_operation_time = current_time;
                    _4g_retry_count++;
//                    printf("[4G] Retry %d\n", _4g_retry_count);
                }
                else {
                    _4g_state = _4G_ERROR;
//                    printf("[4G] ERROR: Module not responding\n");
                }
            }
            break;

        case _4G_INIT_SET_ECHO:
            HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[1], strlen(at_commands[1]), 1000);
            _4g_last_operation_time = current_time;
            _4g_state = _4G_INIT_SET_ERROR_REPORT;
//            printf("[4G] Setting echo off\n");
            break;

        case _4G_INIT_SET_ERROR_REPORT:
            if(current_time - _4g_last_operation_time > 500) {
                if(check_at_response("OK")) {
                    _4g_state = _4G_CHECK_SIM_CARD;
                    _4g_retry_count = 0;
//                    printf("[4G] Error report enabled\n");
                }
            }
            break;

        case _4G_CHECK_SIM_CARD:
            HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[3], strlen(at_commands[3]), 1000);
            _4g_last_operation_time = current_time;
            _4g_state = _4G_CHECK_NETWORK_REG;
//            printf("[4G] Checking SIM card\n");
            break;

        case _4G_CHECK_NETWORK_REG:
            if(current_time - _4g_last_operation_time > 500) {
                // 检查SIM卡响应：+CPIN:READY
                if(check_at_response("READY")) {
                    // SIM卡正常，查询4G网络注册
                    HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[4], strlen(at_commands[4]), 1000);
                    _4g_last_operation_time = current_time;
                    _4g_retry_count = 0;
                    _4g_state = _4G_SET_URC_PORT;
//                    printf("[4G] SIM ready, checking 4G network registration\n");
                }
                else if(check_at_response("ERROR")) {
                    _4g_state = _4G_ERROR;
//                    printf("[4G] ERROR: SIM card error\n");
                }
            }
            break;

        case _4G_SET_URC_PORT:
            // 检查4G网络注册响应：+CEREG:0,1 或 +CEREG:0,5
            if(current_time - _4g_last_operation_time > 500) {
                if(check_at_response("0,1") || check_at_response("0,5")) {
                    // 4G网络注册成功，设置URC端口（必须步骤！）
                    HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[5], strlen(at_commands[5]), 1000);
                    _4g_last_operation_time = current_time;
                    _4g_state = _4G_CONFIG_APN;
//                    printf("[4G] 4G network registered, setting URC port to uart1\n");
                }
                else if(_4g_retry_count < 30) {
                    // 等待网络注册，最多15秒（30*500ms）
                    if(_4g_retry_count % 5 == 0) {
                        // 每2.5秒重发查询命令
                        HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[4], strlen(at_commands[4]), 1000);
//                        printf("[4G] Waiting for network registration...\n");
                    }
                    _4g_retry_count++;
                }
                else {
                    _4g_state = _4G_ERROR;
//                    printf("[4G] ERROR: Network registration timeout\n");
                }
            }
            break;

        case _4G_CONFIG_APN:
            if(current_time - _4g_last_operation_time > 500) {
                // 检查URC端口设置响应：OK
                if(check_at_response("OK")) {
                    // URC端口设置成功，配置APN
                    HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[6], strlen(at_commands[6]), 1000);
                    _4g_last_operation_time = current_time;
                    _4g_state = _4G_ACTIVATE_CONTEXT;
//                    printf("[4G] URC port set, configuring APN\n");
                }
            }
            break;


        case _4G_ACTIVATE_CONTEXT:
            if(current_time - _4g_last_operation_time > 500) {
                if(check_at_response("OK")) {
                    // APN配置成功，激活上下文
                    HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[7], strlen(at_commands[7]), 1000);
                    _4g_last_operation_time = current_time;
                    _4g_state = _4G_CHECK_ACTIVATION;
//                    printf("[4G] Activating context\n");
                }
            }
            break;

        case _4G_CHECK_ACTIVATION:
            if(current_time - _4g_last_operation_time > 500) {
//                if(check_at_response("OK")) {
                    // 上下文激活成功，查询IP确认
                    HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[8], strlen(at_commands[8]), 1000);
                    _4g_last_operation_time = current_time;
                    _4g_state = _4G_OPEN_TCP;
//                    printf("[4G] Context activated, checking IP\n");
//                }
            }
            break;

        case _4G_OPEN_TCP:
            if(current_time - _4g_last_operation_time > 500) {
                if(check_at_response("+QIACT:")) {
                    // 有IP地址，可以打开TCP连接
                    HAL_UART_Transmit(&huart2, (uint8_t*)at_commands[9], strlen(at_commands[9]), 1000);
                    _4g_last_operation_time = current_time;
                    _4g_state = _4G_CONNECTED;
//                    printf("[4G] Opening TCP connection\n");
                }
            }
            break;

        case _4G_CONNECTED:
            if(current_time - _4g_last_operation_time > 10000) {
                if(check_at_response("CONNECT")) {
                    // TCP连接成功
                    _4g_is_connecting = 0;
                    dis_connect = 0;
                    _4g_state = _4G_IDLE;
//                    printf("[4G] TCP Connected Successfully!\n");
                }
                else {
                    _4g_state = _4G_ERROR;
//                    printf("[4G] ERROR: TCP connection failed\n");
                }
            }
            break;

        case _4G_ERROR:
            // 错误处理
            _4g_is_connecting = 0;
            _4g_state = _4G_IDLE;
            _4g_retry_count = 0;
//            printf("[4G] Connection error, resetting\n");
            break;
    }
}

// ============== 外部调用接口 ==============
void Start_4G_Connection(void)
{
    if(_4g_state == _4G_IDLE) {
        _4g_state = _4G_INIT_CHECK_READY;
        _4g_is_connecting = 1;
        _4g_retry_count = 0;
        memset(at_response_buffer, 0, AT_RESPONSE_BUF_SIZE);
//        printf("Starting 4G Connection...\r\n");
    }
}

// 清空接收缓冲区函数（修改为HAL库版本）
void Clear_Buffer(void)
{
    // 清空缓冲区
    memset(at_response_buffer, 0, AT_RESPONSE_BUF_SIZE);

}

// AT命令发送函数（替代printf）
void AT_SendCommand(const char* cmd)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), 1000);
}


// EC800初始化函数（HAL库版本）
void EC800_Init(void)
{
    TickType_t start_time;
    uint8_t init_success = 1;  // 初始化成功标志，假设能成功，如果不成功，就是0

    Clear_Buffer();

    // 发送AT测试模块
    AT_SendCommand("AT\r\n");
    vTaskDelay(500);

    AT_SendCommand("AT\r\n");
    vTaskDelay(500);

    // 等待OK响应（带超时）
    start_time = xTaskGetTickCount();
    strx = strstr((const char*)at_response_buffer, "OK");
    while(strx == NULL)
    {
        if(IS_TIMEOUT(start_time, AT_TIMEOUT_MS))
        {
            // AT命令超时，标记初始化失败
            init_success = 0;
            _4g_is_connecting = 1;
            break;
        }

        Clear_Buffer();
        AT_SendCommand("AT\r\n");
        vTaskDelay(500);
        strx = strstr((const char*)at_response_buffer, "OK");
    }

    // 如果AT命令失败，直接返回
    if(!init_success) return;

    // 关闭回显
    AT_SendCommand("ATE0\r\n");
    vTaskDelay(500);
    Clear_Buffer();

    // 检查信号质量
    AT_SendCommand("AT+CSQ\r\n");
    vTaskDelay(500);

    // 检查SIM卡状态（带超时）
    AT_SendCommand("AT+CPIN?\r\n");
    vTaskDelay(500);

    start_time = xTaskGetTickCount();
    strx = strstr((const char*)at_response_buffer, "+CPIN: READY");
    while(strx == NULL)
    {
        if(IS_TIMEOUT(start_time, CPIN_TIMEOUT_MS))
        {
            init_success = 0;
            _4g_is_connecting = 1;
            break;
        }

        Clear_Buffer();
        AT_SendCommand("AT+CPIN?\r\n");
        vTaskDelay(500);
        strx = strstr((const char*)at_response_buffer, "+CPIN: READY");
    }

    if(!init_success) return;
    Clear_Buffer();

    // 断开连接
    AT_SendCommand("AT+QICLOSE=0\r\n");
    vTaskDelay(500);

    // 检查GSM网络注册（带超时）
    AT_SendCommand("AT+CREG?\r\n");
    vTaskDelay(500);

    start_time = xTaskGetTickCount();
    strx = strstr((const char*)at_response_buffer, "+CREG: 0,1");
    extstrx = strstr((const char*)at_response_buffer, "+CREG: 0,5");
    while(strx == NULL && extstrx == NULL)
    {
        if(IS_TIMEOUT(start_time, CREG_TIMEOUT_MS))
        {
            init_success = 0;
            _4g_is_connecting = 1;
            break;
        }

        Clear_Buffer();
        AT_SendCommand("AT+CREG?\r\n");
        vTaskDelay(500);
        strx = strstr((const char*)at_response_buffer, "+CREG: 0,1");
        extstrx = strstr((const char*)at_response_buffer, "+CREG: 0,5");
    }

    if(!init_success) return;
    Clear_Buffer();

    // 检查GPRS网络注册（带超时）
    AT_SendCommand("AT+CGREG?\r\n");
    vTaskDelay(500);

    start_time = xTaskGetTickCount();
    strx = strstr((const char*)at_response_buffer, "+CGREG: 0,1");
    extstrx = strstr((const char*)at_response_buffer, "+CGREG: 0,5");
    while(strx == NULL && extstrx == NULL)
    {
        if(IS_TIMEOUT(start_time, CGREG_TIMEOUT_MS))
        {
            init_success = 0;
            _4g_is_connecting = 1;
            break;
        }

        Clear_Buffer();
        AT_SendCommand("AT+CGREG?\r\n");
        vTaskDelay(500);
        strx = strstr((const char*)at_response_buffer, "+CGREG: 0,1");
        extstrx = strstr((const char*)at_response_buffer, "+CGREG: 0,5");
    }

    if(!init_success) return;
    Clear_Buffer();

    // 激活PDP上下文
    AT_SendCommand("AT+QIACT=1\r\n");
    vTaskDelay(500);

    // 查询IP地址
    AT_SendCommand("AT+QIACT?\r\n");
    vTaskDelay(500);

    // 打开TCP连接（带超时）
    AT_SendCommand("AT+QIOPEN=1,0,\"TCP\",\"36.139.155.149\",9004,0,1\r\n");
    vTaskDelay(500);

    start_time = xTaskGetTickCount();
    strx = strstr((const char*)at_response_buffer, "+QIOPEN: 0,0");
    while(strx == NULL)
    {
        if(IS_TIMEOUT(start_time, QIOPEN_TIMEOUT_MS))
        {
            init_success = 0;
            _4g_is_connecting = 1;
            break;
        }

        strx = strstr((const char*)at_response_buffer, "+QIOPEN: 0,0");
        vTaskDelay(100);
    }

    if(init_success)
    {
    	dis_connect=0;
        _4g_is_connecting = 0; // =0是连上了
    }
    else
    {
        _4g_is_connecting = 1; // =1是没有连上
    }

    Clear_Buffer();
}

void Disconnect_4G(void)
{
//	  HAL_UART_Transmit(&huart2,(uint8_t*)str5,strlen(str5),1000);
//	  vTaskDelay(1000);
	  HAL_UART_Transmit(&huart2,(uint8_t*)str6,strlen(str6),1000);
	  vTaskDelay(1000);
}

void Get_Gps_data()
{
    switch (gps_ask_state)
    {
        case 0:                    //打开GPS数据流
        	HAL_UART_Transmit(&huart2,(uint8_t*)str7,strlen(str7),1000);
            vTaskDelay(50);
            gps_ask_state++;
            break;

        case 1:
        	gps_lose_cnt++;
            HAL_UART_Transmit(&huart2,(uint8_t*)str8,strlen(str8),1000); //问讯数据
            vTaskDelay(50);
            if(gps_lose_cnt>5)                                            //5次没要到数据，就默认为断联
            {
            	gps_ask_state++;
            }

            break;

        case 2:
            HAL_UART_Transmit(&huart2,(uint8_t*)str9,strlen(str9),1000); //关闭GPS
            vTaskDelay(50);
            gps_ask_state++;
            gps_lose_cnt=0;
            break;
        case 3:                                      //重新连接TCP的代码
        	gps_ask_state=0;
        	ask_gps_state=0;
        	break;
    }



}

void open_4G(void)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	vTaskDelay(30);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	vTaskDelay(1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
}

void ask_sig(void)
{
	HAL_UART_Transmit(&huart2,(uint8_t*)str10,strlen(str10),1000); //关闭GPS
	vTaskDelay(50);
}

void Get_time_data()
{
	HAL_UART_Transmit(&huart2,(uint8_t*)str11,strlen(str11),1000); //问时间
	vTaskDelay(50);
}
