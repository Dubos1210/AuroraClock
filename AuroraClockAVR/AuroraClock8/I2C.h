/*
 * I2C.h
 *
 * Created: 28.12.2018 14:44:47
 *  Author: Vladimir
 */ 


#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

#define SDA_in() DDRC &=~ (1<<4) //SDA - вход (высокий уровень на шине благодаря внешней подтяжке)
#define SDA_out() DDRC |= (1<<4) //SDA - выход
#define SDA_pin() PINC & (1<<4)  //Проверка состояния SDA

#define SCL_in() DDRC &=~ (1<<5) //SCL - вход (высокий уровень на шине благодаря внешней подтяжке)
#define SCL_out() DDRC |= (1<<5) //SCL - выход

#define I2C_DELAY 2

void I2C_read(uint8_t addr, uint8_t reg, uint8_t * data, uint8_t count);
void I2C_write(uint8_t addr, uint8_t reg, uint8_t * data, uint8_t count);

#endif /* I2C_H_ */