/******************** (C) COPYRIGHT 2007 STMicroelectronics ********************
* File Name          : msd.c
* Author             : MCD Application Team
* Version            : V1.0
* Date               : 10/08/2007
* Description        : MSD card driver source file.
*                      Pin assignment:
*             ----------------------------------------------
*             |  STM32F10x    |     MSD          Pin        |
*             ----------------------------------------------
*             | PA.15         |   ChipSelect      1         |
*             | PB.? / MOSI   |   DataIn          2         |
*             |               |   GND             3 (0 V)   |
*             |               |   VDD             4 (3.3 V) |
*             | PB.? / SCLK   |   Clock           5         |
*             |               |   GND             6 (0 V)   |
*             | PB.? / MISO   |   DataOut         7         |
*             -----------------------------------------------
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
//�޸��� ��xingxing
//ʱ��	 ��2011/02/27
//����   ���Ѿ�֧��SDHC��


/* Includes ------------------------------------------------------------------*/
#include "msd.h"
#include "spi.h"
//#include "stm32f10x.h"

extern vu8 SD_Card_Ready;//SD����ʼ���ɹ���־����diskio.c��
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/


/* Global Variable ------------------------------------------------------------*/  
u8  SD_Type=0;//SD��������,Ϊ��֧�ָ��ٿ�������

u32 Mass_Block_Count;
u32 Mass_Block_Size;
u32 Mass_Memory_Size;
sMSD_CSD MSD_csd;

void SD_SPI_Configuration(void)
{
   MX_SPI1_Init();
  
}
extern SPI_HandleTypeDef hspi1; 
//������ֲʱ,��Ҫ�޸�
void SD_SPI_HightSpeed(void)  //���SD���Ķ�д�ٶ�
{
	  SPI_InitTypeDef SPI_InitStructure ;
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
//  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
//  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}  

//������ֲʱ,��Ҫ�޸�
void MSD_WriteByte(u8 Data)
{
	HAL_SPI_Transmit(&hspi1, &Data, 1,100);	
}


//������ֲʱ,��Ҫ�޸�
u8 MSD_ReadByte(void)
{
	u8 Data,TxData=0xff;
 HAL_SPI_TransmitReceive(&hspi1, &TxData,&Data, 1,100);	
	return Data;
}

//������ֲʱ,��Ҫ�޸�
u8 SPI2_ReadWriteByte(u8 TxData)//�ڸ���SD���������������õ�
{
	u8 RxData;	

  HAL_SPI_TransmitReceive(&hspi1, &TxData,&RxData, 1,100);	
	return RxData;
}



/*******************************************************************************
* Function Name  : MSD_Init
* Description    : Initializes the MSD/SD communication.
* Input          : None
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_Init(void)
{
  u32 i = 0;
  u8 ret;

#if SDHC_EN
  /*********2011/02/27 by xingxing*********/
  //Ϊ��֧�ָ���sd���������
  ret = SDHC_Init();
  SD_Card_Ready = ret;//ret=1 �� SD��δ׼����
  return ret;
#endif

  /* Initialize SPI */
  SD_SPI_Configuration();
  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte 0xFF, 10 times with CS high*/
  /* rise CS and MOSI for 80 clocks cycles */
  for (i = 0; i <= 9; i++)
  {
    /* Send dummy byte 0xFF */
    MSD_WriteByte(DUMMY);
  }
  /*------------Put MSD in SPI mode--------------*/
  /* MSD initialized and set to SPI mode properly */
  ret = MSD_GoIdleState();
  SD_Card_Ready = ret;
  return ret;
  //return (MSD_GoIdleState());	
}

/*******************************************************************************
* Function Name  : MSD_WriteBlock
* Description    : Writes a block on the MSD
* Input          : - pBuffer : pointer to the buffer containing the data to be
*                    written on the MSD.
*                  - WriteAddr : address to write on.
*                  - NumByteToWrite: number of data to write
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_WriteBlock(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
  u32 i = 0;
  u8 rvalue = MSD_RESPONSE_FAILURE;

  /* MSD chip select low */
  MSD_CS_LOW();
  /* Send CMD24 (MSD_WRITE_BLOCK) to write multiple block */
  MSD_SendCmd(MSD_WRITE_BLOCK, WriteAddr, 0xFF);

  /* Check if the MSD acknowledged the write block command: R1 response (0x00: no errors) */
  if (!MSD_GetResponse(MSD_RESPONSE_NO_ERROR))
  {
    /* Send a dummy byte */
    MSD_WriteByte(DUMMY);
    /* Send the data token to signify the start of the data */
    MSD_WriteByte(0xFE);
    /* Write the block data to MSD : write count data by block */
    for (i = 0; i < NumByteToWrite; i++)
    {
      /* Send the pointed byte */
      MSD_WriteByte(*pBuffer);
      /* Point to the next location where the byte read will be saved */
      pBuffer++;
    }
    /* Put CRC bytes (not really needed by us, but required by MSD) */
    MSD_ReadByte();
    MSD_ReadByte();
    /* Read data response */
    if (MSD_GetDataResponse() == MSD_DATA_OK)
    {
      rvalue = MSD_RESPONSE_NO_ERROR;
    }
  }

  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte: 8 Clock pulses of delay */
  MSD_WriteByte(DUMMY);
  /* Returns the reponse */
  return rvalue;
}

