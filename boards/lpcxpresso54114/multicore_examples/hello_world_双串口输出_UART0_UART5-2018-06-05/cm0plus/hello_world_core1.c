/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "board.h"
#include "mcmgr.h"

#include "fsl_common.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_usart.h"
#include "app_si70xx.h"
#include "fsl_mailbox.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LED_INIT() GPIO_PinInit(GPIO, BOARD_LED_RED_GPIO_PORT, BOARD_LED_RED_GPIO_PIN, &led_config);
#define LED_TOGGLE() GPIO_TogglePinsOutput(GPIO, BOARD_LED_RED_GPIO_PORT, 1u << BOARD_LED_RED_GPIO_PIN);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Function to create delay for Led blink.
 */
void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 1000000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}
struct mail_message
{
	uint8_t num;						//传递的字符数
	uint8_t read_falg;
	double temperature;         //传递字符串指针
	double humidity;
	uint8_t sensor_error;
}s_message;
struct mail_message *p_message;
volatile uint32_t g_msg;
volatile uint32_t *message;
uint8_t receive_flag = 0;			//收到M4+数据标志位
void MAILBOX_IRQHandler()
{
    g_msg = MAILBOX_GetValue(MAILBOX, kMAILBOX_CM0Plus);
		
		p_message = (struct mail_message *)g_msg;   //将g_msg转换为指针
	if(p_message->read_falg==1)
		receive_flag = 1;														//收到M0+数据标志位
    MAILBOX_ClearValueBits(MAILBOX, kMAILBOX_CM0Plus, 0xffffffff);
		
}

/*!
 * @brief Main function
 */
int main(void)
{
    usart_config_t config;
    usart_transfer_t xfer;
    usart_transfer_t sendXfer;
    usart_transfer_t receiveXfer;
	status_t reVal = kStatus_Fail;
	
	uint32_t startupData, i;

    /* Define the init structure for the output LED pin*/
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput, 0,
    };
    /* Initialize MCMGR before calling its API */
    MCMGR_Init();

    /* Get the startup data */
    MCMGR_GetStartupData(kMCMGR_Core1, &startupData);

    /* Make a noticable delay after the reset */
    /* Use startup parameter from the master core... */
    for (i = 0; i < startupData; i++)
        delay();

    /* Init board hardware.*/
    /* enable clock for GPIO */
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_Gpio1);
	
	
    /* Signal the other core we are ready */
    MCMGR_SignalReady(kMCMGR_Core1);
	MAILBOX_Init(MAILBOX);
	NVIC_EnableIRQ(MAILBOX_IRQn);

	si70xxInit();
	
    while (1)
    {
		if(receive_flag == 1)
		{
			reVal = si7020Measure(&s_message.temperature,&s_message.humidity);
			if(reVal==kStatus_Success)
			{
				s_message.sensor_error=1;
			}
			else
			{
				s_message.sensor_error=0;
			}
			s_message.read_falg=0;
			p_message = &s_message;
			g_msg = (uint32_t)p_message;
			MAILBOX_SetValue(MAILBOX, kMAILBOX_CM4, g_msg);
			receive_flag=0;
		}
    }
}
