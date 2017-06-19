/*
	LCD A55:
		VDD		|		PIO2_1
		SCLK	|		PIO1_9
		SDIN	|		PIO2_4
		DC		|		PIO2_5
		CD		|		PIO0_7
		RES		|		PIO2_9
	
	ADC:
		Фотодиод1	|	PIO1_0
		Фотодиод2	|	PIO1_1
	
	Между выводом ADC и землёй - резистор, который должен быть подобран в режиме CHRON_DEBUG_ADC
	Что бы при выключенных светодиодах было значение близкое к нулю, а при включенных - намного больше ноля. 
	В случае использования IR светодиодов как фотодиодов подходит значение около 830 кОм
	
	Светодиоды подключаются через гасящие резисторы к 3.3V микроконтроллёра
*/

#include <chip.h>
#include <board.h>
#include <adc_13xx.h>
#include <stdarg.h>
#include <string.h>
#include <stopwatch.h>

#include "printf.h"
#include "lcd.h"

#define CHRON_DEBUG_ADC		0	// 1, если нужен постоянный вывод min/max на ADC

#define DISTANCE	0.1 // 10 см

uint32_t time0 = 0;
uint32_t time1 = 0;

uint32_t max0 = 0, max1 = 0;
uint32_t min0 = 99999, min1 = 99999;

uint32_t calibrate = 0;

void uart_putc(void *p, char c) {
	Chip_UART_SendBlocking(LPC_USART, &c, 1);
}

// Вывод отдельного сегмента цифры в стиле сегментного LCD
void show_segment(uint8_t *screen, uint8_t seg, uint8_t bx, uint8_t by) {
	struct {
		uint8_t w;
		uint8_t h;
		uint8_t data[7 * 21];
	} segments[] = {
		{ // L
			.w = 5, 
			.h = 15, 
			.data = {
				1, 1, 0, 0, 0, 
				1, 1, 1, 0, 0, 
				1, 1, 1, 1, 0, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 0, 
				1, 1, 1, 0, 0, 
				1, 1, 0, 0, 0, 
			}
		}, 
		{ // R
			.w = 5, 
			.h = 15, 
			.data = {
				0, 0, 0, 1, 1, 
				0, 0, 1, 1, 1, 
				0, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 
				0, 1, 1, 1, 1, 
				0, 0, 1, 1, 1, 
				0, 0, 0, 1, 1, 
			}
		}, 
		{ // U
			.w = 15, 
			.h = 5, 
			.data = {
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
				0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
				0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 
				0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
			}
		}, 
		{ // D
			.w = 15, 
			.h = 5, 
			.data = {
				0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
				0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 
				0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
			}
		}, 
		{ // M
			.w = 15, 
			.h = 5, 
			.data = {
				0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
				0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 
				0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
				0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 
				0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 
			}
		}
	};
	
	for (uint8_t y = 0; y < segments[seg].h; ++y) {
		for (uint8_t x = 0; x < segments[seg].w; ++x)
			screen[(bx + x) + (by + y) * LCD_WIDTH] = segments[seg].data[x + y * segments[seg].w];
	}
}

// Вывод нескольких сегментов цифры
void show_segments(uint8_t *screen, uint8_t flags, uint8_t x, uint8_t y) {
	// L
	if (flags & (1 << 0))
		show_segment(screen, 0, x + 0, y + 3);
	if (flags & (1 << 1))
		show_segment(screen, 0, x + 0, y + 19);
	
	// R
	if (flags & (1 << 2))
		show_segment(screen, 1, x + 16, y + 3);
	if (flags & (1 << 3))
		show_segment(screen, 1, x + 16, y + 19);
	
	// U + D
	if (flags & (1 << 4))
		show_segment(screen, 2, x + 3, y + 0);
	if (flags & (1 << 5))
		show_segment(screen, 3, x + 3, y + 33);
	
	// M
	if (flags & (1 << 6))
		show_segment(screen, 4, x + 3, y + 16);
}

// Конвертер цифры в нужную комбинацию сегментов
uint8_t num2flags(uint8_t n) {
	uint8_t map[] = {
		(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5),			// 0
		(1 << 2) | (1 << 3),														// 1
		(1 << 4) | (1 << 2) | (1 << 6) | (1 << 1) | (1 << 5),						// 2
		(1 << 4) | (1 << 5) | (1 << 6) | (1 << 2) | (1 << 3),						// 3
		(1 << 0) | (1 << 2) | (1 << 6) | (1 << 3),									// 4
		(1 << 4) | (1 << 0) | (1 << 6) | (1 << 3) | (1 << 5),						// 5
		(1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6),			// 6
		(1 << 0) | (1 << 2) | (1 << 3) | (1 << 4), 									// 7
		(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6),	// 8
		(1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6),			// 9
		(1 << 6), 																	// -
		(1 << 4) | (1 << 5) | (1 << 6) | (1 << 0) | (1 << 1),						// E
		(1 << 4) | (1 << 5) | (1 << 0) | (1 << 1)									// C
	};
	return map[n];
}