/*******************************************************************************
* Function Name  : MSD_ReadBlock
* Description    : Reads a block of data from the MSD.
* Input          : - pBuffer : pointer to the buffer that receives the data read
*                    from the MSD.
*                  - ReadAddr : MSD's internal address to read from.
*                  - NumByteToRead : number of bytes to read from the MSD.
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_ReadBlock(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
  u32 i = 0;
  u8 rvalue = MSD_RESPONSE_FAILURE;

  /* MSD chip select low */
  MSD_CS_LOW();
  /* Send CMD17 (MSD_READ_SINGLE_BLOCK) to read one block */
  MSD_SendCmd(MSD_READ_SINGLE_BLOCK, ReadAddr, 0xFF);

  /* Check if the MSD acknowledged the read block command: R1 response (0x00: no errors) */
  if (!MSD_GetResponse(MSD_RESPONSE_NO_ERROR))
  {
    /* Now look for the data token to signify the start of the data */
    if (!MSD_GetResponse(MSD_START_DATA_SINGLE_BLOCK_READ))
    {
      /* Read the MSD block data : read NumByteToRead data */
      for (i = 0; i < NumByteToRead; i++)
      {
        /* Save the received data */
        *pBuffer = MSD_ReadByte();
        /* Point to the next location where the byte read will be saved */
        pBuffer++;
      }
      /* Get CRC bytes (not really needed by us, but required by MSD) */
      MSD_ReadByte();
      MSD_ReadByte();
      /* Set response value to success */
      rvalue = MSD_RESPONSE_NO_ERROR;
    }
  }

  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte: 8 Clock pulses of delay */
  MSD_WriteByte(DUMMY);
  /* Returns the reponse */
  return rvalue;
}

/*******************************************************************************
* Function Name  : MSD_WriteBuffer
* Description    : Writes many blocks on the MSD
* Input          : - pBuffer : pointer to the buffer containing the data to be
*                    written on the MSD.
*                  - WriteAddr : address to write on.
*                  - NumByteToWrite: number of data to write
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_WriteBuffer(u8* pBuffer, u32 WriteAddr, u32 NumByteToWrite)
{
  u32 i = 0, NbrOfBlock = 0, Offset = 0;
  u8 rvalue = MSD_RESPONSE_FAILURE;

  /* Calculate number of blocks to write */
  NbrOfBlock = NumByteToWrite / BLOCK_SIZE;
  /* MSD chip select low */
  MSD_CS_LOW();

  /* Data transfer */
  while (NbrOfBlock --)
  {
    /* Send CMD24 (MSD_WRITE_BLOCK) to write blocks */
    MSD_SendCmd(MSD_WRITE_BLOCK, WriteAddr + Offset, 0xFF);

    /* Check if the MSD acknowledged the write block command: R1 response (0x00: no errors) */
    if (MSD_GetResponse(MSD_RESPONSE_NO_ERROR))
    {
      return MSD_RESPONSE_FAILURE;
    }
    /* Send dummy byte */
    MSD_WriteByte(DUMMY);
    /* Send the data token to signify the start of the data */
    MSD_WriteByte(MSD_START_DATA_SINGLE_BLOCK_WRITE);
    /* Write the block data to MSD : write count data by block */
    for (i = 0; i < BLOCK_SIZE; i++)
    {
      /* Send the pointed byte */
      MSD_WriteByte(*pBuffer);
      /* Point to the next location where the byte read will be saved */
      pBuffer++;
    }
    /* Set next write address */
    Offset += 512;
    /* Put CRC bytes (not really needed by us, but required by MSD) */
    MSD_ReadByte();
    MSD_ReadByte();
    /* Read data response */
    if (MSD_GetDataResponse() == MSD_DATA_OK)
    {
      /* Set response value to success */
      rvalue = MSD_RESPONSE_NO_ERROR;
    }
    else
    {
      /* Set response value to failure */
      rvalue = MSD_RESPONSE_FAILURE;
    }
  }

  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte: 8 Clock pulses of delay */
  MSD_WriteByte(DUMMY);
  /* Returns the reponse */
  return rvalue;
}

