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

/*
 * TEXT BELOW IS USED AS SETTING FOR THE PINS TOOL *****************************
PinsProfile:
- !!product 'Pins v2.0'
- !!processor 'LPC54114J256'
- !!package 'LPC54114J256BD64'
- !!mcu_data 'ksdk2_0'
- !!processor_version '1.1.0'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR THE PINS TOOL ***
 */

#include "fsl_common.h"
#include "fsl_iocon.h"
#include "pin_mux.h"

/*
 * TEXT BELOW IS USED AS SETTING FOR THE PINS TOOL *****************************
BOARD_InitPins:
- options: {coreID: cm4, enableClock: 'true'}
- pin_list: []
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR THE PINS TOOL ***
 */

/*FUNCTION**********************************************************************
 *
 * Function Name : BOARD_InitPins
 * Description   : Configures pin routing and optionally pin electrical features.
 *
 *END**************************************************************************/
void BOARD_InitPins(void)
{ /* Function assigned for the Core #0 (ARM Cortex-M4) */
}

#define IOCON_PIO_DIGITAL_EN 0x80u    /*!< Enables digital function */
#define IOCON_PIO_FUNC1 0x01u         /*!< Selects pin function 1 */
#define IOCON_PIO_INPFILT_OFF 0x0100u /*!< Input filter disabled */
#define IOCON_PIO_INV_DI 0x00u        /*!< Input function is not inverted */
#define IOCON_PIO_MODE_INACT 0x00u    /*!< No addition pin function */
#define IOCON_PIO_OPENDRAIN_DI 0x00u  /*!< Open drain is disabled */
#define IOCON_PIO_SLEW_STANDARD 0x00u /*!< Standard mode, output slew rate control is enabled */
#define PIN0_IDX 0u                   /*!< Pin number for pin 0 in a port 0 */
#define PIN1_IDX 1u                   /*!< Pin number for pin 1 in a port 0 */
#define PORT0_IDX 0u                  /*!< Port index */

#define PIN18_IDX                         18u   /*!< Pin number for pin 0 in a port 0 */
#define PIN20_IDX                         20u   /*!< Pin number for pin 1 in a port 0 */


#define PIO00_DIGIMODE_DIGITAL        0x01u   /*!< Select Analog/Digital mode.: Digital mode. */
#define PIO00_FUNC_ALT0               0x00u   /*!< Selects pin function.: Alternative connection 0. */
#define PIO01_DIGIMODE_DIGITAL        0x01u   /*!< Select Analog/Digital mode.: Digital mode. */
#define PIO01_FUNC_ALT0               0x00u   /*!< Selects pin function.: Alternative connection 0. */

/* TEXT BELOW IS USED AS SETTING FOR THE PINS TOOL ***************************** */
/*
BOARD_InitPins_Core0:
- options: {coreID: cm4, enableClock: 'true'}
- pin_list:
  - {pin_num: '31', peripheral: FLEXCOMM0, signal: RXD_SDA_MOSI, pin_signal:
PIO0_0/FC0_RXD_SDA_MOSI/FC3_CTS_SDA_SSEL0/CTIMER0_CAP0/SCT0_OUT3, mode: inactive, invert: disabled,
    glitch_filter: disabled, slew_rate: standard, open_drain: disabled}
  - {pin_num: '32', peripheral: FLEXCOMM0, signal: TXD_SCL_MISO, pin_signal:
PIO0_1/FC0_TXD_SCL_MISO/FC3_RTS_SCL_SSEL1/CTIMER0_CAP1/SCT0_OUT1, mode: inactive, invert: disabled,
    glitch_filter: disabled, slew_rate: standard, open_drain: disabled}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR THE PINS TOOL ***
 */

/*FUNCTION**********************************************************************
 *
 * Function Name : BOARD_InitPins_Core0
 *
 *END**************************************************************************/
