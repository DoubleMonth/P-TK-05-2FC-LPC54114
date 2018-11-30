#include "app_si70xx.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"

#include "fsl_common.h"
#include "fsl_iocon.h"
#include "fsl_i2c.h"
#include "app_interrupt.h"

#include <stdbool.h>

struct SI7020_DATA si7020_data;//读取的传感器值
struct sensor sensor;
void si70xxInit(void)
{


	i2c_master_config_t masterConfig;
	
	IOCON_PinMuxSet(IOCON, 1,  1, IOCON_MODE_PULLUP | IOCON_FUNC5 | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);
	IOCON_PinMuxSet(IOCON, 1,  2, IOCON_MODE_PULLUP | IOCON_FUNC5 | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);
	
	
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);
	RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn);
	
	I2C_MasterGetDefaultConfig(&masterConfig);
	masterConfig.baudRate_Bps = 100000U;
	I2C_MasterInit((I2C_Type *)I2C4_BASE, &masterConfig, 12000000);
}
uint8_t si70xx_buff[1];
uint8_t si70xx_data[2];
uint8_t si7020Measure(double *temperature, double *humidity)
{
	status_t reVal = kStatus_Fail;

	uint16_t value=0;
	double temp;
    if (kStatus_Success == I2C_MasterStart((I2C_Type *)I2C4_BASE, 0x40, kI2C_Write))
    {
		si70xx_buff[0] = 0xE3;
        reVal = I2C_MasterWriteBlocking((I2C_Type *)I2C4_BASE, si70xx_buff, 1, 1);
        if (reVal != kStatus_Success)
        {
            return 0;
        }
		I2C_MasterStart((I2C_Type *)I2C4_BASE, 0x40, kI2C_Read);
		reVal = I2C_MasterReadBlocking((I2C_Type *)I2C4_BASE, si70xx_data, 2, 0);
		I2C_MasterStop((I2C_Type *)I2C4_BASE);
//		dwLedTime=500;
		value=si70xx_data[1]|(si70xx_data[0]<<8);
		temp=(double)value;
		temp=(temp/65536.0f)*175.72f-46.85f;
		*temperature=temp*10;
//		sensor.temp=(int16_t)(temp * 10 );
//		PRINTF("si70xx_temp=%d\r\n",(uint16_t)(temp*10));
    }
	if (kStatus_Success == I2C_MasterStart((I2C_Type *)I2C4_BASE, 0x40, kI2C_Write))
    {
		si70xx_buff[0] = 0xE5;
        reVal = I2C_MasterWriteBlocking((I2C_Type *)I2C4_BASE, si70xx_buff, 1, 1);
        if (reVal != kStatus_Success)
        {
            return 0;
        }
		I2C_MasterStart((I2C_Type *)I2C4_BASE, 0x40, kI2C_Read);
		reVal = I2C_MasterReadBlocking((I2C_Type *)I2C4_BASE, si70xx_data, 2, 0);
		I2C_MasterStop((I2C_Type *)I2C4_BASE);
//		dwLedTime=500;
		value=si70xx_data[1]|(si70xx_data[0]<<8);
		temp=(double)value;
		temp=(temp/65536.0f)*125.0f-6.0f;
		*humidity=temp*10;
//		sensor.humi = (uint16_t)(temp*10);    
//		PRINTF("si70xx_humi=%d\r\n",(uint16_t)(temp*10));
    }
	if( reVal != kStatus_Success )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
