#include "Auxiliary.h"


/*******************************************************************************
  * @brief  CRC16 冗余循环校验计算
  * @param  传入待计算的数据帧指针及数据长
  * @retval 计算的冗余循环校验--u16,注意:CRC16初值为0xFFFF
********************************************************************************/
u16 CRC16(u8 *p, u8 length)
{
	u8 CRC16Low,CRC16High,CL,CH,SaveHi,SaveLo,i,Flag;

	CRC16Low = 0xFF;
	CRC16High = 0xFF;
	CL = 0x01;//多项式
	CH = 0xA0;//多项式

	for(i = 0;i < length;i++)
	{
		CRC16Low ^= *(p + i);//每一个数据与CRC寄存器进行异或

		for(Flag = 0;Flag < 8;Flag++)
		{
			SaveHi = CRC16High;
			SaveLo = CRC16Low;
			CRC16High >>= 1 ;//高位右移一位
			CRC16Low >>= 1 ; //低位右移一位

			if ((SaveHi & 0x01) == 0x01)  //如果高位字节最后一位为1
			{
				CRC16Low |=0x80;     //则低位字节右移后前面补1否则自动补0
			}

			if ((SaveLo & 0x01) == 0x01) //如果LSB为1，则与多项式码进行异或
			{
				CRC16High ^= CH;
				CRC16Low ^= CL;
			}
		}
	}
	return (CRC16High<<8) | CRC16Low;
}
/*******************************************************************************
  * @brief  XOR 异或校验计算
  * @param  传入待计算的数据帧指针,数据长,起始计算位置字节[从0计数]
  * @retval 
********************************************************************************/
u8 XOR(u8 *p,u16 length,u8 start_pos) 
{ 
	u16 i = 0;
	u8 xor_temp = 0;
	    
	for (i = start_pos;  i < length + start_pos; i++)//注意是从第x个字节开始计算
	{
		xor_temp = xor_temp ^ *(p + i); //进行异或校验取值
	}
	
	return xor_temp;
}
/***********************************************************************************
 *函数名称:CheckSum
 *函数功能:校验和效验函数
 *入口参数:*p-效验帧的指针   length:帧长 start_pos:起始位置字节
 *返回值	:8 bit
 *备注	  :
 **********************************************************************************/
u8 CheckSum8(u8 * p, u16 length,u8 start_pos)
{
	u16 i = 0;
	u8 temp = 0;

	for(i = start_pos;i < length + start_pos;i++)//注意是从第x个字节开始计算
	{
		temp += *(p + i);
	}
	return temp;
}
/***********************************************************************************
 *函数名称	:CheckSum
 *函数功能	:校验和效验函数
 *入口参数	:*p-效验帧的指针   length:帧长 start_pos:起始位置字节
 *返回值	:16 bit
 *备注	  	:
 **********************************************************************************/
u16 CheckSum16(u8 * p, u16 length,u8 start_pos)
{
	u16 i = 0;
	u16 temp = 0;

	for(i = start_pos;i < length + start_pos;i++)//注意是从第x个字节开始计算
	{
		temp += (u16)*(p + i);
	}
	return temp;
}
/***********************************************************************************
 *函数名称:CheckSum
 *函数功能:校验和效验函数
 *入口参数:*p-效验帧的指针   length:帧长 start_pos:起始位置字节
 *返回值	:32bit
 *备注	  :
 **********************************************************************************/
u32 CheckSum32(u8 * p, u16 length,u8 start_pos)
{
	u16 i = 0;
	u32 temp = 0;

	for(i = start_pos;i < length + start_pos;i++)//注意是从第x个字节开始计算
	{
		temp += (u32)*(p + i);
	}
	return temp;
}

