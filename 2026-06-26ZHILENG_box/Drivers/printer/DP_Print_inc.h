#ifndef		_DP_PRINT_INC_H
#define		_DP_PRINT_INC_H

/*
** ���ڷ���1�ֽڵĺ��� 
** ���ɿͻ��ڸ����Լ�����ĵ�Ƭ����д
** ��������������������һ��
*/
void UART_SendByte(unsigned char Send_Dat);

/*
** ���ڽ���1�ֽڵĺ���
** ���ɿͻ��ڸ����Լ�����ĵ�Ƭ����д
*/
extern unsigned char UART_RecByte(void);


/*
** ������void jump_lattice(void)
** ��������
** ���ܣ�ÿ����һ�Σ���ӡλ������ƶ�һ���ֵķ���λ
*/
void jump_lattice(void);



/*
** ������void print_And_Line(void)
** ��������
** ���ܣ���ӡ���������ݲ���ǰ�ƽ�һ��
*/
void print_And_Line(void);


/*
** ������void Print_ASCII(unsigned char *buf)
** ������*buf ָ��Ҫ��ӡ���ַ���
** ���ܣ���Ҫ��ӡ���ַ������뻺����
** ʾ������ӡASCII abcdefg Print_ASCII("abcdefg");
*/
void Print_ASCII(unsigned char *buf);

/*
** ������void Set_Right_Interval(unsigned char interval)
** ������interval �Ҳ��ַ���� ÿ��λ0.125mm
** ���ܣ������ַ��Ҳ�ļ��Ϊinterval X 0.125mm
*/
void Set_Right_Interval(unsigned char interval);


/*
** ������void Set_Print_Mode(unsigned char optbit)
** ������optbit  ��������ĺ궨������������ֵ����ѡ��ͬ�Ĺ���ģʽ
** ���ܣ�ѡ���ӡģʽ
** ʾ������Ҫѡ������B 
					unsigned char optbit;
					EnableASCII9X17(optbit);
					Set_Print_Mode(optbit);
*/
#define		EnableASCII9X17(optbit)			optbit=(optbit|0x01)			/* ѡ������B 9*17 */
#define		EnableASCII12X24(optbit)		optbit=(optbit&0xfe)			/* ѡ������A 12*24 */
#define		EnableInverse(optbit)				optbit=(optbit|0x02)			/* ʹ�ܴ�ӡ����ģʽ */
#define		DisableInverse(optbit)			optbit=(optbit&0xfd)			/* ȡ����ӡ����ģʽ */	
#define		EnableInversion(optbit)			optbit=(optbit|0x04)			/* �������µ���ģʽ */
#define		DisableInversion(optbit)		optbit=(optbit&0xfb)			/* ȡ�����µ���ģʽ */
#define		EnableBold(optbit)					optbit=(optbit|0x08)			/* ���ô���ģʽ */
#define		DisableBold(optbit)					optbit=(optbit&0x07)			/* ȡ������ģʽ */
#define		EnableDoubleHight(optbit)		optbit=(optbit|0x10)			/* ���ñ���ģʽ */
#define		DisableDoubleHight(optbit)	optbit=(optbit&0xef)			/* ȡ������ģʽ */
#define		EnableDoubleWide(optbit)		optbit=(optbit|0x20)			/* ���ñ���ģʽ */
#define		DisableDoubleWide(optbit)		optbit=(optbit&0xdf)			/* ȡ������ģʽ */
#define		EnableUnderLine(optbit)			optbit=(optbit|0x40)			/* ʹ���»��� */
#define		DisableUnderLine(optbit)		optbit=(optbit&0xbf)			/* ȡ���»��� */
void Set_Print_Mode(unsigned char optbit);


/*
** ������void Set_Print_Position(unsigned char Lpos,unsigned char Hpos)
** ������Lpos��ӡλ����ֵ�ĵ�8λ��Hpos��ӡλ����ֵ�ĸ�8λ
** ���ܣ��趨��һ�еĿ�ʼ����Ҫ��ӡ�ַ���λ�þ���Ϊ(Lpos+Hpos*256)*0.125mm
*/
void Set_Print_Position(unsigned char Lpos,unsigned char Hpos);