/*******************************************************************************
* Function Name  : MSD_ReadBuffer
* Description    : Reads multiple block of data from the MSD.
* Input          : - pBuffer : pointer to the buffer that receives the data read
*                    from the MSD.
*                  - ReadAddr : MSD's internal address to read from.
*                  - NumByteToRead : number of bytes to read from the MSD.
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_ReadBuffer(u8* pBuffer, u32 ReadAddr, u32 NumByteToRead)
{
  u32 i = 0, NbrOfBlock = 0, Offset = 0;
  u8 rvalue = MSD_RESPONSE_FAILURE;

  /* Calculate number of blocks to read */
  NbrOfBlock = NumByteToRead / BLOCK_SIZE;
  /* MSD chip select low */
  MSD_CS_LOW();

  /* Data transfer */
  while (NbrOfBlock --)
  {
    /* Send CMD17 (MSD_READ_SINGLE_BLOCK) to read one block */
    MSD_SendCmd (MSD_READ_SINGLE_BLOCK, ReadAddr + Offset, 0xFF);
    /* Check if the MSD acknowledged the read block command: R1 response (0x00: no errors) */
    if (MSD_GetResponse(MSD_RESPONSE_NO_ERROR))
    {
      return  MSD_RESPONSE_FAILURE;
    }
    /* Now look for the data token to signify the start of the data */
    if (!MSD_GetResponse(MSD_START_DATA_SINGLE_BLOCK_READ))
    {
      /* Read the MSD block data : read NumByteToRead data */
      for (i = 0; i < BLOCK_SIZE; i++)
      {
        /* Read the pointed data */
        *pBuffer = MSD_ReadByte();
        /* Point to the next location where the byte read will be saved */
        pBuffer++;
      }
      /* Set next read address*/
      Offset += 512;
      /* get CRC bytes (not really needed by us, but required by MSD) */
      MSD_ReadByte();
      MSD_ReadByte();
      /* Set response value to success */
      rvalue = MSD_RESPONSE_NO_ERROR;
    }
    else
    {
      /* Set response value to failure */
      rvalue = MSD_RESPONSE_FAILURE;
    }
  }

  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte: 8 Clock pulses of delay */
  MSD_WriteByte(DUMMY);
  /* Returns the reponse */
  return rvalue;
}

