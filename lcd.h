#include <stdint.h>

#define LCD_VDD  2, 1
#define LCD_SCLK 1, 9
#define LCD_SDIN 2, 4
#define LCD_DC   2, 5
#define LCD_CD 	 0, 7
#define LCD_RES  2, 9

#define LCD_WIDTH  102
#define LCD_HEIGHT 64

void lcd_init(); 

void lcd_on(); 
void lcd_off(); 

void lcd_send_byte(unsigned char data); 
void lcd_send_data(unsigned char data); 
void lcd_send_command(unsigned char data); 

void lcd_ext_set_bias(unsigned char bias); 
void lcd_ext_set_temperature(unsigned char temperature); 
void lcd_ext_set_hv_gen(unsigned char hv_gen); 
void lcd_ext_set_voltage(unsigned char voltage); 

void lcd_ext_command_set_enable(unsigned char flag); 
void lcd_set_inversion(unsigned char flag); 
void lcd_set_visible(unsigned char flag); 
void lcd_clear(); 
void lcd_set_position(unsigned char x, unsigned char y); 
void lcd_power(unsigned char flag); 

void lcd_write_bitmap(unsigned char *bitmap); 
