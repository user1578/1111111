#include "main.h"
#include "printer_driver.h"
#include "data_send.h"
#include "rec_dealprint.h"
#include "data_rec.h"




TempRecord g_records[MAX_RECORDS];
uint16_t g_record_count = 0;
uint8_t g_total_packets = 0;
uint8_t g_received_packets = 0;
extern uint8_t print_state;

void sort_records(void)
{
    uint16_t i, j;
    TempRecord tmp;
    for (i = 0; i < g_record_count - 1; i++) {
        for (j = 0; j < g_record_count - 1 - i; j++) {
            TempRecord *a = &g_records[j];
            TempRecord *b = &g_records[j + 1];

            if (a->year > b->year) {
                tmp = *a; *a = *b; *b = tmp;
            } else if (a->year < b->year) {
                continue;
            } else {
                if (a->month > b->month) {
                    tmp = *a; *a = *b; *b = tmp;
                } else if (a->month < b->month) {
                    continue;
                } else {
                    if (a->day > b->day) {
                        tmp = *a; *a = *b; *b = tmp;
                    } else if (a->day < b->day) {
                        continue;
                    } else {
                        if (a->hour > b->hour) {
                            tmp = *a; *a = *b; *b = tmp;
                        } else if (a->hour < b->hour) {
                            continue;
                        } else {
                            if (a->minute > b->minute) {
                                tmp = *a; *a = *b; *b = tmp;
                            }
                        }
                    }
                }
            }
        }
    }
}



void handle_print_command(uint8_t *pkt)
{
    // pkt ָ�����ͬ��ͷ���������ݰ�

    uint8_t pkt_len = pkt[2];          // ���ݰ��ܳ���
    uint8_t cur_pkt = pkt[5];           // ��ǰ����
    uint8_t total_pkt = pkt[6];         // �ܰ���
    uint16_t year = (pkt[7] << 8) | pkt[8];
    uint8_t month = pkt[9];
    uint8_t day = pkt[10];

    // �������ݼ�¼�������ܳ��ȼ�ȥ�̶�ͷ������Ԫ��ַ�����ڹ�8�ֽڣ�������У���ֽ�
    uint16_t data_len = pkt_len - 10;  // 10 = 8(�̶�ͷ) + 2(У��)
    uint16_t record_num = data_len / 8;

    uint8_t *data_ptr = pkt + 11;  // ������ʼλ��

    for (uint16_t i = 0; i < record_num; i++) {
        if (g_record_count >= MAX_RECORDS) {
            break;  // �洢������������������
        }

        uint8_t hour = data_ptr[0];
        uint8_t minute = data_ptr[1];
        int16_t temp1 = (data_ptr[2] << 8) | data_ptr[3];
        uint8_t humi1 = data_ptr[4];
        int16_t temp2 = (data_ptr[5] << 8) | data_ptr[6];
        uint8_t humi2 = data_ptr[7];

        // �洢��¼
        g_records[g_record_count].year = year;
        g_records[g_record_count].month = month;
        g_records[g_record_count].day = day;
        g_records[g_record_count].hour = hour;
        g_records[g_record_count].minute = minute;
        g_records[g_record_count].temp1 = temp1;
        g_records[g_record_count].humi1 = humi1;
        g_records[g_record_count].temp2 = temp2;
        g_records[g_record_count].humi2 = humi2;
        g_record_count++;

        data_ptr += 8;  // �ƶ�����һ����¼
    }

    // ���½��ռ���
    if (cur_pkt == 1) {
        g_total_packets = total_pkt;
        g_received_packets = 1;
    } else {
        g_received_packets++;
    }

    // �ظ��յ���ǰ��
    print_OK(cur_pkt);

    // �ж��Ƿ����а����ѽ���
    if (g_received_packets == g_total_packets && g_total_packets > 0) {
        print_temp_records();  // ��ӡ����
        print_state=1;
        print_send();
        // ��ռ�¼��׼����һ��
        g_record_count = 0;
        g_total_packets = 0;
        g_received_packets = 0;
    }
}