/*******************************************************************************
* Function Name  : MSD_GetCSDRegister
* Description    : Read the CSD card register.
*                  Reading the contents of the CSD register in SPI mode
*                  is a simple read-block transaction.
* Input          : - MSD_csd: pointer on an SCD register structure
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_GetCSDRegister(sMSD_CSD* MSD_csd)
{
  u32 i = 0;
  u8 rvalue = MSD_RESPONSE_FAILURE;
  u8 CSD_Tab[16];

  /* MSD chip select low */
  MSD_CS_LOW();
  /* Send CMD9 (CSD register) or CMD10(CSD register) */
  MSD_SendCmd(MSD_SEND_CSD, 0, 0xFF);

  /* Wait for response in the R1 format (0x00 is no errors) */
  if (!MSD_GetResponse(MSD_RESPONSE_NO_ERROR))
  {
    if (!MSD_GetResponse(MSD_START_DATA_SINGLE_BLOCK_READ))
    {
      for (i = 0; i < 16; i++)
      {
        /* Store CSD register value on CSD_Tab */
        CSD_Tab[i] = MSD_ReadByte();
      }
    }
    /* Get CRC bytes (not really needed by us, but required by MSD) */
    MSD_WriteByte(DUMMY);
    MSD_WriteByte(DUMMY);
    /* Set response value to success */
    rvalue = MSD_RESPONSE_NO_ERROR;
  }

  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte: 8 Clock pulses of delay */
  MSD_WriteByte(DUMMY);

  /* Byte 0 */
  MSD_csd->CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
  MSD_csd->SysSpecVersion = (CSD_Tab[0] & 0x3C) >> 2;
  MSD_csd->Reserved1 = CSD_Tab[0] & 0x03;
  /* Byte 1 */
  MSD_csd->TAAC = CSD_Tab[1] ;
  /* Byte 2 */
  MSD_csd->NSAC = CSD_Tab[2];
  /* Byte 3 */
  MSD_csd->MaxBusClkFrec = CSD_Tab[3];
  /* Byte 4 */
  MSD_csd->CardComdClasses = CSD_Tab[4] << 4;
  /* Byte 5 */
  MSD_csd->CardComdClasses |= (CSD_Tab[5] & 0xF0) >> 4;
  MSD_csd->RdBlockLen = CSD_Tab[5] & 0x0F;
  /* Byte 6 */
  MSD_csd->PartBlockRead = (CSD_Tab[6] & 0x80) >> 7;
  MSD_csd->WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
  MSD_csd->RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
  MSD_csd->DSRImpl = (CSD_Tab[6] & 0x10) >> 4;
  MSD_csd->Reserved2 = 0; /* Reserved */
  MSD_csd->DeviceSize = (CSD_Tab[6] & 0x03) << 10;
  /* Byte 7 */
  MSD_csd->DeviceSize |= (CSD_Tab[7]) << 2;
  /* Byte 8 */
  MSD_csd->DeviceSize |= (CSD_Tab[8] & 0xC0) >> 6;
  MSD_csd->MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
  MSD_csd->MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);
  /* Byte 9 */
  MSD_csd->MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
  MSD_csd->MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
  MSD_csd->DeviceSizeMul = (CSD_Tab[9] & 0x03) << 1;
  /* Byte 10 */
  MSD_csd->DeviceSizeMul |= (CSD_Tab[10] & 0x80) >> 7;
  MSD_csd->EraseGrSize = (CSD_Tab[10] & 0x7C) >> 2;
  MSD_csd->EraseGrMul = (CSD_Tab[10] & 0x03) << 3;
  /* Byte 11 */
  MSD_csd->EraseGrMul |= (CSD_Tab[11] & 0xE0) >> 5;
  MSD_csd->WrProtectGrSize = (CSD_Tab[11] & 0x1F);
  /* Byte 12 */
  MSD_csd->WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
  MSD_csd->ManDeflECC = (CSD_Tab[12] & 0x60) >> 5;
  MSD_csd->WrSpeedFact = (CSD_Tab[12] & 0x1C) >> 2;
  MSD_csd->MaxWrBlockLen = (CSD_Tab[12] & 0x03) << 2;
  /* Byte 13 */
  MSD_csd->MaxWrBlockLen |= (CSD_Tab[13] & 0xc0) >> 6;
  MSD_csd->WriteBlockPaPartial = (CSD_Tab[13] & 0x20) >> 5;
  MSD_csd->Reserved3 = 0;
  MSD_csd->ContentProtectAppli = (CSD_Tab[13] & 0x01);
  /* Byte 14 */
  MSD_csd->FileFormatGrouop = (CSD_Tab[14] & 0x80) >> 7;
  MSD_csd->CopyFlag = (CSD_Tab[14] & 0x40) >> 6;
  MSD_csd->PermWrProtect = (CSD_Tab[14] & 0x20) >> 5;
  MSD_csd->TempWrProtect = (CSD_Tab[14] & 0x10) >> 4;
  MSD_csd->FileFormat = (CSD_Tab[14] & 0x0C) >> 2;
  MSD_csd->ECC = (CSD_Tab[14] & 0x03);
  /* Byte 15 */
  MSD_csd->CRC_Check = (CSD_Tab[15] & 0xFE) >> 1;
  MSD_csd->Reserved4 = 1;

  /* Return the reponse */
  return rvalue;
}

/*******************************************************************************
* Function Name  : MSD_GetCIDRegister
* Description    : Read the CID card register.
*                  Reading the contents of the CID register in SPI mode
*                  is a simple read-block transaction.
* Input          : - MSD_cid: pointer on an CID register structure
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_GetCIDRegister(sMSD_CID* MSD_cid)
{
  u32 i = 0;
  u8 rvalue = MSD_RESPONSE_FAILURE;
  u8 CID_Tab[16];

  /* MSD chip select low */
  MSD_CS_LOW();
  /* Send CMD10 (CID register) */
  MSD_SendCmd(MSD_SEND_CID, 0, 0xFF);

  /* Wait for response in the R1 format (0x00 is no errors) */
  if (!MSD_GetResponse(MSD_RESPONSE_NO_ERROR))
  {
    if (!MSD_GetResponse(MSD_START_DATA_SINGLE_BLOCK_READ))
    {
      /* Store CID register value on CID_Tab */
      for (i = 0; i < 16; i++)
      {
        CID_Tab[i] = MSD_ReadByte();
      }
    }
    /* Get CRC bytes (not really needed by us, but required by MSD) */
    MSD_WriteByte(DUMMY);
    MSD_WriteByte(DUMMY);
    /* Set response value to success */
    rvalue = MSD_RESPONSE_NO_ERROR;
  }

  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte: 8 Clock pulses of delay */
  MSD_WriteByte(DUMMY);

  /* Byte 0 */
  MSD_cid->ManufacturerID = CID_Tab[0];
  /* Byte 1 */
  MSD_cid->OEM_AppliID = CID_Tab[1] << 8;
  /* Byte 2 */
  MSD_cid->OEM_AppliID |= CID_Tab[2];
  /* Byte 3 */
  MSD_cid->ProdName1 = CID_Tab[3] << 24;
  /* Byte 4 */
  MSD_cid->ProdName1 |= CID_Tab[4] << 16;
  /* Byte 5 */
  MSD_cid->ProdName1 |= CID_Tab[5] << 8;
  /* Byte 6 */
  MSD_cid->ProdName1 |= CID_Tab[6];
  /* Byte 7 */
  MSD_cid->ProdName2 = CID_Tab[7];
  /* Byte 8 */
  MSD_cid->ProdRev = CID_Tab[8];
  /* Byte 9 */
  MSD_cid->ProdSN = CID_Tab[9] << 24;
  /* Byte 10 */
  MSD_cid->ProdSN |= CID_Tab[10] << 16;
  /* Byte 11 */
  MSD_cid->ProdSN |= CID_Tab[11] << 8;
  /* Byte 12 */
  MSD_cid->ProdSN |= CID_Tab[12];
  /* Byte 13 */
  MSD_cid->Reserved1 |= (CID_Tab[13] & 0xF0) >> 4;
  /* Byte 14 */
  MSD_cid->ManufactDate = (CID_Tab[13] & 0x0F) << 8;
  /* Byte 15 */
  MSD_cid->ManufactDate |= CID_Tab[14];
  /* Byte 16 */
  MSD_cid->CRC_Check = (CID_Tab[15] & 0xFE) >> 1;
  MSD_cid->Reserved2 = 1;

  /* Return the reponse */
  return rvalue;
}

