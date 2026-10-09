#ifndef		__AUXILIARY_H
#define		__AUXILIARY_H

#include"main.h"


typedef enum
{
	NO_CHECK,
	CRC_CHECK,
	XOR_CHECK,
	SUM_CHECK,
}CHECK_Type;
//*********************** API **************************************
u16 CRC16(u8 * p, u8 length);
u8 XOR(u8 * p,u16 length,u8 start_pos) ;
u8 CheckSum8(u8 * p, u16 length,u8 start_pos);
u16 CheckSum16(u8 * p, u16 length,u8 start_pos);
u32 CheckSum32(u8 * p, u16 length,u8 start_pos);

#endif
