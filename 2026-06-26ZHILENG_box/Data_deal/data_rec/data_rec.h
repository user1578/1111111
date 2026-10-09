#include "main.h"
#include "Auxiliary.h"

#define HEADER_BYTE1 0xA5
#define HEADER_BYTE2 0x5A
#define HEADER_BYTE3 0xC3
#define HEADER_BYTE4 0x3C
#define HEADER_BYTE5 0xF0
#define HEADER_BYTE6 0x0F
#define HEADER_LEN   6

#define PKT_LEN_POS   6
#define PKT_ADDR_POS  7
#define PKT_ADDR_LEN  4
#define PKT_CMD_POS   11
#define PKT_DATA_POS  12
#define PKT_OVERHEAD  7

static inline uint8_t check_pkt_header(uint8_t *data)
{
    return (data[0] == HEADER_BYTE1 && data[1] == HEADER_BYTE2 &&
            data[2] == HEADER_BYTE3 && data[3] == HEADER_BYTE4 &&
            data[4] == HEADER_BYTE5 && data[5] == HEADER_BYTE6);
}

static inline uint8_t check_pkt_address(uint8_t *data)
{
    return (data[PKT_ADDR_POS]   == (uint8_t)(MY_ADDRESS >> 24) &&
            data[PKT_ADDR_POS+1] == (uint8_t)(MY_ADDRESS >> 16) &&
            data[PKT_ADDR_POS+2] == (uint8_t)(MY_ADDRESS >> 8)  &&
            data[PKT_ADDR_POS+3] == (uint8_t)(MY_ADDRESS));
}

static inline void write_pkt_header(uint8_t *buff, uint16_t *i)
{
    buff[(*i)++] = HEADER_BYTE1;
    buff[(*i)++] = HEADER_BYTE2;
    buff[(*i)++] = HEADER_BYTE3;
    buff[(*i)++] = HEADER_BYTE4;
    buff[(*i)++] = HEADER_BYTE5;
    buff[(*i)++] = HEADER_BYTE6;
    buff[(*i)++] = 0x00;
    buff[(*i)++] = (uint8_t)(MY_ADDRESS >> 24);
    buff[(*i)++] = (uint8_t)(MY_ADDRESS >> 16);
    buff[(*i)++] = (uint8_t)(MY_ADDRESS >> 8);
    buff[(*i)++] = (uint8_t)(MY_ADDRESS);
}

static inline void write_pkt_checksum(uint8_t *buff, uint16_t length_index, uint16_t *i)
{
    buff[PKT_LEN_POS] = length_index - 5;
    buff[(*i)++] = XOR(buff, length_index - 7, 7);
    buff[(*i)++] = CheckSum8(buff, length_index - 7, 7);
}

void data_deal(void);
int parse_csv(char* str, char* tokens[], int max_tokens, char delimiter);
