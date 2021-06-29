/*
 * DS3231.c
 *
 * Created: 13.06.2021 16:17:05
 *  Author: Dubishkin
 */ 


#include "settings.h"

#include <avr/io.h>
#include "I2C.h"
#include "DS3231.h"

unsigned char DS3231_buf[8] = {0};

void DS3231_init(void) {
	DS3231_buf[0] = (0<<DS3231_CONTROL_EOSC)|(0<<DS3231_CONTROL_BBSQW)|(0<<DS3231_CONTROL_CONV)|(0<<DS3231_CONTROL_RS2)|(0<<DS3231_CONTROL_RS1)|(0<<DS3231_CONTROL_INTCN)|(0<<DS3231_CONTROL_A2IE)|(0<<DS3231_CONTROL_A1IE);
	DS3231_buf[1] = (0<<DS3231_CONTROL_OSF)|(0<<DS3231_CONTROL_EN32kHz)|(0<<DS3231_STATUS_BSY)|(0<<DS3231_STATUS_A2F)|(0<<DS3231_STATUS_A1F);
	I2C_write(DS3231_ADDR, DS3231_CONTROL, DS3231_buf, 2);
}

void DS3231_getTime(unsigned char * hours, unsigned char * minutes) {
	I2C_read(DS3231_ADDR, DS3231_MINUTES, DS3231_buf, 2);

	*minutes = ((DS3231_buf[0] >> 4) * 10) + (DS3231_buf[0] & 0b00001111);
	*hours = ((DS3231_buf[1] >> 4) * 10) + (DS3231_buf[1] & 0b00001111);
}

void DS3231_setTime(unsigned char hours, unsigned char minutes) {
	hours %= 24; minutes %= 60;

	DS3231_buf[0] = (minutes / 10 * 16) + (minutes % 10);
	DS3231_buf[1] = (hours / 10 * 16) + (hours % 10);
	I2C_write(DS3231_ADDR, DS3231_MINUTES, DS3231_buf, 2);
}

void DS3231_getDay(unsigned char * day) {
	I2C_read(DS3231_ADDR, DS3231_DAY, DS3231_buf, 1);

	*day = DS3231_buf[0] & 0b00000111;
}

void DS3231_setDay(unsigned char day) {
	if((day < 1) || (day > 12)) return;
	DS3231_buf[0] = day;
	I2C_write(DS3231_ADDR, DS3231_DAY, DS3231_buf, 1);
}

void DS3231_getDate(unsigned char * date, unsigned char * month, unsigned char * year) {
	I2C_read(DS3231_ADDR, DS3231_DATE, DS3231_buf, 3);

	*date = ((DS3231_buf[0] >> 4) * 10) + (DS3231_buf[0] & 0b00001111);
	*month = (((DS3231_buf[1] & 0b00010000) >> 4) * 10) + (DS3231_buf[1] & 0b00001111);
	*year = ((DS3231_buf[2] >> 4) * 10) + (DS3231_buf[2] & 0b00001111);
}

void DS3231_setDate(unsigned char date, unsigned char month, unsigned char year) {
	year %= 100; 
	if((month < 1) || (month > 12)) return;
	if((date < 1) || (date > 31)) return;

	DS3231_buf[0] = (date / 10 * 16) + (date % 10);
	DS3231_buf[1] = (month / 10 * 16) + (month % 10);
	DS3231_buf[2] = (year / 10 * 16) + (year % 10);
	I2C_write(DS3231_ADDR, DS3231_DATE, DS3231_buf, 3);
}