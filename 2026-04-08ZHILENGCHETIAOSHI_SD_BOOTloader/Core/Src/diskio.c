/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs             (C)ChaN, 2010*/
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "msd.h"
#include "diskio.h"
#include  <stdio.h>


//��ʹ���ļ�ϵͳ��ʱ���¼����ʱ��Ҫ��������Ӻ���������
#define  GET_FATTIME_EN  1	


//#if	 GET_FATTIME_EN
//#include "calendar.h"  //�����Ӻ���������
//#endif
//	 
 							

vu8 SD_Card_Ready;//SD����ʼ���ɹ���־

extern u8  SD_Type;//SD��������

//�⼸��������msd.c��,��¼��sd����һЩ����											
extern u32 Mass_Block_Count;//������
extern u32 Mass_Block_Size;//������С
extern u32 Mass_Memory_Size;//����

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
    u8 state;

    if(drv)
    {
        return STA_NOINIT;  //��֧�ִ���0�Ĳ���
    }


    state = MSD_Init();
	SD_Card_Ready=state;//SD����ʼ���ɹ���־

	
	if(SD_Card_Ready)//SD��û��ʼ���ɹ�
	{
	   return STA_NODISK;
	}

    if(state == STA_NODISK)
    {
        return STA_NODISK;
    }
    else if(state != 0)
    {
        return STA_NOINIT;  //�������󣺳�ʼ��ʧ��
    }
    else
    {
        return 0;           //��ʼ���ɹ�
    }
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
    if(drv)
    {
        return STA_NOINIT;  //��֧�ִ���0����
    }

	if(SD_Card_Ready)//SD��û��ʼ���ɹ�
	{
	   return STA_NODISK;
	}

    //���SD���Ƿ����
  //  if(!SD_DET())
  //  {
  //     return STA_NODISK;
  //  }
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	u8 res=0;

    if (drv || !count)
    {   
        return RES_PARERR;  //��֧�ֵ����̲�����count���ܵ���0�����򷵻ز�������
    }

	if(SD_Card_Ready)//SD��û��ʼ���ɹ�
	{
	   return RES_NOTRDY;
	}

   // if(!SD_DET())
   // {
   //     return RES_NOTRDY;  //û�м�⵽SD������NOT READY����
   // }

    if(count==1)            //1��sector�Ķ�����      
    { 
	

    	//�������SDHC����������sector��ַ������ת����byte��ַ
    	if(SD_Type!=SD_TYPE_V2HC)
    	{
       		res = MSD_ReadBlock(buff, (u32)sector*512, 512);  
    	}else{
	
	   		res = MSD_ReadBlock(buff, (u32)sector, 512);  
		}
  
    }                                                
    else                    //多sector的读操作     
    
	{
  	   
		if(SD_Type!=SD_TYPE_V2HC)
		{
			res = MSD_ReadBuffer(buff, (u32)sector*512, (u32)count*512);
		}else{
			res = MSD_ReadBuffer(buff, (u32)sector, (u32)count*512);
		}

    }                                                
	/*
    do                           
    {                                          
        if(SD_ReadSingleBlock(sector, buff)!=0)
        {                                      
            res = 1;                           
            break;                             
        }                                      
        buff+=512;                             
    }while(--count);                                         
    */

    //��������ֵ����SPI_SD_driver.c�ķ���ֵת��ff.c�ķ���ֵ
    if(res == 0x00)
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR;
    }
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	        /* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	u8 res;

    if (drv || !count)
    {    
        return RES_PARERR;  //��֧�ֵ����̲�����count���ܵ���0�����򷵻ز�������
    }

	if(SD_Card_Ready)//SD��û��ʼ���ɹ�
	{
	   return RES_NOTRDY;
	}

	/*
    if(!SD_DET())
    {
        return RES_NOTRDY;  //û�м�⵽SD������NOT READY����
    }  */

    // ��д����
    if(count == 1)
    {

    	//�������SDHC����������sector��ַ������ת����byte��ַ
    	if(SD_Type!=SD_TYPE_V2HC)
    	{	      	   
			res = MSD_WriteBlock((u8*)buff, (u32)sector*512, 512);
		}else{
	    	res = MSD_WriteBlock((u8*)buff, (u32)sector, 512);
		}

    }
    else
    {
 
    	//�������SDHC����������sector��ַ������ת����byte��ַ
    	if(SD_Type!=SD_TYPE_V2HC)
    	{	   
			res = MSD_WriteBuffer((u8*)buff, (u32)sector*512, (u32)count*512);
		}else{
			res = MSD_WriteBuffer((u8*)buff, (u32)sector, (u32)count*512);
		}

 
    }

    // ����ֵת��
    if(res == 0)
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR;
    }
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    DRESULT res;

	if(SD_Card_Ready)//SD��û��ʼ���ɹ�
	{
	   return RES_NOTRDY;
	}

	//Get_Medium_Characteristics();//��ȡ������Ϣ

    if (drv)
    {    
        return RES_PARERR;  //��֧�ֵ����̲��������򷵻ز�������
    }
    
    //FATFSĿǰ�汾���账��CTRL_SYNC��GET_SECTOR_COUNT��GET_BLOCK_SIZ��������
    switch(ctrl)
    {
    case CTRL_SYNC:

        SD_CS_ENABLE();
        if(MSD_GetResponse(0xff)==0)
        {
            res = RES_OK;
        }
        else
        {
            res = RES_ERROR;
        }
        SD_CS_DISABLE();
        break;
        
    case GET_BLOCK_SIZE:
	    //Get_Medium_Characteristics();//��ȡ������Ϣ
        *(WORD*)buff = Mass_Block_Size;
		//*(WORD*)buff = 512;
        res = RES_OK;
        break;

    case GET_SECTOR_COUNT:
		//Get_Medium_Characteristics();//��ȡ������Ϣ
        *(DWORD*)buff = Mass_Block_Count;
        res = RES_OK;
        break;
    default:
        res = RES_PARERR;
        break;
    }

    return res;
}

/*-----------------------------------------------------------------------*/
/* User defined function to give a current time to fatfs module          */
/* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */                                                                                                                                                                                                                                          
/* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */                                                                                                                                                                                                                                                
DWORD get_fattime (void)
{
#if	 GET_FATTIME_EN
    DWORD temp;
  //  rtc_get();
	//temp=(timer.w_year-1980)<<25|timer.w_month<<21|timer.w_date<<16|timer.hour<<11|timer.min<<5|timer.sec;
    return temp;
#else
     return 0;
#endif
}

