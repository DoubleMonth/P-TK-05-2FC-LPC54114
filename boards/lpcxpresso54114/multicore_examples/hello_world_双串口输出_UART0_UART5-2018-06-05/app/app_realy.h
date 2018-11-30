#ifndef _REALY_H
#define _REALY_H
#include <stdio.h>
#include <stdint.h>
//#define REALY_CTRL1_PORT                1 // GPIOB                                  //µÍ
//#define REALY_CTRL1_PIN                 1 // GPIO_Pin_15//15GPIO_Pin_15
//#define REALY_CTRL1_PINSource           1 // GPIO_PinSource1//GPIO_PinSource15
//#define REALY_CTRL1_RCC							1 //		 RCC_AHBPeriph_GPIOB

//#define REALY_CTRL2_PORT                 1 //GPIOB                                  //ÖÐ
//#define REALY_CTRL2_PIN                 1 // GPIO_Pin_1//GPIO_Pin_14
//#define REALY_CTRL2_PINSource           1 // GPIO_PinSource12//GPIO_PinSource14
//#define REALY_CTRL2_RCC						1 //			 RCC_AHBPeriph_GPIOB

//#define REALY_CTRL3_PORT                1 // GPIOB                                   //¸ß
//#define REALY_CTRL3_PIN                 1// GPIO_Pin_12//GPIO_Pin_12
//#define REALY_CTRL3_PINSource           1// GPIO_PinSource14//GPIO_PinSource12
//#define REALY_CTRL3_RCC						1 //			 RCC_AHBPeriph_GPIOB

//#define REALY_CTRL4_PORT                1// GPIOB                                   //·§   
//#define REALY_CTRL4_PIN                  1//GPIO_Pin_14//GPIO_Pin_1
//#define REALY_CTRL4_PINSource            1 //GPIO_PinSource15//GPIO_PinSource1
//#define REALY_CTRL4_RCC					1 //				 RCC_AHBPeriph_GPIOB

enum 
{
	REALY1,
	REALY2,
	REALY3,
	REALY4,
};
enum 
{
	REALY_ON,
	REALY_OFF,
	REALY_TOGGLE,
};
uint8_t realy_init(void);
void relay_ctrl(uint8_t realy,uint8_t ctrl);
void realy_on(uint8_t num);
void realy_off(uint8_t num);
void realy_toggle(uint8_t num);
#endif