// Функция вывода 4-х цифр + точки
void show_numbers(uint8_t *screen, uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4) {
	uint8_t x = 4;
	show_segments(screen, num2flags(n1), x, 13);
	x += 21 + 3;
	show_segments(screen, num2flags(n2), x, 13);
	x += 21 + 3;
	show_segments(screen, num2flags(n3), x, 13);
	x += 21 + 3;
	show_segments(screen, num2flags(n4), x, 13);
	
	screen[74 + 49 * LCD_WIDTH] = 1;
	screen[74 + 50 * LCD_WIDTH] = 1;
	screen[75 + 49 * LCD_WIDTH] = 1;
	screen[75 + 50 * LCD_WIDTH] = 1;
}

void ADC_IRQHandler() {
	if (Chip_ADC_ReadStatus(LPC_ADC, ADC_CH1, ADC_DR_DONE_STAT) == SET) {
		uint16_t adc = 0;
		Chip_ADC_ReadValue(LPC_ADC, ADC_CH1, &adc);
		if (calibrate) {
			// Калибровка - считаем максимальное и минимальное значение
			if (adc < min0)
				min0 = adc;
			if (adc > max0)
				max0 = adc;
		} else {
			// Если значение ADC упало до минимального порога - пуля пролетела черех первую оптопару
			if (adc < min0 && time0 == 0)
				time0 = StopWatch_Start();
		}
	}
	
	if (Chip_ADC_ReadStatus(LPC_ADC, ADC_CH2, ADC_DR_DONE_STAT) == SET) {
		uint16_t adc = 0;
		Chip_ADC_ReadValue(LPC_ADC, ADC_CH2, &adc);
		if (calibrate) {
			// Калибровка - считаем максимальное и минимальное значение ADC
			if (adc < min1)
				min1 = adc;
			if (adc > max1)
				max1 = adc;
		} else {
			// Если значение ADC упало до минимального порога - пуля пролетела черех вторую оптопару
			if (adc < min1 && time1 == 0)
				time1 = StopWatch_Start();
		}
	}
	
	if (CHRON_DEBUG_ADC) {
		// Режим отладки, выводит min и max на входе ADC за 1 секунду
		uint32_t now = StopWatch_Start();
		if (StopWatch_TicksToMs(now - calibrate) >= 1000) {
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, DISABLE);
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, DISABLE);
			tfp_printf("[0] CH0: min=%d, max=%d, d=%d\r\n", min0, max0, max0 - min0);
			tfp_printf("[0] CH1: min=%d, max=%d, d=%d\r\n", min1, max1, max1 - min1);
			tfp_printf("\r\n");
			min0 = min1 = 99999;
			max0 = max1 = 0;
			calibrate = StopWatch_Start();
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, ENABLE);
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, ENABLE);
		}
	}
}

