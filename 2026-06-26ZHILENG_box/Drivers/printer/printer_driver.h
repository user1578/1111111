#include "main.h"
#include "DP_Print_inc.h"
#include "string.h"
#include "stdio.h"
//#include "bitmap.h"


void  select_lines(uint8_t times);
void  init_putstr(uint8_t *buf, unsigned char nsel);
void  print_show_str(void);
void  w_h_print(unsigned char width,unsigned char hight, unsigned char flag);
void font_type_print(void);
void mode_and_line(uint8_t *buf, uint8_t mode);
void font_mode_show(void);
void print_bitmap(void);
void Bar_class_print(uint8_t *buf, uint8_t *code_buf, uint8_t mode);
void Barcode_printf(void);
void QR_code_print(void);
void print_temp_records(void);
