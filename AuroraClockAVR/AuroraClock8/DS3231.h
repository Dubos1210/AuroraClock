/*
 * DS3231.h
 *
 * Created: 13.06.2021 16:17:18
 *  Author: Dubishkin
 */ 


#ifndef DS3231_H_
#define DS3231_H_

#define DS3231_ADDR	0xD0

#define DS3231_SECONDS			0x00
#define DS3231_MINUTES			0x01
#define DS3231_HOURS			0x02
#define DS3231_DAY				0x03
#define DS3231_DATE				0x04
#define DS3231_MONTH			0x05
#define DS3231_YEAR				0x06
#define DS3231_ALARM1_SECONDS	0x07
#define DS3231_ALARM1_MINUTES	0x08
#define DS3231_ALARM1_HOURS		0x09
#define DS3231_ALARM1_DAY_DATE	0x0A
#define DS3231_ALARM2_MINUTES	0x0B
#define DS3231_ALARM2_HOURS		0x0C
#define DS3231_ALARM2_DAY_DATE	0x0D
#define DS3231_CONTROL			0x0E
	#define DS3231_CONTROL_EOSC		7
	#define DS3231_CONTROL_BBSQW	6
	#define DS3231_CONTROL_CONV		5
	#define DS3231_CONTROL_RS2		4
	#define DS3231_CONTROL_RS1		3
	#define DS3231_CONTROL_INTCN	2
	#define DS3231_CONTROL_A2IE		1
	#define DS3231_CONTROL_A1IE		0
#define DS3231_CONTROL_STATUS	0x0F
	#define DS3231_CONTROL_OSF		7
	#define DS3231_CONTROL_EN32kHz	3
	#define DS3231_STATUS_BSY		2
	#define DS3231_STATUS_A2F		1
	#define DS3231_STATUS_A1F		0
#define DS3231_AGING_OFFSET		0x10
#define DS3231_TEMP_MSB			0x11
#define DS3231_TEMP_LSB			0x12

void DS3231_init(void);
void DS3231_getTime(unsigned char * hours, unsigned char * minutes);
void DS3231_setTime(unsigned char hours, unsigned char minutes);
void DS3231_getDay(unsigned char * day);	//День недели в диапазоне 1-7
void DS3231_setDay(unsigned char day);
void DS3231_getDate(unsigned char * date, unsigned char * month, unsigned char * year);
void DS3231_setDate(unsigned char date, unsigned char month, unsigned char year);

#endif /* DS3231_H_ */