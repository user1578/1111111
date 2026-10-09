#include "main.h"

// AT命令响应处理缓冲区
#define AT_RESPONSE_BUF_SIZE 800
// ============== 状态定义 ==============
typedef enum {
    _4G_IDLE = 0,
    _4G_INIT_CHECK_READY,      // 检查模块就绪
    _4G_INIT_SET_ECHO,         // 设置回显
    _4G_INIT_SET_ERROR_REPORT, // 设置错误报告
    _4G_CHECK_SIM_CARD,        // 检查SIM卡
    _4G_CHECK_NETWORK_REG,     // 检查网络注册
    _4G_SET_URC_PORT,          // 设置URC端口（你原来有的）
    _4G_CONFIG_APN,            // 配置APN
    _4G_ACTIVATE_CONTEXT,      // 激活上下文
    _4G_CHECK_ACTIVATION,      // 检查激活状态（查询IP）
    _4G_OPEN_TCP,              // 打开TCP连接
    _4G_CONNECTED,             // 连接完成
    _4G_ERROR                  // 错误状态
} _4G_ConnectState_t;

void Connect_4G(void);
void Disconnect_4G(void);
void Get_Gps_data();

void Start_4G_Connection(void);
void _4G_Connect_StateMachine(void);
uint8_t check_at_response(const char* expected);
void AT_SendCommand(const char* cmd);
void EC800_Init(void);
void Clear_Buffer(void);
void open_4G(void);
void ask_sig(void);
void Get_time_data();