/*******************************************************************************
* Function Name  : MSD_SendCmd
* Description    : Send 5 bytes command to the MSD card.
* Input          : - Cmd: the user expected command to send to MSD card
*                  - Arg: the command argument
*                  - Crc: the CRC
* Output         : None
* Return         : None
*******************************************************************************/
void MSD_SendCmd(u8 Cmd, u32 Arg, u8 Crc)
{
  u32 i = 0x00;
  u8 Frame[6];

  /* Construct byte1 */
  Frame[0] = (Cmd | 0x40);
  /* Construct byte2 */
  Frame[1] = (u8)(Arg >> 24);
  /* Construct byte3 */
  Frame[2] = (u8)(Arg >> 16);
  /* Construct byte4 */
  Frame[3] = (u8)(Arg >> 8);
  /* Construct byte5 */
  Frame[4] = (u8)(Arg);
  /* Construct CRC: byte6 */
  Frame[5] = (Crc);

  /* Send the Cmd bytes */
  for (i = 0; i < 6; i++)
  {
    MSD_WriteByte(Frame[i]);
  }
}

/*******************************************************************************
* Function Name  : MSD_GetDataResponse
* Description    : Get MSD card data response.
* Input          : None
* Output         : None
* Return         : The MSD status: Read data response xxx0<status>1
*                   - status 010: Data accecpted
*                   - status 101: Data rejected due to a crc error
*                   - status 110: Data rejected due to a Write error.
*                   - status 111: Data rejected due to other error.
*******************************************************************************/
u8 MSD_GetDataResponse(void)
{
  u32 i = 0;
  u8 response, rvalue;

  while (i <= 64)
  {
    /* Read resonse */
    response = MSD_ReadByte();
    /* Mask unused bits */
    response &= 0x1F;

    switch (response)
    {
      case MSD_DATA_OK:
      {
        rvalue = MSD_DATA_OK;
        break;
      }

      case MSD_DATA_CRC_ERROR:
        return MSD_DATA_CRC_ERROR;

      case MSD_DATA_WRITE_ERROR:
        return MSD_DATA_WRITE_ERROR;

      default:
      {
        rvalue = MSD_DATA_OTHER_ERROR;
        break;
      }
    }
    /* Exit loop in case of data ok */
    if (rvalue == MSD_DATA_OK)
      break;
    /* Increment loop counter */
    i++;
  }
  /* Wait null data */
  while (MSD_ReadByte() == 0);
  /* Return response */
  return response;
}

/*******************************************************************************
* Function Name  : MSD_GetResponse
* Description    : Returns the MSD response.
* Input          : None
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_GetResponse(u8 Response)
{
  u32 Count = 0xFFF;

  /* Check if response is got or a timeout is happen */
  while ((MSD_ReadByte() != Response) && Count)
  {
    Count--;
  }

  if (Count == 0)
  {
    /* After time out */
    return MSD_RESPONSE_FAILURE;
  }
  else
  {
    /* Right response got */
    return MSD_RESPONSE_NO_ERROR;
  }
}

