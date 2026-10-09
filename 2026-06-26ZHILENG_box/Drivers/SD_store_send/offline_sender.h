// offline_sender.h
#ifndef OFFLINE_SENDER_H
#define OFFLINE_SENDER_H

#include <stdint.h>
#include <stdbool.h>

// 函数声明
void Offline_Sender_Process(void);
bool Offline_Sender_IsActive(void);
uint32_t Offline_Sender_GetSentCount(void);
uint32_t Offline_Sender_GetRemainingCount(void);
void Offline_Sender_Stop(void);

#endif
