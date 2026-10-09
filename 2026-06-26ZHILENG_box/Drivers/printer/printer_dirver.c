/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#include "DP_Print_inc.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "bitmap.h"
#include "main.h"
#include "rec_dealprint.h"
/************ 以下是基于STM32F103RB/RCT6开发板串口控制打印机演示代码 *****************/
//打印时间指令
extern uint32_t start_year;
extern uint32_t start_month;
extern uint32_t start_day;
extern uint32_t start_hour;
extern uint32_t start_min;
extern uint32_t end_year;
extern uint32_t end_month;
extern uint32_t end_day;
extern uint32_t end_hour;
extern uint32_t end_min;
extern TempRecord g_records[MAX_RECORDS];
extern uint16_t g_record_count;
extern uint8_t g_total_packets;     // 总包数
extern uint8_t g_received_packets;  // 已接收包数
/*
** 函数： void select_lines(uint8_t times)
** 参数： times  换行的次数
   功能： 换行
*/
void select_lines(uint8_t times)
{
	uint8_t i;
	for(i=1;i<=times;i++)
	{
		print_And_Line();  /* 单次换行 */
	}

}

/*
** 函数： void init_putstr(uint8_t *buf, unsigned char nsel)
** 参数： *buf  字符数据  nsel  对齐方式 0居左 1居中 2居右
   描述： print_show_str()函数中的部分代码进一步封装，
	 功能：	初始化打印机和对齐方式的选择
*/
void  init_putstr(uint8_t *buf, unsigned char nsel)
{
		/* 初始化打印机 */
		InitializePrint();

		/* 0居左、1居中、2居右 */
		Sel_Align_Way(nsel);
		Print_ASCII(buf);
}

/*
** 函数： void print_show_str(void)
** 功能：	居左、居中、加粗等效果打印示例

*/
void print_show_str(void)
{
		uint8_t buf[100]={"打印文本效果展示："};
		InitializePrint(); /* 初始化打印机 */
		Print_ASCII(buf);			 /* 发送数据 */
		select_lines(2);   /* 换行两次 */
		//0 左对齐
		sprintf((char *)buf,"文字居左效果演示abc123：");
		/* 初始化打印机、选择对齐方式、发送数据 */
		init_putstr(buf,0);
		select_lines(2);
		/*  居中对齐 */
		sprintf((char *)buf,"文字居中效果演示abc123：");
		init_putstr(buf,1);
		select_lines(2);
		/* 右对齐 */
		sprintf((char *)buf,"文字居右效果演示abc123：");
		init_putstr(buf,2);
		select_lines(2);
		/* 文字加粗 */
		sprintf((char *)buf,"文字下加粗效果演示abc123：");
	  InitializePrint();
		SetBold(1);
		Print_ASCII(buf);
		select_lines(2);
		/* 下划线 1 */
		sprintf((char *)buf,"文字下划线效果演示abc123：");
		InitializePrint();
		Set_UnderLine(1);
		Print_ASCII(buf);
		select_lines(2);
}


/*
** 函数： void w_h_print(unsigned char width,unsigned char hight, unsigned char flag)
** 参数：width 放大倍宽 hight 放大倍高  flag 0表示同行放大倍数  1代表不同行的放大倍数
	 描述：	对font_type_print(void)函数的进一步封装
	 功能：设置文本放大倍数与发送数据

*/
void w_h_print(unsigned char width,unsigned char hight, unsigned char flag)
{

		uint8_t buf[60]={0};
		uint8_t buf1[10]={"放大"};
		uint8_t buf2[50]={"倍数效果演示abc123"};
		if(!flag)
		{
			sprintf((char *)buf,"%d倍",(hight+1));
		}
		else
		{
			sprintf((char *)buf,"%s%d%s",buf1,(width+1),buf2);
		}

		/* 设置字体的大小 */
		SetCharacterSize(width,hight);
		Print_ASCII(buf);

}

/*
** 函数： void font_type_print(void)
**
	 功能：高、宽、放大倍数等效果设置

*/
void font_type_print(void)
{
	  uint8_t i=0;
		uint8_t buf[50]={"同行不同高效果演示："};
		InitializePrint(); /* 初始化打印机 */
		Print_ASCII(buf);
		select_lines(2);

		/* 同行放大 i表示宽高的倍数 */
		for(i=0;i<4;i++)
		{
			w_h_print(i,i,0);
		}
		select_lines(2);

		/* 不同行放大 i表示宽高的倍数 */
		for(i=0;i<4;i++)
		{
			w_h_print(i,i,1);
			select_lines(2);
		}
		InitializePrint(); /* 初始化打印机 */
}

/*
** 函数：void mode_and_line(uint8_t *buf, uint8_t mode)
** 参数：*buf  要发送的数据   mode打印模式选择
	 描述：对font_mode_show(void)函数部分内容进一步封装
	 功能：粗体模式选择与换行

*/
void mode_and_line(uint8_t *buf, uint8_t mode)
{
	Set_Print_Mode(mode);/* 粗体模式的选择 */
	Print_ASCII(buf);
	select_lines(2);

}