/*******************************************************************************
* Function Name  : MSD_GetStatus
* Description    : Returns the MSD status.
* Input          : None
* Output         : None
* Return         : The MSD status.
*******************************************************************************/
u16 MSD_GetStatus(void)
{
  u16 Status = 0;

  /* MSD chip select low */
  MSD_CS_LOW();
  /* Send CMD13 (MSD_SEND_STATUS) to get MSD status */
  MSD_SendCmd(MSD_SEND_STATUS, 0, 0xFF);

  Status = MSD_ReadByte();
  Status |= (u16)(MSD_ReadByte() << 8);

  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte 0xFF */
  MSD_WriteByte(DUMMY);

  return Status;
}

/*******************************************************************************
* Function Name  : MSD_GoIdleState
* Description    : Put MSD in Idle state.
* Input          : None
* Output         : None
* Return         : The MSD Response: - MSD_RESPONSE_FAILURE: Sequence failed
*                                    - MSD_RESPONSE_NO_ERROR: Sequence succeed 
*******************************************************************************/
u8 MSD_GoIdleState(void)
{
//��ʱ�˳�
#if  XING_DO
   u32 Count = 0xFFF*2;
#endif

  /* MSD chip select low */
  MSD_CS_LOW();
  /* Send CMD0 (GO_IDLE_STATE) to put MSD in SPI mode */
  MSD_SendCmd(MSD_GO_IDLE_STATE, 0, 0x95);

  /* Wait for In Idle State Response (R1 Format) equal to 0x01 */
  if (MSD_GetResponse(MSD_IN_IDLE_STATE))
  {
    /* No Idle State Response: return response failue */
    return MSD_RESPONSE_FAILURE;
  }
  /*----------Activates the card initialization process-----------*/
  do
  {
    /* MSD chip select high */
    MSD_CS_HIGH();
    /* Send Dummy byte 0xFF */
    MSD_WriteByte(DUMMY);

    /* MSD chip select low */
    MSD_CS_LOW();

    /* Send CMD1 (Activates the card process) until response equal to 0x0 */
    MSD_SendCmd(MSD_SEND_OP_COND, 0, 0xFF);
    /* Wait for no error Response (R1 Format) equal to 0x00 */

//��ʱ�˳�
#if  XING_DO
    Count--;
    if(Count==0){
       /* No Idle State Response: return response failue */
       return MSD_RESPONSE_FAILURE;
    }
#endif

  }
  while (MSD_GetResponse(MSD_RESPONSE_NO_ERROR));

  /* MSD chip select high */
  MSD_CS_HIGH();
  /* Send dummy byte 0xFF */
  MSD_WriteByte(DUMMY);

#if  XING_DO
  SD_SPI_HightSpeed();//����SPI�ٶ�
  Get_Medium_Characteristics();//��ȡ������Ϣ
  SD_Card_Ready = 0;
#else
  SD_SPI_HightSpeed();//����SPI�ٶ�
#endif

  return MSD_RESPONSE_NO_ERROR;
}


/*******************************************************************************
* Function Name  : Get_Medium_Characteristics.
* Description    : Get the microSD card size.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Get_Medium_Characteristics(void)
{
  u32 temp1 = 0;
  u32 temp2 = 0;

  MSD_GetCSDRegister(&MSD_csd);

  temp1 = MSD_csd.DeviceSize + 1;
  temp2 = 1 << (MSD_csd.DeviceSizeMul + 2);

  Mass_Block_Count = temp1 * temp2;

  Mass_Block_Size =  1 << MSD_csd.RdBlockLen;

  Mass_Memory_Size = (Mass_Block_Count * Mass_Block_Size);

}

/******************* (C) COPYRIGHT 2007 STMicroelectronics *****END OF FILE****/





//>>>>>>>>>>>>>>>>>>>>>>>>>>>start of  code<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//������: xingxing
//ʱ��  : 2011/02/27
//˵��  : ���¼���Ĵ�����Ϊ��ʹϵͳ֧�ָ���SDHC��
//------------------------------------------------

/******************************************************************************* 
*  ���ļ�ΪSPI����SD���ĵײ������ļ�
*  ����SPIģ�鼰���IO�ĳ�ʼ����SPI��дSD����дָ������ݵȣ�
*******************************************************************************/   

	 
/*******************************************************************************
* Function Name  : SD_SendCommand
* Description    : ��SD������һ������
* Input          : u8 cmd   ���� 
*                  u32 arg  �������
*                  u8 crc   crcУ��ֵ
* Output         : None
* Return         : u8 r1 SD�����ص���Ӧ
*******************************************************************************/
u8 SD_SendCommand(u8 cmd, u32 arg, u8 crc)
{
    unsigned char r1;
    unsigned char Retry = 0;

    //????????
    SPI2_ReadWriteByte(0xff);
    //Ƭѡ���õͣ�ѡ��SD��
    SD_CS_ENABLE();

    //����
    SPI2_ReadWriteByte(cmd | 0x40);                         //�ֱ�д������
    SPI2_ReadWriteByte(arg >> 24);
    SPI2_ReadWriteByte(arg >> 16);
    SPI2_ReadWriteByte(arg >> 8);
    SPI2_ReadWriteByte(arg);
    SPI2_ReadWriteByte(crc);
    
    //�ȴ���Ӧ����ʱ�˳�
    while((r1 = SPI2_ReadWriteByte(0xFF))==0xFF)
    {
        Retry++;
        if(Retry > RE_TRY_CNT)break; 
    }   
    //�ر�Ƭѡ
    SD_CS_DISABLE();
    //�������϶�������8��ʱ�ӣ���SD�����ʣ�µĹ���
    SPI2_ReadWriteByte(0xFF);

    //����״ֵ̬
    return r1;
}


