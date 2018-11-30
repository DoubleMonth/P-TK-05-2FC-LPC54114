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

#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "board.h"
#include "mcmgr.h"

#include "fsl_common.h"
#include "pin_mux.h"
#include "fsl_usart.h"

/* USER CODE BEGIN Includes */
#include "fsl_sctimer.h"
#include "app_si70xx.h"
#include "app_dev_ctrl.h"
#include "app_lcd.h"
#include "app_key.h"
#include "app_realy.h"
#include "app_spiflash.h"
#include "app_beep.h"
#include "smart_plc.h"
#include "fsl_usart_cmsis.h"
#include "alloter.h"
#include "fsl_mailbox.h"
/* USER CODE END Includes */
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Address of RAM, where the image for core1 should be copied */
#define CORE1_BOOT_ADDRESS (void *)0x20010000

#if defined(__CC_ARM)
extern uint32_t Image$$CORE1_REGION$$Base;
extern uint32_t Image$$CORE1_REGION$$Length;
#define CORE1_IMAGE_START &Image$$CORE1_REGION$$Base
#elif defined(__ICCARM__)
extern unsigned char core1_image_start[];
#define CORE1_IMAGE_START core1_image_start
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

#ifdef CORE1_IMAGE_COPY_TO_RAM
uint32_t get_core1_image_size(void);
#endif


/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/


/* USER CODE END PV */


void delay(void);

/*******************************************************************************
 * Code
 ******************************************************************************/

#ifdef CORE1_IMAGE_COPY_TO_RAM
uint32_t get_core1_image_size()
{
    uint32_t core1_image_size;
#if defined(__CC_ARM)
    core1_image_size = (uint32_t)&Image$$CORE1_REGION$$Length;
#elif defined(__ICCARM__)
#pragma section = "__sec_core"
    core1_image_size = (uint32_t)__section_end("__sec_core") - (uint32_t)&core1_image_start;
#endif
    return core1_image_size;
}
#endif

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */
extern struct SI7020_DATA si7020_data;
/*******************************************************************************
 * Code
 ******************************************************************************/
extern struct sensor sensor;

struct mail_message
{
	uint8_t num;						//´«µÝµÄ×Ö·ûÊý
	uint8_t read_falg;
	double temperature;         //´«µÝ×Ö·û´®Ö¸Õë
	double humidity;
	uint8_t sensor_error;
}s_message;
struct mail_message *p_message;
volatile uint32_t g_msg;
volatile uint32_t *message;
extern struct FLASH_STRUCT flash_struct;
uint8_t sensor_error_flag;
void read_si7020_data(void)
{
	s_message.read_falg=1;
	s_message.humidity=0;
	s_message.temperature=0;
	p_message = &s_message;
	g_msg = (uint32_t)p_message;
	if(sensor_error_flag==1)
	{
		flash_struct.breakdown_start_flag=true;
	}
	else
		flash_struct.breakdown_start_flag=false;
	PRINTF("breakdown_start_flag=%d\n", flash_struct.breakdown_start_flag);
	/* Wr ite g_msg to CM0+ mailbox register - it causes interrupt on CM0+ core */
	MAILBOX_SetValue(MAILBOX, kMAILBOX_CM0Plus, g_msg);
}
void MAILBOX_IRQHandler()
{
    g_msg = MAILBOX_GetValue(MAILBOX, kMAILBOX_CM4);
	p_message =(struct mail_message *)g_msg;
	si7020_data.temperature=p_message->temperature;
	si7020_data.humidity=p_message->humidity;
	sensor_error_flag=p_message->sensor_error;
	sensor.temp=si7020_data.temperature;
	sensor.humi=si7020_data.humidity;
    MAILBOX_ClearValueBits(MAILBOX, kMAILBOX_CM4, 0xffffffff);
}

/*!
 * @brief Main function
 */
int main(void)
{
    /* Define the init structuDre for the switches*/
    gpio_pin_config_t sw_config = {kGPIO_DigitalInput, 0};
	

    /* Init board hardware.*/
    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);

    BOARD_InitPins_Core0();
	CLOCK_EnableClock(kCLOCK_InputMux);
	CLOCK_EnableClock(kCLOCK_Iocon);
	CLOCK_EnableClock(kCLOCK_Gpio0);
	CLOCK_EnableClock(kCLOCK_Gpio1);
    BOARD_BootClockFROHF48M();
    BOARD_InitDebugConsole();
	SysTick_Config(SystemCoreClock/1000);
#ifdef CORE1_IMAGE_COPY_TO_RAM
    /* Calculate size of the image  - not required on LPCExpresso. LPCExpresso copies image to RAM during startup
     * automatically */
    uint32_t core1_image_size;
    core1_image_size = get_core1_image_size();
    PRINTF("Copy Secondary core image to address: 0x%x, size: %d\n", CORE1_BOOT_ADDRESS, core1_image_size);

    /* Copy Secondary core application from FLASH to RAM. Primary core code is executed from FLASH, Secondary from RAM
     * for maximal effectivity.*/
    memcpy(CORE1_BOOT_ADDRESS, (void *)CORE1_IMAGE_START, core1_image_size);
#endif

    /* Initialize MCMGR before calling its API */
    MCMGR_Init();

    /* Boot Secondary core application */
    PRINTF("Starting Secondary core.\n");
    MCMGR_StartCore(kMCMGR_Core1, CORE1_BOOT_ADDRESS, 1, kMCMGR_Start_Synchronous);

    /* Print the initial banner from Primary core */
    PRINTF("\r\nLPC54114 Multicore Core Thermostat\r\n\n");
	PRINTF("\r\nDesigned by haifeng-388081\r\n\n");
	
	/* USER CODE BEGIN Init */
	
	
	spiflash_init();
	bu9796Init();
	key_init();
	realy_init();
	beep_init();
	backlight_init();
	app_init();    
	plc_init();
	init_chn_pool_mgr();
	init_uart_infor();
	dev_init();
	system_init();
	MAILBOX_Init(MAILBOX);
	NVIC_EnableIRQ(MAILBOX_IRQn);
	MCMGR_StartCore(kMCMGR_Core1, CORE1_BOOT_ADDRESS, 5, kMCMGR_Start_Synchronous);
    while (1)
    {
		task_handle();
    }
}

void delay(void)
{
    for (uint32_t i = 0; i < 0x7fffffU; i++)
    {
        __NOP();
    }
}
