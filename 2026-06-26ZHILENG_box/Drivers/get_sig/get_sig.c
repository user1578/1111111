#include "main.h"
#include "stdio.h"
#include  "string.h"
#include "stdlib.h"


extern uint8_t signal_ready;     // 数据就绪标志
extern int16_t signal_rsrp ;      // RSRP 值（dBm）
extern int16_t signal_sinr;      // SINR 值（dB）
extern uint16_t signal_mcc;      // MCC
extern uint8_t  signal_mnc;      // MNC
extern uint32_t signal_tac;      // TAC（十六进制）
extern uint32_t signal_cellid;   // Cell ID（十六进制）
extern char signal_rx_buffer[300]; // 根据实际情况调整大小
/**
 * @brief 解析 +QENG:"servingcell" 响应
 *        解析字段：MCC、MNC、TAC、Cell ID、RSRP、SINR
 *        协议样例：
 *        +QENG: "servingcell","NOCONN","LTE","TDD",460,00,6C4E907,198,38950,40,5,5,7208,-88,-6,-80,29,38
 *        位置（servingcell 后）：
 *          1=NOCONN 2=LTE 3=TDD 4=MCC 5=MNC 6=TAC(hex)
 *          7=PCID 8=EARFCN 9=band 10=ul_bw 11=dl_bw 12=CellID(hex)
 *          13=RSRP 14=RSRQ 15=RSSI 16=SINR 17=srxlev
 * @note  RSRP/SINR 解析逻辑保持不变
 */
void process_signal(void)
{
    char *p = signal_rx_buffer;
    char *end;
    int16_t rsrp = 0, sinr = 0;
    uint16_t mcc = 0;
    uint8_t  mnc = 0;
    uint32_t tac = 0;
    uint32_t cellid = 0;
    char temp[16];

    // 查找 "+QENG:"
    p = strstr(p, "+QENG:");
    if (!p) goto exit;
    // 查找 "servingcell"
    p = strstr(p, "servingcell");
    if (!p) goto exit;
    // 跳到第一个逗号后（即 "NOCONN" 字段）
    p = strchr(p, ',');
    if (!p) goto exit;
    p++;

    // 跳过 3 个字段（NOCONN, LTE, TDD）到达 MCC
    for (int i = 0; i < 3; i++) {
        p = strchr(p, ',');
        if (!p) goto exit;
        p++;
    }

    // 提取 MCC（十进制）
    end = strchr(p, ',');
    if (!end) goto exit;
    if ((end - p) < (int)sizeof(temp)) {
        strncpy(temp, p, end - p);
        temp[end - p] = '\0';
        mcc = (uint16_t)atoi(temp);
    }
    p = end + 1;

    // 提取 MNC（十进制）
    end = strchr(p, ',');
    if (!end) goto exit;
    if ((end - p) < (int)sizeof(temp)) {
        strncpy(temp, p, end - p);
        temp[end - p] = '\0';
        mnc = (uint8_t)atoi(temp);
    }
    p = end + 1;

    // 提取 TAC（十六进制）
    end = strchr(p, ',');
    if (!end) goto exit;
    if ((end - p) < (int)sizeof(temp)) {
        strncpy(temp, p, end - p);
        temp[end - p] = '\0';
        tac = strtoul(temp, NULL, 16);
    }
    p = end + 1;

    // 跳过 5 个字段（PCID, EARFCN, 频段, UL带宽, DL带宽）到达 Cell ID
    for (int i = 0; i < 5; i++) {
        p = strchr(p, ',');
        if (!p) goto exit;
        p++;
    }

    // 提取 Cell ID（十六进制）
    end = strchr(p, ',');
    if (!end) goto exit;
    if ((end - p) < (int)sizeof(temp)) {
        strncpy(temp, p, end - p);
        temp[end - p] = '\0';
        cellid = strtoul(temp, NULL, 16);
    }
    p = end + 1;

    // 提取 RSRP（十进制）
    end = strchr(p, ',');
    if (!end) goto exit;
    if ((end - p) < (int)sizeof(temp)) {
        strncpy(temp, p, end - p);
        temp[end - p] = '\0';
        rsrp = atoi(temp);
    }

    // 跳过两个字段（RSRQ, RSSI）到达 SINR
    p = end + 1;
    for (int i = 0; i < 2; i++) {
        p = strchr(p, ',');
        if (!p) goto exit;
        p++;
    }

    // 提取 SINR
    end = strchr(p, ',');
    if (!end) end = p + strlen(p);  // 可能最后一个字段
    if ((end - p) < (int)sizeof(temp)) {
        strncpy(temp, p, end - p);
        temp[end - p] = '\0';
        sinr = atoi(temp);
    }

    // 存储结果
    signal_mcc = mcc;
    signal_mnc = mnc;
    signal_tac = tac;
    signal_cellid = cellid;
    signal_rsrp = rsrp;
    signal_sinr = sinr;

exit:
    // 无论解析成功与否，都清标志（防止重复解析）
    signal_ready = 0;
}


//void process_signal(void)
//{
//    if(signal_ready)
//    {
//    	process_signal();
//        // 使用signal_rsrp和signal_sinr做进一步处理，比如上传等
//        signal_ready = 0; // 清除标志
//    }
//}
