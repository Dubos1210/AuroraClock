/*
 * I2C.c
 *
 * Created: 28.12.2018 14:44:30
 *  Author: Vladimir
 */ 
 
#include "settings.h"

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "I2C.h"

uint8_t I2C_counter1, I2C_counter2;

/***************************************
 * Функция	 : отправка сигнала "СТАРТ"
 ***************************************/
static void inline I2C_start(void) {
	PORTC &=~ (1<<4);              //Низкий уровень на SDA, если выход
	PORTC &=~ (1<<5);              //Низкий уровень на SCL, если выход
	SDA_in();
	_delay_us(I2C_DELAY);
	SCL_in();
	_delay_us(I2C_DELAY);
	SDA_out();
	_delay_us(I2C_DELAY);
	SCL_out();
	return;
}

/***************************************
 * Функция	 : отправка байта
 * Аргументы : байт для отправки
 ***************************************/
static void inline I2C_transmit(uint8_t msg) {
	for(I2C_counter1 = 8; I2C_counter1 > 0; I2C_counter1--) {
		if(msg & (1<<(I2C_counter1 - 1))) {			
			SDA_in();
		}
		else {			
			SDA_out();
		}
		_delay_us(I2C_DELAY);
		SCL_in();
		_delay_us(I2C_DELAY);
		_delay_us(I2C_DELAY);
		SCL_out();
		_delay_us(I2C_DELAY);
	}	
	SCL_in();
	SDA_in();
	_delay_us(I2C_DELAY);
	I2C_counter1 = 0;
	while(SDA_pin()) {
		if(I2C_counter1 < 200) {
			_delay_us(1);
			//I2C_counter1++;
		}
		else {
			return;
		}
	};
	_delay_us(I2C_DELAY);
	SCL_out();
	_delay_us(I2C_DELAY);
	SDA_out();
	_delay_us(I2C_DELAY);
	return;
}

/***************************************
 * Функция	 : прием байта
 * Ответ	 : полученный байт
 ***************************************/
static uint8_t inline I2C_receive(void) {
	uint8_t msg = 0;
	SDA_in();
	for(I2C_counter1 = 0; I2C_counter1 < 7; I2C_counter1++) {
		SCL_in();
		_delay_us(I2C_DELAY);
		if(SDA_pin()) {
			msg = msg + 1;
		}
		_delay_us(I2C_DELAY);
		SCL_out();
		_delay_us(I2C_DELAY);
		_delay_us(I2C_DELAY);
		msg = msg << 1;
	}
	SCL_in();
	_delay_us(I2C_DELAY);
	if(SDA_pin()) {
		msg = msg + 1;
	}
	_delay_us(I2C_DELAY);
	SCL_out();
	_delay_us(I2C_DELAY);
	_delay_us(I2C_DELAY);
	return msg;
}

/***************************************
 * Функция	 : отправка сигнала "ACK"
 ***************************************/
static void inline I2C_send_ack(void) {
	SDA_out();
	SCL_in();
	_delay_us(I2C_DELAY);
	SCL_out();
	SDA_in();
	return;
}

/***************************************
 * Функция	 : отправка сигнала "NACK"
 ***************************************/
static void inline I2C_send_nack(void) {
	SDA_in();
	SCL_in();
	_delay_us(I2C_DELAY);
	SCL_out();
	return;
}

/***************************************
 * Функция	 : отправка сигнала "СТОП"
 ***************************************/
static void inline I2C_stop(void) {
	SDA_out();
	_delay_us(I2C_DELAY);
	SCL_in();
	_delay_us(I2C_DELAY);
	SDA_in();
	return;
}

void I2C_read(uint8_t addr, uint8_t reg, uint8_t * data, uint8_t count) {
	I2C_start();
	I2C_transmit(addr);
	I2C_transmit(reg);
	I2C_start();
	I2C_transmit(addr + 1);
	for(I2C_counter2 = 0; I2C_counter2 < (count - 1); I2C_counter2++) {
		((uint8_t*) data)[I2C_counter2] = I2C_receive();
		I2C_send_ack();
	}
	((uint8_t*) data)[count-1] = I2C_receive();
	I2C_send_nack();
	I2C_stop();
}

void I2C_write(uint8_t addr, uint8_t reg, uint8_t * data, uint8_t count) {
	I2C_start();
	I2C_transmit(addr);
	I2C_transmit(reg);
	for(I2C_counter2 = 0; I2C_counter2 < count; I2C_counter2++) {
		I2C_transmit(((uint8_t*) data)[I2C_counter2]);
	}
	I2C_stop();
}