/*
** ������void Set_Left_Interval(unsigned char nL, unsigned char nH)
** �������� nL �� nH ������߾࣬��߾�����Ϊ[( nL + nH��256)�������ƶ���λ]Ӣ
��.
** ���ܣ�������߼��
*/
void Set_Left_Interval(unsigned char nL, unsigned char nH);



/*
** ������void Sel_Custom_Character(unsigned char SelOpt)
** ������SelOpt  0x00 ��ʾȡ���û��Զ����ַ���
								 0x01 ��ʾѡ���û��Զ����ַ���
** ���ܣ�ѡ��/ȡ���û��Զ����ַ���
** ʾ����ѡ���Զ����ַ��� 
					Sel_Custom_Character(EnableCustomCharacter);
*/
#define		EnableCustomCharacter		(0x01)
#define		DisableCustomCharacter	(0x00)
void Sel_Custom_Character(unsigned char SelOpt);



/*
** ������void UserDefineCharacter(unsigned char yByte,unsigned char xDot, unsigned char DefChar,unsigned char *pData)
** ������yByte ��ֱ�����ֽ���
				 xDot	 ˮƽ�������
				 DefChar �Զ����ַ�
				 *pData �Զ����ַ���������
*/
void UserDefineCharacter(unsigned char yByte,unsigned char xDot, unsigned char DefChar,unsigned char *pData);


/*
** ������void Sel_Bitmap_Mode(unsigned char mode,unsigned char DatLenLowByte,unsigned char DatLenHightByte,unsigned char *pDotData)
** ������mode 0 8-�� ���ܶ�
							1 8-�� ˫�ܶ�
							32 24-�� ���ܶ�
							33 24-�� ˫�ܶ�
				 DatLenLowByte ��������ˮƽ���ȵĵ�8λ
				 DatLenHightByte ��������ˮƽ���ȵĸ�8λ
				 pDotData ָ��������ݵ�ָ��
*/
void Sel_Bitmap_Mode(unsigned char mode,unsigned char DatLenLowByte,unsigned char DatLenHightByte,unsigned char *pDotData);



/*
** ������void Set_UnderLine(unsigned char Opt)
** ������Opt 0 ����»���ģʽ
						 1 �趨1����»���
						 2 �趨2����»���
*/
void Set_UnderLine(unsigned char Opt);



/*
** ������void SetDefaultLineInterval(void)
** ���ܣ�����ȱʡ�м��3.75mm
*/
void SetDefaultLineInterval(void);


/*
** ������void Del_UserDefineCharacter(unsigned char SelCharacter)
** ������SelCharacter  32=<SelCharacter<=126
** ���ܣ�ȡ���Զ����ַ�
*/
void Del_UserDefineCharacter(unsigned char SelCharacter);


/*
** ������void InitializePrint(void)
** ���ܣ���ʼ����ӡ���������ӡ����
*/
void InitializePrint(void);

/*
** ������void SetHorizPosition(unsigned char Position)
** ������Position ˮƽ��λλ��
*/
void SetHorizPosition(unsigned char Position);



/*
** ������void SetBold(unsigned char opt)
** ������opt  ==0 ��������ӡģʽ
				      >=1 �趨�����ӡģʽ
*/
void SetBold(unsigned char opt);


/*
** ������void PrintGoPage(unsigned char nstep)
** ������nstep ��ֽ���� nstep*0.125mm
** ���ܣ���ӡ����ֽnstep*0.125mm
*/
void PrintGoPage(unsigned char nstep);


/*
** ������void SelCountryCharacter(unsigned char nsel)
** ������nsel  0<=nsel<=15
					0:����	1:����	2:�¹�	3:Ӣ��	4:����I		5:���	6:�����
					7:������I		8:�ձ�	9:Ų��	10:����II		11:������II		12:��������
					13:����		14:˹��������		15:�й�
*/
void SelCountryCharacter(unsigned char nsel);