void BOARD_InitPins_Core0(void)
{                                    /* Function assigned for the Core #0 (ARM Cortex-M4) */
    CLOCK_EnableClock(kCLOCK_Iocon); /* Enables the clock for the IOCON block. 0 = Disable; 1 = Enable.: 0x01u */

    const uint32_t port0_pin0_config =
        (IOCON_PIO_FUNC1 |         /* Pin is configured as FC0_RXD_SDA_MOSI */
         IOCON_PIO_MODE_INACT |    /* No addition pin function */
         IOCON_PIO_INV_DI |        /* Input function is not inverted */
         IOCON_PIO_DIGITAL_EN |    /* Enables digital function */
         IOCON_PIO_INPFILT_OFF |   /* Input filter disabled */
         IOCON_PIO_SLEW_STANDARD | /* Standard mode, output slew rate control is enabled */
         IOCON_PIO_OPENDRAIN_DI    /* Open drain is disabled */
         );
    IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN0_IDX,
                    port0_pin0_config); /* PORT0 PIN0 (coords: 31) is configured as FC0_RXD_SDA_MOSI */
    const uint32_t port0_pin1_config =
        (IOCON_PIO_FUNC1 |         /* Pin is configured as FC0_TXD_SCL_MISO */
         IOCON_PIO_MODE_INACT |    /* No addition pin function */
         IOCON_PIO_INV_DI |        /* Input function is not inverted */
         IOCON_PIO_DIGITAL_EN |    /* Enables digital function */
         IOCON_PIO_INPFILT_OFF |   /* Input filter disabled */
         IOCON_PIO_SLEW_STANDARD | /* Standard mode, output slew rate control is enabled */
         IOCON_PIO_OPENDRAIN_DI    /* Open drain is disabled */
         );
    IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN1_IDX,
                    port0_pin1_config); /* PORT0 PIN1 (coords: 32) is configured as FC0_TXD_SCL_MISO */
}

void BOARD_InitPins_Core1(void)
{                                    /* Function assigned for the Core #0 (ARM Cortex-M4) */
    CLOCK_EnableClock(kCLOCK_Iocon); /* Enables the clock for the IOCON block. 0 = Disable; 1 = Enable.: 0x01u */

    const uint32_t port0_pin0_config =
        (IOCON_PIO_FUNC1 |         /* Pin is configured as FC0_RXD_SDA_MOSI */
         IOCON_PIO_MODE_INACT |    /* No addition pin function */
         IOCON_PIO_INV_DI |        /* Input function is not inverted */
         IOCON_PIO_DIGITAL_EN |    /* Enables digital function */
         IOCON_PIO_INPFILT_OFF |   /* Input filter disabled */
         IOCON_PIO_SLEW_STANDARD | /* Standard mode, output slew rate control is enabled */
         IOCON_PIO_OPENDRAIN_DI    /* Open drain is disabled */
         );
    IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN18_IDX,
                    port0_pin0_config); /* PORT0 PIN0 (coords: 31) is configured as FC0_RXD_SDA_MOSI */
    const uint32_t port0_pin1_config =
        (IOCON_PIO_FUNC1 |         /* Pin is configured as FC0_TXD_SCL_MISO */
         IOCON_PIO_MODE_INACT |    /* No addition pin function */
         IOCON_PIO_INV_DI |        /* Input function is not inverted */
         IOCON_PIO_DIGITAL_EN |    /* Enables digital function */
         IOCON_PIO_INPFILT_OFF |   /* Input filter disabled */
         IOCON_PIO_SLEW_STANDARD | /* Standard mode, output slew rate control is enabled */
         IOCON_PIO_OPENDRAIN_DI    /* Open drain is disabled */
         );
    IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN20_IDX,
                    port0_pin1_config); /* PORT0 PIN1 (coords: 32) is configured as FC0_TXD_SCL_MISO */
}

//void BOARD_InitPins_Uart5(void)
//{                                    /* Function assigned for the Core #0 (ARM Cortex-M4) */
//     CLOCK_EnableClock(kCLOCK_Iocon);                           /* Enables the clock for the IOCON block. 0 = Disable; 1 = Enable.: 0x01u */

//  const uint32_t port0_pin18_config = (
//    IOCON_PIO_FUNC1 |                                        /* Pin is configured as FC0_RXD_SDA_MOSI */
//    IOCON_PIO_MODE_INACT |                                   /* No addition pin function */
//    IOCON_PIO_INV_DI |                                       /* Input function is not inverted */
//    IOCON_PIO_DIGITAL_EN |                                   /* Enables digital function */
//    IOCON_PIO_INPFILT_OFF |                                  /* Input filter disabled */
//    IOCON_PIO_SLEW_STANDARD |                                /* Standard mode, output slew rate control is enabled */
//    IOCON_PIO_OPENDRAIN_DI                                   /* Open drain is disabled */
//  );
//  IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN18_IDX, port0_pin18_config); /* PORT0 PIN0 (coords: 31) is configured as FC0_RXD_SDA_MOSI */
//  const uint32_t port0_pin20_config = (
//    IOCON_PIO_FUNC1 |                                        /* Pin is configured as FC0_TXD_SCL_MISO */
//    IOCON_PIO_MODE_INACT |                                   /* No addition pin function */
//    IOCON_PIO_INV_DI |                                       /* Input function is not inverted */
//    IOCON_PIO_DIGITAL_EN |                                   /* Enables digital function */
//    IOCON_PIO_INPFILT_OFF |                                  /* Input filter disabled */
//    IOCON_PIO_SLEW_STANDARD |                                /* Standard mode, output slew rate control is enabled */
//    IOCON_PIO_OPENDRAIN_DI                                   /* Open drain is disabled */
//  );
//  IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN20_IDX, port0_pin20_config); /* PORT0 PIN1 (coords: 32) is configured as FC0_TXD_SCL_MISO */
//}

