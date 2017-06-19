#include <chip.h>
#include <board.h>
#include <stopwatch.h>

#include "lcd.h"

void lcd_init() {
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, LCD_VDD);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, LCD_SCLK);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, LCD_SDIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, LCD_DC);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, LCD_CD);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, LCD_RES);
}

void lcd_on() {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_RES, 0);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_VDD, 0);
	StopWatch_DelayMs(1);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_DC, 1);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_CD, 1);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_RES, 1);
	StopWatch_DelayMs(1);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_VDD, 1);
	
	lcd_ext_command_set_enable(1); 
	lcd_ext_set_bias(0x06); 
	lcd_ext_set_hv_gen(0x01); 
	lcd_ext_set_temperature(0x02); 
	lcd_ext_set_voltage(0x44); 
	lcd_ext_command_set_enable(0); 
	lcd_set_inversion(0); 
	StopWatch_DelayMs(1); 
}

void lcd_off() {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_VDD, 0);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_SCLK, 0);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_SDIN, 0);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_DC, 0);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_CD, 0);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_RES, 0);
	StopWatch_DelayMs(1);
}

void lcd_send_byte(unsigned char b) {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_CD, 0);
	for (int i = 7; i >= 0; i--) {
		Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_SCLK, 0);
		Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_SDIN, (b >> i & 0x1));
		Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_SCLK, 1);
	}
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_CD, 1);
}

void lcd_send_data(unsigned char data) {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_DC, 1);
	lcd_send_byte(data); 
}

void lcd_send_command(unsigned char data) {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, LCD_DC, 0);
	lcd_send_byte(data); 
}

void lcd_ext_command_set_enable(unsigned char flag) {
	lcd_send_command(flag ? 0b00100001 : 0b00100000); 
}

void lcd_set_inversion(unsigned char flag) {
	lcd_send_command(flag ? 0b00001101 : 0b00001100); 
}

void lcd_set_visible(unsigned char flag) {
	lcd_send_command(flag ? 0b00001001 : 0b00001000); 
}

void lcd_write_bitmap(unsigned char *bitmap) {
	unsigned char pixel = 0; 
	lcd_set_position(0, 0);
	for (int y = LCD_HEIGHT / 8 - 1; y >= 0; y--) {
		for (int x = LCD_WIDTH - 1; x >= 0; x--) {
			pixel = 0; 
			for (int i = 0; i < 8; i++)
				pixel += (bitmap[(y * 8 + (7 - i)) * LCD_WIDTH + x] ? 1 : 0) * (1 << i); 
			lcd_send_data(pixel); 
		}
	}
}

void lcd_clear() {
	lcd_set_position(0, 0); 
	for (unsigned short i = 0; i < LCD_WIDTH * LCD_HEIGHT / 8; i++)
		lcd_send_data(0b00000000); 
	lcd_set_position(0, 0); 
}

void lcd_set_position(unsigned char x, unsigned char y) {
	lcd_send_command(128 + x); 
	lcd_send_command(64 + y); 
}

void lcd_power(unsigned char flag) {
	lcd_send_command(flag ? 0b00100100 : 0b00100000); 
}

void lcd_ext_set_bias(unsigned char bias) {
	bias = bias > 7 ? 7 : bias; 
	lcd_send_command(0b00010000 + bias); 
}

void lcd_ext_set_temperature(unsigned char temperature) {
	temperature = temperature > 3 ? 3 : temperature; 
	lcd_send_command(0b00000100 + temperature); 
}

void lcd_ext_set_hv_gen(unsigned char hv_gen) {
	hv_gen = hv_gen > 3 ? 3 : hv_gen; 
	lcd_send_command(0b00001000 + hv_gen); 
}

void lcd_ext_set_voltage(unsigned char voltage) {
	lcd_send_command(0b10000000 + voltage); 
}