/*
** ������void Set_Rotate(unsigned char nsel)
** ������nsel  0 ȡ��˳ʱ��90����תģʽ
							 1 ����˳ʱ��90����תģʽ
*/
void Set_Rotate(unsigned char nsel);





/*
** ������void Sel_Align_Way(unsigned char nsel)
** ������nsel  0<=nsel<=2   0 �����		1 ����		2 �Ҷ���
*/
void Sel_Align_Way(unsigned char nsel);



/*
** ������void SetReversal(unsigned char nsel)
** ������nsel 0 �رյߵ���ӡģʽ		1 �򿪵ߵ���ӡģʽ
*/
void SetReversal(unsigned char nsel);


/*
** ������void SetCharacterSize(unsigned char width,unsigned char hight)
** ������ 0<=width<=7 ��ȷŴ�1~8��  0<=hight<=7 �߶ȷŴ�1~8��
** ���ܣ������ַ���С
*/
void SetCharacterSize(unsigned char width,unsigned char hight);


/*
** ������void DownLoadBitmap(unsigned char xDot,unsigned char yDot,unsigned char *pDat)
** ������xDot ˮƽ������� yDot ��ֱ������� *pDat λͼ��������ָ�� xDot yDot���Ϊ8�ı���
*/
void DownLoadBitmap(unsigned char xDot,unsigned char yDot,unsigned char *pDat);


/*
** ������void PrintDownLoadBitmap(unsigned char mode)
** ������mode ��ӡģʽ 0 ��ͨ  1 ����  2 ����  3 4����С
** ���ܣ���ӡ��DownLoadBitmap���������λͼ����
*/
void PrintDownLoadBitmap(unsigned char mode);

/*
** ������void Set_Inverse(unsigned char opt)
** ������opt 0 ����ģʽ�ر�
						 1 ����ģʽ��
*/
void Set_Inverse(unsigned char opt);


/*
** ������void Set_LeftSpaceNum(unsigned char nL,unsigned char nH)
** ������������߿հ���Ϊ(nL+nH*256)*0.125mm
** ���ܣ�
*/
void Set_LeftSpaceNum(unsigned char nL,unsigned char nH);

/*
** ������void Set_HRIPosition(unsigned char opt)
** ������opt 0 ����ӡHRI		1	�������Ϸ�		2	�������·�		3 ��������Ϸ����·�
** ���ܣ���ӡ������ʱѡ��HRI�ַ��Ĵ�ӡλ��
*/

void Set_HRIPosition(unsigned char opt);

/*
** ������void Set_BarCodeHight(unsigned char verticalDotNum)
** ������verticalDotNum ����ĸ߶ȣ���ֱ����ĵ��� ȱʡֵ162
** ���ܣ�����������߶�
*/
void Set_BarCodeHight(unsigned char verticalDotNum);

/*
** ������void Set_BarCodeLeftSpace(unsigned char nL, unsigned char nH)
** �������� nL �� nH ������߾࣬��߾�����Ϊ[( nL + nH��256)�������ƶ���λ]Ӣ
��.
** ���ܣ�������������߼��
*/
void Set_BarCodeLeftSpace(unsigned char nL, unsigned char nH);

/*
** ������void Set_BarCodeWidth(unsigned char widthsel)
** ������widthsel (2<=widthsel<=6)  2->0.25mm	3->0.375mm 4->0.56mm 5->0.625mm 6->0.75mm
** ���ܣ�����������
*/
void Set_BarCodeWidth(unsigned char widthsel);