/*******************************************************************************
* Function Name  : SD_SendCommand_NoDeassert
* Description    : ��SD������һ������(�����ǲ�ʧ��Ƭѡ�����к������ݴ�����
* Input          : u8 cmd   ���� 
*                  u32 arg  �������
*                  u8 crc   crcУ��ֵ
* Output         : None
* Return         : u8 r1 SD�����ص���Ӧ
*******************************************************************************/
u8 SD_SendCommand_NoDeassert(u8 cmd, u32 arg, u8 crc)
{
    unsigned char r1;
    unsigned char Retry = 0;

    //????????
    SPI2_ReadWriteByte(0xff);
    //Ƭѡ���õͣ�ѡ��SD��
    SD_CS_ENABLE();

    //����
    SPI2_ReadWriteByte(cmd | 0x40);                         //�ֱ�д������
    SPI2_ReadWriteByte(arg >> 24);
    SPI2_ReadWriteByte(arg >> 16);
    SPI2_ReadWriteByte(arg >> 8);
    SPI2_ReadWriteByte(arg);
    SPI2_ReadWriteByte(crc);

    //�ȴ���Ӧ����ʱ�˳�
    while((r1 = SPI2_ReadWriteByte(0xFF))==0xFF)
    {
        Retry++;
        if(Retry > RE_TRY_CNT)break;   
    }
    //������Ӧֵ
    return r1;
}

