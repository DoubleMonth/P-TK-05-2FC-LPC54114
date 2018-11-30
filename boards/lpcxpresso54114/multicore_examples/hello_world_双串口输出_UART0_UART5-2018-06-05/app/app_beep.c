#include <stdbool.h>
#include "board.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "fsl_iocon.h"
#include "app_beep.h"
#include "fsl_sctimer.h"
#define IOCON_PIO_DIGITAL_EN          0x80u   /*!< Enables digital function */
#define IOCON_PIO_FUNC1               0x01u   /*!< Selects pin function 1 */
#define IOCON_PIO_FUNC2               0x02u   /*!< Selects pin function 2 */
#define IOCON_PIO_FUNC3               0x03u   /*!< Selects pin function 3 */
#define IOCON_PIO_INPFILT_OFF       0x0100u   /*!< Input filter disabled */
#define IOCON_PIO_INV_DI              0x00u   /*!< Input function is not inverted */
#define IOCON_PIO_MODE_INACT          0x00u   /*!< No addition pin function */
#define IOCON_PIO_OPENDRAIN_DI        0x00u   /*!< Open drain is disabled */
#define IOCON_PIO_SLEW_STANDARD       0x00u   /*!< Standard mode, output slew rate control is enabled */
#define PIN29_IDX                       29u   /*!< Pin number for pin 29 in a port 0 */
#define PORT0_IDX                        0u   /*!< Port index */

#define BUS_CLK_FREQ CLOCK_GetFreq(kCLOCK_BusClk)
uint32_t sctimerClock;
sctimer_config_t sctimerInfo;
sctimer_pwm_signal_param_t pwmParam;
uint32_t event;

void beep_init(void) 
{ /* Function assigned for the Core #0 (ARM Cortex-M4) */
	CLOCK_EnableClock(kCLOCK_Iocon);                           /* Enables the clock for the IOCON block. 0 = Disable; 1 = Enable.: 0x01u */
	const uint32_t port0_pin29_config = (
    IOCON_PIO_FUNC2 |                                        /* Pin is configured as SCT0_OUT2 */
    IOCON_PIO_MODE_INACT |                                   /* No addition pin function */
    IOCON_PIO_INV_DI |                                       /* Input function is not inverted */
    IOCON_PIO_DIGITAL_EN |                                   /* Enables digital function */
    IOCON_PIO_INPFILT_OFF |                                  /* Input filter disabled */
    IOCON_PIO_OPENDRAIN_DI                                   /* Open drain is disabled */
  );
  IOCON_PinMuxSet(IOCON, PORT0_IDX, PIN29_IDX, port0_pin29_config); /* PORT0 PIN29 (coords: 11) is configured as SCT0_OUT2 */
	
	sctimerClock = BUS_CLK_FREQ;
	SCTIMER_GetDefaultConfig(&sctimerInfo);

    /* Initialize SCTimer module */
    SCTIMER_Init(SCT0, &sctimerInfo);


    /* Configure second PWM with different duty cycle but same frequency as before */
    pwmParam.output = kSCTIMER_Out_2;
    pwmParam.level = kSCTIMER_LowTrue;
    pwmParam.dutyCyclePercent = 0;
    SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 1000U, sctimerClock, &event);

//        return -1;


    /* Start the timer */
    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_L);
}
void beep_on(void)
{
	SCTIMER_GetDefaultConfig(&sctimerInfo);
	SCTIMER_Init(SCT0, &sctimerInfo);
	pwmParam.output = kSCTIMER_Out_2;
    pwmParam.level = kSCTIMER_LowTrue;
	pwmParam.dutyCyclePercent = 20;
	SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 1000U, sctimerClock, &event);
	SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_L);
}
void beep_off(void)
{
	SCTIMER_GetDefaultConfig(&sctimerInfo);
	SCTIMER_Init(SCT0, &sctimerInfo);
	pwmParam.output = kSCTIMER_Out_2;
    pwmParam.level = kSCTIMER_LowTrue;
	pwmParam.dutyCyclePercent = 0;
	SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 1000U, sctimerClock, &event);
	SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_L);
}