/*
** 函数：void font_mode_show(void)
**
	 功能：文字的粗体设置
*/
void font_mode_show(void)
{
	uint8_t buf[100]={"文字加粗演示"};
	/* 初始化打印机*/
	InitializePrint();
	/* 文字加粗演示*/
	mode_and_line(buf, 0x08);

	sprintf((char *)buf,"文字加粗倍高演示");
	mode_and_line(buf, 0x08|0x10);

	sprintf((char *)buf,"文字加粗倍宽演示");
	mode_and_line(buf, 0x08|0x20);

	sprintf((char *)buf,"文字加粗倍高倍宽演示");
	mode_and_line(buf, 0x08|0x10|0x20);

	sprintf((char *)buf,"文字下划线演示");
	mode_and_line(buf, (~(0x08|0x10|0x20))|0x40); /* 取消加粗、倍高、倍宽，使能下划线 */
	select_lines(2);

}


/*
** 函数：void print_bitmap(void)
**
	 功能：打印图片
*/
void print_bitmap(void)
{
	/*文本打印*/
	uint8_t buf[50]={"图片打印效果演示："};
	/* 初始化打印机*/
	InitializePrint();
	Print_ASCII(buf);
	select_lines(2);

	/*图片打印*/

	InitializePrint();    								 /* 初始化打印机*/
	Sel_Align_Way(0x01);									 /* 居中对齐 */
	PrintGratinmap(0,224,220,bitbuffer1);	 /* 打印光栅位图*/

	select_lines(2);

	InitializePrint(); 	  								/* 初始化打印机*/
	Sel_Align_Way(0x01);									/* 居中对齐 */
	PrintGratinmap(0,184, 180,bitbuffer2);/* 打印光栅位图*/
	select_lines(2);

}

/*
** 函数：void Bar_class_print(uint8_t *buf, uint8_t *code_buf, uint8_t mode)
** 参数：*buf 发送的缓冲数据  *code_buf 条形码的缓冲数据   mode 条形码的类型
	 描述：对Barcode_printf(void)函数部分内容进一步封装
	 功能：条形码的类型选择与打印
*/
void Bar_class_print(uint8_t *buf, uint8_t *code_buf, uint8_t mode)
{
	unsigned char len = strlen(code_buf);
	Print_ASCII(buf);
	select_lines(2);/* 换行 */
	/* 条形码的打印及类型 */
	PrintBarCode(mode,code_buf,len);
	select_lines(2);
}


/*
** 函数：Barcode_printf(void)
**
	 功能：Barcode条形码打印
*/

void Barcode_printf(void)
{
	uint8_t buf[100]={"打印BarcodeType.CODE39演示："};
	uint8_t code_buf[128]={"123456"};
	InitializePrint();   /* 初始化打印机 */
	Barcode_sw(1);       /* 启用条形码打印功能 */
	Set_HRIPosition(2);  /* 设置条形码打印的位置 */
	Bar_class_print(buf, code_buf, 69);

  sprintf((char *)buf,"打印BarcodeType.CODABAR演示");
	Bar_class_print(buf, code_buf, 71);


	sprintf((char *)buf,"打印BarcodeType.ITF演示");
	Bar_class_print(buf, code_buf, 70);

	sprintf((char *)buf,"打印BarcodeType.CODE93演示");
	Bar_class_print(buf, code_buf, 72);

	sprintf((char *)buf,"打印BarcodeType.CODE128演示");
  Bar_class_print(buf, code_buf, 73);

	sprintf((char *)buf,"打印BarcodeType.UPC_A演示");
	sprintf((char *)code_buf,"000000000000");
	Bar_class_print(buf, code_buf, 65);

	sprintf((char *)buf,"打印BarcodeType.UPC_E演示");
	Bar_class_print(buf, code_buf, 66);

	sprintf((char *)buf,"打印BarcodeType.JAN13演示");
	Bar_class_print(buf, code_buf, 67);

	sprintf((char *)code_buf,"00000000");
	sprintf((char *)buf,"打印BarcodeType.JAN8演示");
	Bar_class_print(buf, code_buf, 68);
}


/*
** 函数：QR_code_print(void)
**
	 功能：二维码打印
*/
void QR_code_print(void)
{
	/*  QR_code  */
	uint8_t pdata[5]={"ABC"};
	InitializePrint(); 										/* 初始化打印机 */
	//Set_QRcodeMode(0x03);									/* 设置二维码大小 */
	Set_QRCodeAdjuLevel(0x49);								/* 设置二维码的纠错水平 */
	Set_QRCodeBuffer(strlen((const char *)pdata),pdata);	/* 传输数据至编码缓存 */
	Sel_Align_Way(0x01);									/* 居中对齐 */
	PrintQRCode();											/* 打印编码缓存的二维条码 */
	select_lines(1);										/* 换行 */
	InitializePrint();    									/* 初始化打印机 */

	/* 文本打印 */
	sprintf((char *)pdata,"Scan and pay attention");
	Sel_Align_Way(0x01);
	SetCharacterSize(0,0);					/* 设置文本大小 */
	Print_ASCII(pdata);						/* 打印文本 */
	select_lines(2);

}

