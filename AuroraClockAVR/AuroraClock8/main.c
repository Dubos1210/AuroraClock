/*
 * AuroraClock8.c
 *
 * Created: 18.06.2021 8:56:07
 * Author : dubos
 */ 

#include "settings.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include "I2C.h"
#include "SSD1306.h"
#include "DS3231.h"

#define EEPROM_ALARM_HOUR	0x10		//Час будильника
#define EEPROM_ALARM_MIN	0x11		//Минута будильника
#define EEPROM_ALARM_SIG	0x12		//Сигнал будильника
#define EEPROM_ALARM_DAWN	0x13		//Длительность рассвета
#define EEPROM_LED_HALF		0x20		//Яркость ночника

#define led_max				0xFF
static uint8_t led_half	= 0x2F;
//static const uint16_t led_exp[256] PROGMEM = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 26, 28, 29, 30, 32, 33, 35, 36, 38, 39, 41, 43, 44, 46, 48, 49, 51, 53, 55, 57, 59, 60, 62, 64, 66, 69, 71, 73, 75, 77, 79, 82, 84, 86, 88, 91, 93, 96, 98, 101, 103, 106, 108, 111, 114, 116, 119, 122, 125, 127, 130, 133, 136, 139, 142, 145, 148, 151, 154, 157, 160, 164, 167, 170, 173, 177, 180, 184, 187, 190, 194, 197, 201, 204, 208, 212, 215, 219, 223, 227, 230, 234, 238, 242, 246, 250, 254, 258, 262, 266, 270, 274, 278, 282, 287, 291, 295, 300, 304, 308, 313, 317, 322, 326, 331, 335, 340, 345, 349, 354, 359, 363, 368, 373, 378, 383, 388, 393, 398, 403, 408, 413, 418, 423, 428, 434, 439, 444, 449, 455, 460, 465, 471, 476, 482, 487, 493, 498, 504, 510, 515, 521, 527, 533, 538, 544, 550, 556, 562, 568, 574, 580, 586, 592, 598, 604, 611, 617, 623, 629, 636, 642, 648, 655, 661, 668, 674, 681, 687, 694, 700, 707, 714, 720, 727, 734, 741, 748, 755, 761, 768, 775, 782, 789, 796, 804, 811, 818, 825, 832, 839, 847, 854, 861, 869, 876, 884, 891, 899, 906, 914, 921, 929, 937, 944, 952, 960, 968, 975, 983, 991, 999, 1007, 1015, 1023};

static const uint8_t days[7][12] = {"Понедельник", "  Вторник  ", "   Среда   ", "  Четверг  ", "  Пятница  ", "  Суббота  ", "Воскресенье"};

#define AL_OFF			1
#define ALARM			2
#define DAWN			3
static uint8_t alarm_flags = (0<<DAWN)|(0<<ALARM)|(0<<AL_OFF);
#define SOUND_ALARM_EN	1
#define LIGHT_ALARM_EN	0
unsigned char rtc_hours, rtc_minutes, rtc_day;
unsigned char alarm_hour = 10, alarm_minutes = 26, alarm_signal = 3, dawn_dur=10;
static char str[21];

//static void beep(uint16_t freq, uint16_t dur_ms);
static void buzz(uint16_t dur_ms);
static void lamp(uint8_t pwm);
static void menu(void);
static char menu_item(char * name, char * value, unsigned char min, unsigned char max);
static void saveSettings(void);
static void eeprom_save_byte(uint8_t* addr, uint8_t val);