void USART5_InitPins(void) 
{ /* Function assigned for the Core #0 (ARM Cortex-M4) */
  CLOCK_EnableClock(kCLOCK_Iocon);                           /* Enables the clock for the IOCON block. 0 = Disable; 1 = Enable.: 0x01u */

//////  const uint32_t port0_pin0_config = (
//////    IOCON_PIO_FUNC1 |                                        /* Pin is configured as FC0_RXD_SDA_MOSI */
//////    IOCON_PIO_MODE_INACT |                                   /* No addition pin function */
//////    IOCON_PIO_INV_DI |                                       /* Input function is not inverted */
//////    IOCON_PIO_DIGITAL_EN |                                   /* Enables digital function */
//////    IOCON_PIO_INPFILT_OFF |                                  /* Input filter disabled */
//////    IOCON_PIO_SLEW_STANDARD |                                /* Standard mode, output slew rate control is enabled */
//////    IOCON_PIO_OPENDRAIN_DI                                   /* Open drain is disabled */
//////  );
//////  IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN18_IDX, port0_pin0_config); /* PORT0 PIN0 (coords: 31) is configured as FC0_RXD_SDA_MOSI */
//////  const uint32_t port0_pin1_config = (
//////    IOCON_PIO_FUNC1 |                                        /* Pin is configured as FC0_TXD_SCL_MISO */
//////    IOCON_PIO_MODE_INACT |                                   /* No addition pin function */
//////    IOCON_PIO_INV_DI |                                       /* Input function is not inverted */
//////    IOCON_PIO_DIGITAL_EN |                                   /* Enables digital function */
//////    IOCON_PIO_INPFILT_OFF |                                  /* Input filter disabled */
//////    IOCON_PIO_SLEW_STANDARD |                                /* Standard mode, output slew rate control is enabled */
//////    IOCON_PIO_OPENDRAIN_DI                                   /* Open drain is disabled */
//////  );
//////  IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN20_IDX, port0_pin1_config); /* PORT0 PIN1 (coords: 32) is configured as FC0_TXD_SCL_MISO */
	IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN20_IDX, IOCON_MODE_INACT | IOCON_FUNC1 | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);//RX
	IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN18_IDX, IOCON_MODE_INACT | IOCON_FUNC1 | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);//TX
}


void USART5_DeinitPins(void) { /* Function assigned for the Core #0 (ARM Cortex-M4) */
//  CLOCK_EnableClock(kCLOCK_Iocon);                           /* Enables the clock for the IOCON block. 0 = Disable; 1 = Enable.: 0x01u */

//  IOCON->PIO[0][18] = ((IOCON->PIO[0][18] &
//    (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))      /* Mask bits to zero which are setting */
//      | IOCON_PIO_FUNC(PIO00_FUNC_ALT0)                      /* Selects pin function.: PORT00 (pin 31) is configured as PIO0_0 */
//      | IOCON_PIO_DIGIMODE(PIO00_DIGIMODE_DIGITAL)           /* Select Analog/Digital mode.: Digital mode. */
//    );
//  IOCON->PIO[0][20] = ((IOCON->PIO[0][20] &
//    (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))      /* Mask bits to zero which are setting */
//      | IOCON_PIO_FUNC(PIO01_FUNC_ALT0)                      /* Selects pin function.: PORT01 (pin 32) is configured as PIO0_1 */
//      | IOCON_PIO_DIGIMODE(PIO01_DIGIMODE_DIGITAL)           /* Select Analog/Digital mode.: Digital mode. */
//    );
}


/*
 * TEXT BELOW IS USED AS SETTING FOR THE PINS TOOL *****************************
BOARD_InitPins_Core1:
- options: {coreID: cm0plus, enableClock: 'true'}
- pin_list: []
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR THE PINS TOOL ***
 */

/*FUNCTION**********************************************************************
 *
 * Function Name : BOARD_InitPins_Core1
 *
 *END**************************************************************************/
//void BOARD_InitPins_Core1(void)
//{ /* Function assigned for the Core #1 (ARM Cortex-M0+) */
//}

/*******************************************************************************
 * EOF
 ******************************************************************************/