int main() {
	init_printf(NULL, uart_putc);
	SystemCoreClockUpdate();
	Board_Init();
	Board_ADC_Init();
	StopWatch_Init();
	
	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaud(LPC_USART, 115200);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);
	
	SysTick_Config(SystemCoreClock);
	
	tfp_printf("init - ok\r\n");
	
	uint8_t screen[LCD_WIDTH * LCD_HEIGHT] = {0};
	
	lcd_init();
	lcd_on();
	
	memset(screen, 0, sizeof(screen));
	show_numbers(screen, 10, 10, 10, 10);
	lcd_write_bitmap(screen);
	
	// Есть секунда, что бы быстро убрать пальцы от МК
	// Что бы не создавать наводки для ADC во время калибровки
	// Актуально для кустарного поделия, где нет корпуса
	StopWatch_DelayMs(1000);
	
	memset(screen, 0, sizeof(screen));
	show_numbers(screen, 12, 10, 10, 10);
	lcd_write_bitmap(screen);
	
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_0, IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_ADMODE_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_1, IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_ADMODE_EN);
	
	ADC_CLOCK_SETUP_T clk;
	Chip_ADC_Init(LPC_ADC, &clk);
	Chip_ADC_EnableChannel(LPC_ADC, ADC_CH1, ENABLE);
	Chip_ADC_EnableChannel(LPC_ADC, ADC_CH2, ENABLE);
	Chip_ADC_SetBurstCmd(LPC_ADC, ENABLE);
	
	Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, ENABLE);
	Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, ENABLE);
	NVIC_EnableIRQ(ADC_IRQn);
	
	calibrate = StopWatch_Start();
	
	if (CHRON_DEBUG_ADC) {
		// Режим отладки, дальше не идём
		while (1);
	}
	
	uint32_t err = 1;
	while (1) {
		uint32_t now = StopWatch_Start();
		if (calibrate) {
			/*
				Режим каллибровки!
				Высчитываем минимальный уровнь ADC для срабатывания по простой формуле:
					thresold = min - (max - min) / 2
				Где min и max - максимальный и минимальный уровень ADC за 1 секунду
				А thresold - итогове минимальное значение для срабатывания.
			*/
			if (StopWatch_TicksToMs(now - calibrate) >= 1000) {
				Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, DISABLE);
				Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, DISABLE);
				tfp_printf("[0] CH0: min=%d, max=%d\r\n", min0, max0);
				tfp_printf("[0] CH1: min=%d, max=%d\r\n", min1, max1);
				calibrate = 0;
				min0 -= (max0 - min0) / 2;
				min1 -= (max1 - min1) / 2;
				tfp_printf("[1] CH0: min=%d, max=%d\r\n", min0, max0);
				tfp_printf("[1] CH1: min=%d, max=%d\r\n", min1, max1);
				
				Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, ENABLE);
				Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, ENABLE);
				continue;
			}
			continue;
		}
		
		// Не имеет значения, в какую сторону стрелять
		uint32_t start = MIN(time0, time1);
		uint32_t end = MAX(time0, time1);
		
		if (err > 6) {
			// Выводим ошибку на LCD в течении 6 * 300 ms
			memset(screen, 0, sizeof(screen));
			show_numbers(screen, 10, 10, 10, 10);
			lcd_write_bitmap(screen);
			err = 0;
		}
		if (err)
			++err;
		
		// Сработал только один датчик, выводим ошибку
		if (start == 0 && end > 0 && StopWatch_TicksToMs(now - end) >= 1000) {
			// Отключаем прерывания ADC
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, DISABLE);
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, DISABLE);
			
			tfp_printf("ERR: time0=%d, time1=%d\r\n", time0, time1);
			
			memset(screen, 0, sizeof(screen));
			show_numbers(screen, 11, time0 == 0 ? 0 : 1, 10, 10);
			lcd_write_bitmap(screen);
			
			time0 = 0;
			time1 = 0;
			err = 1;
			
			// Включаем прерывания ADC
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, ENABLE);
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, ENABLE);
		}
		
		// Сработало оба датчика, можно вычислить скорость
		if (start && end) {
			// Отключаем прерывания ADC
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, DISABLE);
			Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, DISABLE);
			
			uint32_t time = StopWatch_TicksToUs(end - start);
			float velocity = (1000000 * DISTANCE) / time;
			
			uint32_t velocity_floor = (uint32_t) velocity;
			
			uint32_t n1 = velocity_floor / 100;
			uint32_t n2 = (velocity_floor - (velocity_floor / 100 * 100)) / 10;
			uint32_t n3 = velocity_floor - (velocity_floor / 10 * 10);
			uint32_t n4 = ((velocity - velocity_floor) * 10);
			
			tfp_printf("%d us, %d%d%d,%d m/s ticks=%d\r\n", time, n1, n2, n3, n4, end - start);
			
			if (velocity_floor > 999) {
				// Расчитали слишком большое значение!
				memset(screen, 0, sizeof(screen));
				show_numbers(screen, 11, 2, 10, 10);
				lcd_write_bitmap(screen);
				err = 1;
				
				time0 = 0;
				time1 = 0;
				
				// Включаем прерывания ADC
				Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH1, ENABLE);
				Chip_ADC_Int_SetChannelCmd(LPC_ADC, ADC_CH2, ENABLE);
			} else {
				// 123
				memset(screen, 0, sizeof(screen));
				show_numbers(screen, n1, n2, n3, n4);
				lcd_write_bitmap(screen);
				
				// Если расчитали скорость - обратно прерывания не включаем
				// Следующий расчёт - только через кнопку RESET на контроллёре
				// Очень лениво паять ещё одну кнопку на GPIO, когда есть существующая с аналогичной функцией =)
				break;
			}
		}
		
		StopWatch_DelayMs(300);
	}
	
	while (1);
	
	return 0;
}

