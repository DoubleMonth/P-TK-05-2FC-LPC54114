#include "board.h"

#include "pin_mux.h"

#include "fsl_common.h"
#include "fsl_iocon.h"

#include <stdbool.h>
#include "app_realy.h"
#include "app_lcd.h"
#define REALY_NUM 4
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define REALY_GPIO_CFG IOCON_MODE_PULLUP | IOCON_FUNC0 | IOCON_GPIO_MODE | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
const uint8_t  REALY_GPIO_PORT[REALY_NUM] = { 0,  0,  0,  0, };
const uint8_t  REALY_GPIO_PIN [REALY_NUM] = {22, 21, 19, 15, };
const uint8_t  REALY_GPIO_ON  [REALY_NUM] = { 1,  1,  1,  1, };
const uint8_t  REALY_GPIO_OFF [REALY_NUM] = { 0,  0,  0,  0, };

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
uint8_t realy_init(void)
{	
	uint8_t ret = 0;
	uint8_t i = 0;
	
// Init output LED GPIO
	for(i=0; i<REALY_NUM; i++)
	{
		GPIO->DIR[REALY_GPIO_PORT[i]] |= 1U << REALY_GPIO_PIN[i];
		GPIO->B[REALY_GPIO_PORT[i]][REALY_GPIO_PIN[i]] = REALY_GPIO_OFF[i];
		IOCON_PinMuxSet(IOCON, REALY_GPIO_PORT[i], REALY_GPIO_PIN[i], REALY_GPIO_CFG);
		realy_off(i);
	}
	
	ret = 1;
	
	return ret;
}

void realy_on(uint8_t num)
{
	GPIO->B[REALY_GPIO_PORT[num]][REALY_GPIO_PIN[num]] = REALY_GPIO_ON[num];
}

void realy_off(uint8_t num)
{
	GPIO->B[REALY_GPIO_PORT[num]][REALY_GPIO_PIN[num]] = REALY_GPIO_OFF[num];
}

void realy_toggle(uint8_t num)
{
	GPIO->NOT[REALY_GPIO_PORT[num]] |= (1 << REALY_GPIO_PIN[num]);
}

/*
继电器控制函数
uint8_t realy  继电器编号  枚举值：REALY1,REALY2,REALY3,REALY4
uint8_t ctrl  开、关、翻转  枚举值：REALY_ON，REALY_OFF,REALY_TOGGLE
*/
void relay_ctrl(uint8_t realy,uint8_t ctrl)
{
	switch(realy)
	{
		case REALY1: 
		{
			switch (ctrl)
			{
				case REALY_ON: 
				{
//					GPIO_SetBits(REALY_CTRL1_PORT,REALY_CTRL1_PIN);
					realy_on(0);
				}break;
				case REALY_OFF: 
				{
//					GPIO_ResetBits(REALY_CTRL1_PORT,REALY_CTRL1_PIN);
					realy_off(0);
				}break;
				case REALY_TOGGLE: 
				{
//					toggle_pin(REALY_CTRL1_PORT,REALY_CTRL1_PIN);
					realy_toggle(0);
				}break;
				default : ;
			}
		}break;
		case REALY2: 
		{
			switch (ctrl)
			{
				case REALY_ON: 
				{
//					GPIO_SetBits(REALY_CTRL2_PORT,REALY_CTRL2_PIN);
					realy_on(1);
				}break;
				case REALY_OFF: 
				{
//					GPIO_ResetBits(REALY_CTRL2_PORT,REALY_CTRL2_PIN);
					realy_off(1);
				}break;
				case REALY_TOGGLE: 
				{
//					toggle_pin(REALY_CTRL2_PORT,REALY_CTRL2_PIN);
					realy_toggle(1);
				}break;
				default : ;
			}
		}break;
		case REALY3: 
		{
			switch (ctrl)
			{
				case REALY_ON: 
				{
//					GPIO_SetBits(REALY_CTRL3_PORT,REALY_CTRL3_PIN);
					realy_on(2);
				}break;
				case REALY_OFF: 
				{
//					GPIO_ResetBits(REALY_CTRL3_PORT,REALY_CTRL3_PIN);
					realy_off(2);
				}break;
				case REALY_TOGGLE: 
				{
//					toggle_pin(REALY_CTRL3_PORT,REALY_CTRL3_PIN);
					realy_toggle(2);
				}break;
				default : ;
			}
		}break;
		case REALY4: 
		{
			switch (ctrl)
			{
				case REALY_ON: 
				{
//					GPIO_SetBits(REALY_CTRL4_PORT,REALY_CTRL4_PIN);
					realy_on(3);
					display_signal(VALVE);//显示阀门图标
				}break;
				case REALY_OFF: 
				{
//					GPIO_ResetBits(REALY_CTRL4_PORT,REALY_CTRL4_PIN);
					realy_off(3);
					clr_display_signal(VALVE);//显示阀门图标
				}break;
				case REALY_TOGGLE: 
				{
//					toggle_pin(REALY_CTRL4_PORT,REALY_CTRL4_PIN);
					realy_toggle(3);
				}break;
				default : ;
			}
		}break;
		default : ;
	}
}