/*
** ������void PrintBarCode(unsigned char CodeType,unsigned char *pData,unsigned char pDataLen)
** ������CodeType								pDataLen					��������
				 65 UPC-A								11<=k<=12				  48<=d<=57
				 66 UPC-E								11<=k<=12					48<=d<=57
				 67 JAN13(EAN13)				12<=k<=13					48<=d<=57
				 68 JAN8(EAN8)					7<=k<=8						48<=d<=57
				 69 CODE39							1<=k							48<=d<=57,65<=d<=90,32,36,37,43,45,46,47
				 70 ITF									1<=k							48<=d<=57
				 71 CODABAR							1<=k							48<=d<=57,65<=d<=90,36,43,45,46,47,58
				 72 CODE93							1<=k<=255					0<=d<=127
				 73 CODE128							2<=k<=255					0<=d<=127
** ���ܣ���ӡ������
*/
void PrintBarCode(unsigned char CodeType,unsigned char *pData,unsigned char pDataLen);

/*
** ������void SetChinesemode(unsigned char opt)
** ������opt ������ĺ궨�����
** ���ܣ����ú����ַ���ӡģʽ
** ʾ����ʹ�ܱ���ʹ�ܱ���
					unsigned char optbit;
					EnableChinaDoubleWidth(optbit);
					EnableChinaDoubleHight(optbit);
					SetChinesemode(optbit);
*/
#define	EnableChinaDoubleWidth(opt)		opt=opt|0x04			/* ʹ�ܱ��� */
#define DisableChinaDoubleWidth(opt)	opt=opt&0xfb			/* ȡ������ */
#define	EnableChinaDoubleHight(opt)		opt=opt|0x08			/* ʹ�ܱ��� */
#define	DisableChinaDoubleHight(opt)	opt=opt&0xf7			/* ȡ������ */
#define	EnableChinaUnderLine(opt)			opt=opt|0x80			/* ʹ���»���*/
#define	DisableChinaUnderLine(opt)		opt=opt&0x7f			/* ȡ���»��� */
void SetChinesemode(unsigned char opt);

/*
** ������void SelChineseChar(void)
** ���ܣ�ѡ���ӡ��Ϊ���ִ�ӡģʽ
*/
void SelChineseChar(void);


/*
** ������void DisChineseChar(void)
** ���ܣ�ȡ����ӡ�����ִ�ӡģʽ
*/
void DisChineseChar(void);

/*
** ������void Set_ChineseCode(unsigned char selopt)
** ������selopt  1 UNICODE����  2 UTF-8����  3 CODEPAGE ����
** ���ܣ�ѡ�����ı����ʽ
*/
void Set_ChineseCode(unsigned char selopt);

/*
** ������void TestPrintPage(void)
** ���ܣ���ӡ�Բ�ҳ
*/
void TestPrintPage(void);


/*
** ������void PrintGratinmap(unsigned char mode,unsigned int xDot,unsigned int yDot,unsigned char *pData)
** ������mode λͼģʽ  0-->��ͨ  1-->����  2-->����   3-->�ı���С
				 xDot ˮƽ�������
				 yDot ��ֱ�������
				 *pData λͼ����
** ���ܣ���ӡ��դλͼ
*/

void PrintGratinmap(unsigned char mode,unsigned int xDot,unsigned int yDot,unsigned char *pData);


/*
** ������void Set_QRcodeMode(unsigned char mode)
** ������mode ����QR���ģ������ [mode*mode]��
*/

void Set_QRcodeMode(unsigned char mode);



/*
** ������void Set_QRCodeAdjuLevel(unsigned char level)
** ������level  level>=0x30&&level<=0x33
** ���ܣ�����QR��Ĵ���У��ˮƽ��� 
*/
void Set_QRCodeAdjuLevel(unsigned char level);


/*
**	������void Set_QRCodeBuffer(unsigned int Len,unsigned char *pData)
**	������Len ���QR�뻺�峤��  *pData ���QR�뻺������
**	���ܣ��洢QR������ݵ�QR�뻺����
*/
void Set_QRCodeBuffer(unsigned int Len,unsigned char *pData);


/*
**	������void PrintQRCode(void)
**	���ܣ���ӡQRCode��
*/
void PrintQRCode(void);


/*
** ������void Barcode_sw(unsigned char mode)
** ������mode 0��ֹ��1����
	 ���ܣ��������ӡʹ��
*/
void Barcode_sw(unsigned char mode);



#endif

