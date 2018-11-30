#include "stdint.h"
#include "fsl_adc.h"
#include "app_led.h"
#include "app_adc.h"
#include "app_dev_ctrl.h"

volatile uint32_t dwSysTicks = 0;
volatile uint32_t dwLedTicks = 0;
volatile uint32_t dwLedTime  = 1000;
extern volatile uint32_t deltms;
void SysTick_Handler(void)
{
	dwSysTicks++;
	dwLedTicks++;
	deltms ++;
	notify(EV_TICK);
//	if(dwLedTicks >= dwLedTime)
//	{
//		dwLedTicks = 0;
//		led_toggle(0);
//	}
}

/*
 * ISR for ADC conversion sequence A done.
 */
//void ADC0_SEQA_IRQHandler(void)
//{
//  if (kADC_ConvSeqAInterruptFlag == (kADC_ConvSeqAInterruptFlag & ADC_GetStatusFlags(ADC0)))
//  {
//    ADC_GetChannelConversionResult(ADC0, ADC_CHANNEL_NUM, gAdcResultInfoPtr);
//    ADC_ClearStatusFlags(ADC0, kADC_ConvSeqAInterruptFlag);
//    bADCSampleFlag = 1;
//  }
//}



// end file