int main(void) {
	//BUZZER
	DDRD |= (1<<4);
	PORTD &=~ (1<<4);
	#ifdef BUZ_EN
		/*beep(440, 25);
		beep(554, 25);
		beep(659, 25);
		beep(880, 25);*/
		buzz(25);
		_delay_ms(10);
		buzz(25);
	#endif

	OLED_init();
	
	OLED_fill(0x00);
	OLED_printString(0, 0, 2, "Аврора");
	OLED_printString(0, 24, 1, "Будильник, с которым");
	OLED_printString(0, 32, 1, "  просыпаться легко!");
	OLED_printString(0, 48, 1, "Дубишкин        v0.1");

	DS3231_init();	

	DDRB &=~ (1<<2)|(1<<3);
	PORTB |= (1<<2)|(1<<3);
	//Timer/Counter 0	-	Buttons polling
	TCCR0 = (1<<CS02)|(0<<CS01)|(1<<CS00);
	TIMSK |= (1<<TOIE0);

	DDRB |= (1<<1);
	//Timer/Counter 1	-	LED PWM
	TCCR1A = (1<<COM1A1)|(0<<COM1A0)|(0<<COM1B1)|(0<<COM1B0)|(0<<FOC1A)|(0<<FOC1B)|(1<<WGM11)|(1<<WGM10);
	TCCR1B = (0<<ICNC1)|(0<<ICES1)|(0<<WGM13)|(1<<WGM12)|(0<<CS12)|(0<<CS11)|(1<<CS10);
	OCR1A = 0;
	
	/*//External interrupt
	MCUCR |= (0<<ISC11)|(0<<ISC10)|(1<<ISC01)|(0<<ISC00);
	GICR |= (0<<INT1)|(1<<INT0);*/
	
	//Read settings from EEPROM
	alarm_hour = eeprom_read_byte((uint8_t*) EEPROM_ALARM_HOUR);
	alarm_minutes = eeprom_read_byte((uint8_t*) EEPROM_ALARM_MIN);
	alarm_signal = eeprom_read_byte((uint8_t*) EEPROM_ALARM_SIG);
	dawn_dur = eeprom_read_byte((uint8_t*) EEPROM_ALARM_DAWN);
	led_half = eeprom_read_byte((uint8_t*) EEPROM_LED_HALF);

	sei();

	_delay_ms(2000);
	OLED_fill(0x00);
	//str[0] = 127; str[1] = 0x20; str[2] = 126;
	//OLED_displayOff();
	
	while (1) 
	{
		//Вывод времени
		DS3231_getTime(&rtc_hours, &rtc_minutes);
		sprintf(str, "%2u", rtc_hours);
		OLED_printString(0, 0, 6, str);
		sprintf(str, ":%02u", rtc_minutes);
		OLED_printString(64, 16, 4, str);
		
		//Вывод дня недели
		DS3231_getDay(&rtc_day);
		if(rtc_day <= 7) OLED_printString(3, 50, 2, (char *) days[rtc_day-1]);
		
		//Вывод иконок состояния
		if(alarm_signal & (1<<LIGHT_ALARM_EN)) str[0] = 127;
		else str[0] = 0x20;
		if(alarm_signal & (1<<SOUND_ALARM_EN)) str[2] = 126;
		else str[2] = 0x20;
		str[1] = 0x20; str[3] = 0;
		//sprintf(str, "%u", alarm_flags);
		OLED_printString(110, 0, 1, str);


		//Будильник
		uint16_t rtc_time = rtc_hours*60 + rtc_minutes;
		uint16_t alarm_time = alarm_hour*60 + alarm_minutes;
		//if(alarm_time < dawn_dur) alarm_time += 

		if((alarm_signal) && (~alarm_flags & (1<<AL_OFF)) && (alarm_time - rtc_time <= dawn_dur)) {	//Будильник включён И будильник не отключили И приближается время
			if((alarm_signal & (1<<LIGHT_ALARM_EN)) && (rtc_time < alarm_time)) {				//Рассвет
				alarm_flags |= (1<<DAWN);
				uint16_t dawn_start_time = alarm_time - dawn_dur;
				uint16_t dawn_moment = rtc_time - dawn_start_time;
				lamp(led_max * dawn_moment / dawn_dur);
				//sprintf(str, "%3u %3u", dawn_moment, dawn_dur);
				//OLED_printString(0, 8, 1, str);
			}
			else if((alarm_signal & (1<<SOUND_ALARM_EN))) {										//Звуковой сигнал
				alarm_flags |= (1<<ALARM);
				uint8_t counter = 0;
				while((~alarm_flags & (1<<AL_OFF)) && (++counter <= 10)) {
					//beep(432, 1000);
					buzz(500);
					_delay_ms(1500);
				}
				OLED_printString(3, 50, 2, "Доброе утро");
			}
			_delay_ms(5000);
		}
		else {
			_delay_ms(10000);
			OLED_displayOff();
		}

		//if(rtc_time > alarm_time) flags &=~ (1<<DAWN)|(1<<ALARM)|(1<<AL_OFF);		//Если время будильника прошло, сбрасываем его флаги
		if(rtc_time > alarm_time) alarm_flags = 0;		//Если время будильника прошло, сбрасываем его флаги

	}
}

