#ifndef __HAL_SI7XX_H_
#define __HAL_SI7XX_H_
#include <stdio.h>
#include <stdint.h>
enum {ACK = 0, NACK};
//typedef enum 
//{
//  ERROR = 0, 
//  SUCCESS = !ERROR
//} ErrorStatus;
struct SI7020_DATA
{
	double temperature;
	double humidity;
};
struct sensor
{
	int16_t temp;
    int16_t humi;
};
#define SI7020_WRITE_ADDR   0x80
#define SI7020_READ_ADDR    0x81

#define SI7020_TEMPERATURE  0xE3
#define SI7020_HUMIDITY     0xE5
#define SI7020_THERMISTOR   0xEE
#define SI7020_SOFT_RESET   0xFE

void si70xxInit(void);
uint8_t si7020Measure(double *temperature, double *humidity);
#endif