/*******************************************************************************
* Function Name  : SDHC_Init
* Description    : ��ʼ��SDHC��
* Input          : None
* Output         : None
* Return         : u8 
*                  0��NO_ERR
*                  1��TIME_OUT
*                  99��NO_CARD
*******************************************************************************/
u8 SDHC_Init(void)// ��ʼ������SD��,��MSD_Init����
{
    u16 i;      // ����ѭ������
    u8 r1;      // ���SD���ķ���ֵ
    u16 retry;  // �������г�ʱ����
    u8 buff[6];
					 
	/* Initialize SPI1 */
    SD_SPI_Configuration();

	  SD_CS_ENABLE();
					  
    // ����ʱ���ȴ�SD���ϵ����
    for(i=0;i<0xf00;i++);

    //�Ȳ���>74�����壬��SD���Լ���ʼ�����
    for(i=0;i<10;i++)
    {
        SPI2_ReadWriteByte(0xFF);
    }

    //-----------------SD����λ��idle��ʼ-----------------
    //ѭ����������CMD0��ֱ��SD������0x01,����IDLE״̬
    //��ʱ��ֱ���˳�
    retry = 0;
    do
    {
        //����CMD0����SD������IDLE״̬
        r1 = SD_SendCommand(CMD0, 0, 0x95);
        retry++;
    }while((r1 != 0x01) && (retry<RE_TRY_CNT));
    //����ѭ���󣬼��ԭ�򣺳�ʼ���ɹ���or ���Գ�ʱ��
    if(retry==RE_TRY_CNT) return 1;   //��ʱ����1	  
    //-----------------SD����λ��idle����-----------------	 
    //��ȡ��Ƭ��SD�汾��Ϣ
    r1 = SD_SendCommand_NoDeassert(8, 0x1aa, 0x87);	     
    //�����Ƭ�汾��Ϣ��v1.0�汾�ģ���r1=0x05����������³�ʼ��
    if(r1 == 0x05)
    {
        //���ÿ�����ΪSDV1.0����������⵽ΪMMC�������޸�ΪMMC
        SD_Type = SD_TYPE_V1;	   
        //�����V1.0����CMD8ָ���û�к�������
        //Ƭѡ�øߣ�������������
        SD_CS_DISABLE();
        //�෢8��CLK����SD������������
        SPI2_ReadWriteByte(0xFF);	  
        //-----------------SD����MMC����ʼ����ʼ-----------------	 
        //������ʼ��ָ��CMD55+ACMD41
        // �����Ӧ��˵����SD�����ҳ�ʼ�����
        // û�л�Ӧ��˵����MMC�������������Ӧ��ʼ��
        retry = 0;
        do
        {
            //�ȷ�CMD55��Ӧ����0x01���������
            r1 = SD_SendCommand(CMD55, 0, 0);
            if(r1 != 0x01)return r1;	  
            //�õ���ȷ��Ӧ�󣬷�ACMD41��Ӧ�õ�����ֵ0x00����������200��
            r1 = SD_SendCommand(ACMD41, 0, 0);
            retry++;
        }while((r1!=0x00) && (retry<RE_TRY_CNT));
        // �ж��ǳ�ʱ���ǵõ���ȷ��Ӧ
        // ���л�Ӧ����SD����û�л�Ӧ����MMC��
        
        //----------MMC�������ʼ��������ʼ------------
        if(retry==RE_TRY_CNT)
        {
            retry = 0;
            //����MMC����ʼ�����û�в��ԣ�
            do
            {
                r1 = SD_SendCommand(1, 0, 0);
                retry++;
            }while((r1!=0x00)&& (retry<RE_TRY_CNT));
            if(retry==RE_TRY_CNT)return 1;   //MMC����ʼ����ʱ		    
            //д�뿨����
            SD_Type = SD_TYPE_MMC;
        }
        //----------MMC�������ʼ����������------------	    
        //����SPIΪ����ģʽ
		SD_SPI_HightSpeed();//����SPI�ٶ�  
		SPI2_ReadWriteByte(0xFF);
        
        //��ֹCRCУ��	   
		r1 = SD_SendCommand(CMD59, 0, 0x95);
        if(r1 != 0x00)return r1;  //������󣬷���r1   	   
        //����Sector Size
        r1 = SD_SendCommand(CMD16, 512, 0x95);
        if(r1 != 0x00)return r1;//������󣬷���r1		 
        //-----------------SD����MMC����ʼ������-----------------

    }//SD��ΪV1.0�汾�ĳ�ʼ������	 
    //������V2.0���ĳ�ʼ��
    //������Ҫ��ȡOCR���ݣ��ж���SD2.0����SD2.0HC��
    else if(r1 == 0x01)
    {
        //V2.0�Ŀ���CMD8�����ᴫ��4�ֽڵ����ݣ�Ҫ�����ٽ���������
        buff[0] = SPI2_ReadWriteByte(0xFF);  //should be 0x00
        buff[1] = SPI2_ReadWriteByte(0xFF);  //should be 0x00
        buff[2] = SPI2_ReadWriteByte(0xFF);  //should be 0x01
        buff[3] = SPI2_ReadWriteByte(0xFF);  //should be 0xAA	    
        SD_CS_DISABLE();	  
        SPI2_ReadWriteByte(0xFF);//the next 8 clocks			 
        //�жϸÿ��Ƿ�֧��2.7V-3.6V�ĵ�ѹ��Χ
        //if(buff[2]==0x01 && buff[3]==0xAA) //���жϣ�����֧�ֵĿ�����
        {	  
            retry = 0;
            //������ʼ��ָ��CMD55+ACMD41
    		do
    		{
    			r1 = SD_SendCommand(CMD55, 0, 0);
    			if(r1!=0x01)return r1;	   
    			r1 = SD_SendCommand(ACMD41, 0x40000000, 0);
                if(retry>RE_TRY_CNT)return r1;  //��ʱ�򷵻�r1״̬  
            }while(r1!=0);		  
            //��ʼ��ָ�����ɣ���������ȡOCR��Ϣ		   
            //-----------����SD2.0���汾��ʼ-----------
            r1 = SD_SendCommand_NoDeassert(CMD58, 0, 0);
            if(r1!=0x00)return r1;  //�������û�з�����ȷӦ��ֱ���˳�������Ӧ��		 
            //��OCRָ����󣬽�������4�ֽڵ�OCR��Ϣ
            buff[0] = SPI2_ReadWriteByte(0xFF);
            buff[1] = SPI2_ReadWriteByte(0xFF); 
            buff[2] = SPI2_ReadWriteByte(0xFF);
            buff[3] = SPI2_ReadWriteByte(0xFF);

            //OCR������ɣ�Ƭѡ�ø�
            SD_CS_DISABLE();
            SPI2_ReadWriteByte(0xFF);

            //�����յ���OCR�е�bit30λ��CCS����ȷ����ΪSD2.0����SDHC
            //���CCS=1��SDHC   CCS=0��SD2.0
            if(buff[0]&0x40)SD_Type = SD_TYPE_V2HC;    //���CCS	 
            else SD_Type = SD_TYPE_V2;	    
            //-----------����SD2.0���汾����----------- 
            //����SPIΪ����ģʽ
			SD_SPI_HightSpeed();//����SPI�ٶ� 
        }	    
    }
    return r1;
}