/*ISR(INT0_vect) {
	GICR &=~ (1<<INT0);

	OLED_displayOn();
	
	GIFR |= (INTF0);
	GICR |= (1<<INT0);
}*/

ISR(TIMER0_OVF_vect) {
	//TIMSK &=~ (1<<TOIE0);
	TCCR0 = 0;

	if(~PINB & (1<<3)) {		//По второй кнопке подсвечиваем дисплей
		if(alarm_flags & ((1<<DAWN)|(1<<ALARM))) alarm_flags |= (1<<AL_OFF);	//Если сейчас рассвет или звонок будильника, отключаем будильник
		OLED_displayOn();	
	}
	if(~PINB & (1<<2)) {
		if(~PINB & (1<<3)) {	//Настройки
			menu();
			#ifdef BUZ_EN
				//beep(440, 25);
				buzz(25);
			#endif
			OLED_fill(0x00);
			saveSettings();
		}
		else {					//Нажата только одна кнопка - режим фонаря
			if(OCR1A > 0) lamp(0);		//Если фонарь горит, быстро гасим его
			else {						//Если фонарь не горит, зажигаем его
				lamp(led_half);
				int8_t btn_counter = 0;
				while((~PINB & (1<<2)) && (btn_counter++ < 20)) _delay_ms(50);
				if(btn_counter >= 20) lamp(led_max);	//Если кнопка удерживается
				//if(~PINB & (1<<2)) lamp(led_max);		//Если кнопку еще не отпустили, зажигаем полностью
			}
		}
		while(~PINB & (1<<2)) {};
	}

	TCCR0 = (1<<CS02)|(0<<CS01)|(1<<CS00);
	//TIMSK |= (1<<TOIE0);
}

static void lamp(uint8_t pwm) {
	//OCR1A = pgm_read_word(&(led_exp[pwm]));
	/*int16_t i = 0;
	while((pgm_read_word(&(led_exp[i])) < OCR1A) && (i < 255)) i++;

	if(i > pwm) {		//Если фонарь горит, быстро гасим его
		//while((OCR1A > pwm) && (i >= 0)) {
		while(--i >= pwm) {
			OCR1A = pgm_read_word(&(led_exp[i]));
			_delay_ms(2);
		}
	}
	else {				//Если фонарь не горит, плавно включаем его
		//while((OCR1A < pwm) & (i <= 255)) {
		while(++i <= pwm) {
			OCR1A = pgm_read_word(&(led_exp[i]));
			_delay_ms(10);
		}
	}*/
	
	int16_t temp = 0;		
	int16_t i = 0;
	while(((temp + 7) < OCR1A) && (i < 255)) {
		temp = i * i;
		temp = temp >> 6;
		i++;
	}

	if(i > pwm) {		//Если фонарь горит, быстро гасим его
		while(--i >= pwm) {
			temp = i * i;
			temp = temp >> 6;
			OCR1A = temp + 7;
			_delay_ms(2);
		}
	}
	else {				//Если фонарь не горит, плавно включаем его
		while(++i <= pwm) {
			temp = i * i;
			temp = temp >> 6;
			OCR1A = temp + 7;
			_delay_ms(10);
		}
	}
	if(pwm == 0) OCR1A = 0;
}

