#include "fsl_debug_console.h"
#include "fsl_inputmux.h"
#include "fsl_pint.h"
#include "board.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "fsl_iocon.h"
#include <stdbool.h>
#include "board.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "fsl_iocon.h"
#include "fsl_i2c.h"
#include "app_interrupt.h"
#include "app_key.h"
#include "app_dev_ctrl.h"
#include "app_lcd.h"
#include "app_beep.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_PINT_PIN_INT0_SRC kINPUTMUX_GpioPort1Pin8ToPintsel
#define DEMO_PINT_PIN_INT1_SRC kINPUTMUX_GpioPort1Pin9ToPintsel
#define DEMO_PINT_PIN_INT2_SRC kINPUTMUX_GpioPort1Pin10ToPintsel

extern void display_signal(uint8_t sig);
//extern void num_display_ctrl(uint8_t position, uint8_t number);
struct TEMP_PARAM tp_param;
struct TEMPERATURE_C temperature_c;

extern void local_key_report(void);
uint8_t key_pressed;
static struct KEY_VALUE key_value_table[]=
{
	{0,0,0},
	{1,0,0},
	{0,1,0},
	{1,1,0},
	{0,1,1},
	{0,0,1},
	{1,1,1},
};
struct KEY_VALUE key_value_temp;//存储按键值
struct CAP_KEY cap_key;
#define KEY_VALUE_TABLE_SIZE (sizeof(key_value_table)/sizeof(key_value_table[0]))
	

#define KEY_GPIO_CFG IOCON_MODE_PULLUP | IOCON_FUNC0 | IOCON_GPIO_MODE | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
const uint8_t  KEY_GPIO_PORT[KEY_NUM] = { 1,  1,  1,  1 };
const uint8_t  KEY_GPIO_PIN [KEY_NUM] = { 8,  9, 10, 11 };

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
void pint_intr_callback(pint_pin_int_t pintr, uint32_t pmatch_status)
{
    
	if(GPIO_ReadPinInput(GPIO, KEY_GPIO_PORT[0], KEY_GPIO_PIN[0])==1)
	{
		key_value_temp.sda=1;
		PRINTF("\f\r\nPINT Pin 1 Interrupt %d event detected.", pintr);
	}
	if(GPIO_ReadPinInput(GPIO, KEY_GPIO_PORT[1], KEY_GPIO_PIN[1])==1)
	{
		key_value_temp.scl=1;
		PRINTF("\f\r\nPINT Pin 2 Interrupt %d event detected.", pintr);
	}
	if(GPIO_ReadPinInput(GPIO, KEY_GPIO_PORT[2], KEY_GPIO_PIN[2])==1)
	{
		key_value_temp.irq=1;
		PRINTF("\f\r\nPINT Pin 3 Interrupt %d event detected.", pintr);
		
	}
	else
		key_value_temp.irq=0;
}
uint8_t key_init(void)
{	
	INPUTMUX_Init(INPUTMUX);
    INPUTMUX_AttachSignal(INPUTMUX, kPINT_PinInt0, DEMO_PINT_PIN_INT0_SRC);
    INPUTMUX_AttachSignal(INPUTMUX, kPINT_PinInt1, DEMO_PINT_PIN_INT1_SRC);
    INPUTMUX_AttachSignal(INPUTMUX, kPINT_PinInt2, DEMO_PINT_PIN_INT2_SRC);

    /* Turnoff clock to inputmux to save power. Clock is only needed to make changes */
    INPUTMUX_Deinit(INPUTMUX);

    /* Initialize PINT */
    PINT_Init(PINT);
	
	/* Setup Pin Interrupt 0 for rising edge */
    PINT_PinInterruptConfig(PINT, kPINT_PinInt0, PINT_PIN_RISE_EDGE, pint_intr_callback);

    /* Setup Pin Interrupt 1 for falling edge */
    PINT_PinInterruptConfig(PINT, kPINT_PinInt1, PINT_PIN_RISE_EDGE, pint_intr_callback);

    /* Setup Pin Interrupt 2 for both rising and falling edge */
    PINT_PinInterruptConfig(PINT, kPINT_PinInt2, PINT_PIN_RISE_EDGE, pint_intr_callback);

    /* Enable callbacks for PINT */
    PINT_EnableCallback(PINT);
	return 0;
}
void clear_iss()
{
	key_value_temp.irq=0;//清除读取到的值，防止出现连续触发。
	key_value_temp.scl=0;
	key_value_temp.sda=0;
}
struct dev dev;

