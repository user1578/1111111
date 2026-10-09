#include "main.h"
#include "msd.h"          // SD卡底层驱动
#include "ff.h"
#include "ffconf.h"

int IAP_ProcessUpgrade(uint16_t total_packets, uint16_t current_packet, uint8_t *data, uint16_t data_len);
void IAP_SendResponse(uint8_t status, uint16_t packet_num);