static void menu(void) {
	#ifdef BUZ_EN
		//beep(659, 25);
		//beep(880, 25);
		buzz(25);
	#endif
	OLED_displayOn();
	OLED_fill(0x00);
	OLED_printString(37, 0, 1, "Настройки");
	#ifndef OLED_ROTATE_180
	OLED_printString(0, 48, 1, ">                  +");
	#else
	OLED_printString(0, 48, 1, "+                  >");
	#endif
	
	if(menu_item("Будильник (час)", (char *) &alarm_hour, 0, 23) != 0) return;
	if(menu_item("Будильник (мин.)", (char *) &alarm_minutes, 0, 59) != 0) return;
	OLED_printString(0, 24, 1, "0-выкл 1-рассв.");
	OLED_printString(0, 32, 1, "2-звук 3-зв+св");
	if(menu_item("Режим будильника", (char *) &alarm_signal, 0, 3) != 0) return;
	OLED_printString(0, 24, 1, "               ");
	OLED_printString(0, 32, 1, "               ");
	if(menu_item("Длит. рассвета (мин)", (char *) &dawn_dur, 10, 255) != 0) return;
	DS3231_getTime(&rtc_hours, &rtc_minutes);
	if(menu_item("Время (час)         ", (char *) &rtc_hours, 0, 23) != 0) return;
	if(menu_item("Время (мин.)        ", (char *) &rtc_minutes, 0, 59) != 0) return;
	if(menu_item("День недели         ", (char *) &rtc_day, 1, 7) != 0) return;
	if(menu_item("Яркость ночника     ", (char *) &led_half, 30, 255) != 0) return;
}

static char menu_item(char * name, char * value, unsigned char min, unsigned char max) {
	uint8_t btn_counter = 0;

	OLED_printString(0, 16, 1, name);
	sprintf(str, "%3u", *value);
	OLED_printString(90, 24, 2, str);

	while((~PINB & (1<<2)) || (~PINB & (1<<3))) {};

	while(1) {
		while((PINB & (1<<2)) && (PINB & (1<<3)) && (btn_counter++ < 150)) _delay_ms(50);		//Ожидание
		if(btn_counter >= 150) return -1;	//Слишком долго ждём - выход
		_delay_ms(25);
		if(~PINB & (1<<2)) return 0;		//Кнопка 1 - следующий пункт
		if(~PINB & (1<<3)) {				//Кнопка 2 - прибавление параметра
			if(++(*value) > max) (*value) = min;
			sprintf(str, "%3u", *value);
			OLED_printString(90, 24, 2, str);
			btn_counter = 0;
			while((~PINB & (1<<3)) && (btn_counter++ < 30)) _delay_ms(5);
		};
	}
}

/*static void delay_us(uint16_t time) {
	while(time--) _delay_us(1);
}

static void beep(uint16_t freq, uint16_t dur_ms) {
	uint16_t per_us = 500000 / freq;
	uint32_t dur_per = dur_ms*freq;
	dur_per /= 1000;
	
	while(dur_per--) {
		PORTD |= (1<<4);
		delay_us(per_us);
		PORTD &=~ (1<<4);
		delay_us(per_us);
	}
}*/

static void buzz(uint16_t dur_ms) {
	PORTD |= (1<<4);
	while(dur_ms--) _delay_ms(1);
	PORTD &=~ (1<<4);
}

static void saveSettings(void) {
	DS3231_setTime(rtc_hours, rtc_minutes);
	DS3231_setDay(rtc_day);
	
	eeprom_save_byte((uint8_t*) EEPROM_ALARM_HOUR, alarm_hour);
	eeprom_save_byte((uint8_t*) EEPROM_ALARM_MIN, alarm_minutes);
	eeprom_save_byte((uint8_t*) EEPROM_ALARM_SIG, alarm_signal);
	eeprom_save_byte((uint8_t*) EEPROM_ALARM_DAWN, dawn_dur);
	eeprom_save_byte((uint8_t*) EEPROM_LED_HALF, led_half);
}

static void eeprom_save_byte(uint8_t* addr, uint8_t val) {
	uint8_t temp = eeprom_read_byte(addr);
	if(temp != val) eeprom_write_byte(addr, val);
}