uint8_t  key_scan(void)
{
//	static uint8_t key_value_test=0;
//	key_value();
	uint8_t i;
	for(i=0;i<KEY_VALUE_TABLE_SIZE;i++)
	{
		if(key_value_table[i].irq==key_value_temp.irq && key_value_table[i].scl== key_value_temp.scl && key_value_table[i].sda==key_value_temp.sda)
		{
			
			if(0!=i&&0==cap_key.response_chn)
			{
				cap_key.key_chn=2;
				cap_key.response_chn=4;
			}
			switch(i)
			{
				case SPEED_KEY:                                                //风速      
				{
					clear_iss();
					if(dev.temp_param.panel_lock==ON) 
					{
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						return SPEED_KEY;
					}
					else 
					if (dev.temp_param.air_coner_switch==OFF)//关机状态下不响应按键
					{
						return SPEED_KEY;
					}
					else
					{
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						beep_on();
						cap_key.key_chn=2;
						win_speed_handle();//风速处理
						beep_on();
                       // key_pressed=true;
//                        local_key_report();
					}
				}break;
				case ADD_KEY:                                                //设置温度+
				{
					clear_iss();
					if(dev.temp_param.panel_lock==ON) //面板锁定按下后只点亮背光
					{
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						return ADD_KEY;
					}
					else 
						if(dev.temp_param.air_coner_switch==OFF)//关机状态下不响应按键
					{
						return ADD_KEY;
					}
					else
					{
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						beep_on();
						cap_key.key_chn=2;
						add_temp_handle();//设置温度++
 //                       local_key_report();
					}
				}break;
				case DEC_KEY:                                                //设置温度-
				{
					clear_iss();
					if(dev.temp_param.panel_lock==ON) //面板锁定按下后只点亮背光
					{
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						return DEC_KEY;
					}
					else if(dev.temp_param.air_coner_switch==OFF)//关机状态下不响应按键
					{
						return DEC_KEY;
					}
					else
					{
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						beep_on();
//						buzzer_on();
						cap_key.key_chn=2;
						dec_temp_handle();//设置温度--
 //                       local_key_report();
					}
				}break;
				case MODE_KEY:                                                //模式处理
				{
					clear_iss();
					if(dev.temp_param.panel_lock==ON) //面板锁定按下后只点亮背光
					{
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						//clear_iss();
						return MODE_KEY;
					}
					else if(dev.temp_param.air_coner_switch==OFF)//关机状态下不响应按键
					{
						return MODE_KEY;
					}
					else
					{
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						beep_on();
//						buzzer_on();
						cap_key.key_chn=2;
						mode_handle();//模式处理
 //                       local_key_report();
					}
				}break;
				case SWITCH_KEY:                                                //开关机
				{

					clear_iss();
						if(dev.temp_param.panel_lock==ON) //面板锁定后 按下只点亮背光
						{
							temperature_c.backlight=BL_on_time;//背光点亮一段时间
							return SWITCH_KEY;
						}
						temperature_c.backlight=BL_on_time;//背光点亮一段时间
						beep_on();
//						buzzer_on();
						cap_key.key_chn=2;
						on_off_handle();//开关机
 //                       local_key_report();
				}break;
				default: 
				{
					clear_iss();
				}break ;
			}
		}
	}
  return 0;//
}