/*
**
** 功能：	打印特定时间的温度 湿度数据
//打印时间指令
extern uint32_t start_year;
extern uint32_t start_month;
extern uint32_t start_day;
extern uint32_t start_hour;
extern uint32_t start_min;
extern uint32_t end_year;
extern uint32_t end_month;
extern uint32_t end_day;
extern uint32_t end_hour;
extern uint32_t end_min;
*/
void print_temp(void)
{
		uint8_t buf[100]={"打印编号：60103101"};
		InitializePrint(); /* 初始化打印机 */
		Print_ASCII(buf);			 /* 发送数据 */
//		select_lines(2);   /* 换行两次 */
		//0 左对齐
		sprintf((char *)buf,"开始时间:%ld-%ld-%ld %ld:%ld",start_year,start_month,start_day,start_hour,start_min);
		/* 初始化打印机、选择对齐方式、发送数据 */
		init_putstr(buf,0);
//		select_lines(2);
		/*  居中对齐 */
		sprintf((char *)buf,"结束时间:%ld-%ld-%ld %ld:%ld",end_year,end_month,end_day,end_hour,end_min);
		init_putstr(buf,0);
//		select_lines(2);
		/* 右对齐 */
		sprintf((char *)buf,"时间  温度1  湿度1  温度2  湿度2");
		init_putstr(buf,1);
//		select_lines(2);
		/* 文字加粗 */
		sprintf((char *)buf,"%ld-%ld-%ld",start_year,start_month,start_day);
		init_putstr(buf,0);
		select_lines(5);
		/* 下划线 1 */
//		sprintf((char *)buf,"文字下划线效果演示abc123：");
//		InitializePrint();
//		Set_UnderLine(1);
//		Print_ASCII(buf);
//		select_lines(2);
}

// 打印温湿度记录
void print_temp_records(void)
{
    if (g_record_count == 0) return;

    // 先排序
    sort_records();

    // 最早和最晚记录（排序后第一个和最后一个）
    TempRecord *first = &g_records[0];
    TempRecord *last = &g_records[g_record_count - 1];

    uint16_t start_year = first->year;
    uint8_t start_month = first->month;
    uint8_t start_day = first->day;
    uint8_t start_hour = first->hour;
    uint8_t start_min = first->minute;

    uint16_t end_year = last->year;
    uint8_t end_month = last->month;
    uint8_t end_day = last->day;
    uint8_t end_hour = last->hour;
    uint8_t end_min = last->minute;

    // 初始化打印机
    InitializePrint();

    uint8_t buf[100];

    // 打印编号
    sprintf((char *)buf, "打印编号：60103101");
    Print_ASCII(buf);

    // 打印开始时间
    sprintf((char *)buf, "开始时间:%ld-%ld-%ld %ld:%ld",
            (long)start_year, (long)start_month, (long)start_day,
            (long)start_hour, (long)start_min);
    init_putstr(buf, 0);

    // 打印结束时间
    sprintf((char *)buf, "结束时间:%ld-%ld-%ld %ld:%ld",
            (long)end_year, (long)end_month, (long)end_day,
            (long)end_hour, (long)end_min);
    init_putstr(buf, 0);

    // 打印表头（居中对齐）
    sprintf((char *)buf, "时间  温度1  湿度1  温度2  湿度2");
    init_putstr(buf, 1);


    // 逐条打印记录，按日期分组
    uint16_t prev_year = 0, prev_month = 0, prev_day = 0;
    for (uint16_t i = 0; i < g_record_count; i++) {
        TempRecord *r = &g_records[i];

        // 如果日期变化，打印新日期行
        if (r->year != prev_year || r->month != prev_month || r->day != prev_day) {
            sprintf((char *)buf, "%d-%d-%d", r->year, r->month, r->day);
            init_putstr(buf, 0);
            select_lines(1);  // 换一行
            prev_year = r->year;
            prev_month = r->month;
            prev_day = r->day;
        }

        // 处理温度的小数部分（可能为负）
        int16_t t1_int = r->temp1 / 10;
        int16_t t1_frac = r->temp1 % 10;
        if (t1_frac < 0) t1_frac = -t1_frac;

        int16_t t2_int = r->temp2 / 10;
        int16_t t2_frac = r->temp2 % 10;
        if (t2_frac < 0) t2_frac = -t2_frac;

        // 构造记录行（使用空格分隔）
        sprintf((char *)buf, "%02d:%02d   %d.%d   %d    %d.%d    %d",
                r->hour, r->minute,
                t1_int, t1_frac, r->humi1,
                t2_int, t2_frac, r->humi2);
        init_putstr(buf, 0);
        select_lines(1);  // 换一行
        vTaskDelay(2);
    }
    select_lines(2);  // 换两行
}
