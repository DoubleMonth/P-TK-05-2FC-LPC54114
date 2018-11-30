#include <stdlib.h>
#include <string.h>
#include "app_lcd.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "fsl_iocon.h"
#include "fsl_i2c.h"
#include "app_interrupt.h"
#include <stdbool.h>
#include "app_dev_ctrl.h"
#include "app_si70xx.h"
#define EXAMPLE_I2C_MASTER_BASE (I2C1_BASE)
//#define EXAMPLE_I2C_SLAVE_BASE (I2C4_BASE)
#define I2C_MASTER_CLOCK_FREQUENCY (12000000)
//#define I2C_SLAVE_CLOCK_FREQUENCY (12000000)

#define EXAMPLE_I2C_MASTER ((I2C_Type *)EXAMPLE_I2C_MASTER_BASE)
//#define EXAMPLE_I2C_SLAVE ((I2C_Type *)EXAMPLE_I2C_SLAVE_BASE)

#define I2C_MASTER_SLAVE_ADDR_7BIT (0x50U)
#define I2C_BAUDRATE (100000) /* 100K */
#define I2C_DATA_LENGTH (32)  /* MAX is 256 */
uint8_t auto_speed;
struct DISP_AP display_ap;
struct FLASH_STRUCT flash_struct;
extern struct SI7020_DATA si7020_data;//读取的传感器值
#define _a      0x80
#define _b      0x08
#define _c      0x02
#define _d      0x10
#define _e      0x20
#define _f      0x40
#define _g      0x04

const uint8_t digit_table[18] =
{
    _a+_b+_c+_d+_e+_f,          //0
    _b + _c,                    //1
    _a+_b+_g+_e+_d,             //2
    _a+_b+_c+_d+_g,             //3
    _b+_c+_g+_f,                //4
    _a+_c+_d+_g+_f,             //5
    _a+_c+_d+_e+_f+_g,          //6
    _a+_b+_c,                   //7
    _a+_b+_c+_d+_e+_f+_g,       //8
    _a+_b+_c+_f+_g+_d,          //9
    _a+_b+_c+_e+_f+_g,          //a
    _c+_d+_e+_f+_g,             //b
    _a+_d+_e+_f,                //c
    _b+_c+_d+_e+_g,             //d
    _a+_d+_e+_f+_g,             //e
    _a+_e+_f+_g,                //f
    _a+_b+_e+_f+_g,             //p
    _b + _c+_d,                 //j   g
};
//4，5位置数字用段码表
#define a      0x10
#define b      0x01
#define c      0x04
#define d      0x80
#define e      0x40
#define f      0x20
#define g      0x02

const uint8_t digit_table2[16] =
{
    a+b+c+d+e+f,                //0
    b + c,                      //1
    a+b+g+e+d,                  //2
    a+b+c+d+g,                  //3
    b+c+g+f,                    //4
    a+c+d+g+f,                  //5
    a+c+d+e+f+g,                //6
    a+b+c ,                     //7
    a+b+c+d+e+f+g,              //8
    a+b+c+f+g+d,                //9
    a+b+c+e+f+g,                //a
    c+d+e+f+g,                  //b
    a+d+e+f,                    //c
    b+c+d+e+g,                  //d
    a+d+e+f+g,                  //e
    a+e+f+g,                    //f
};
const struct LCD_SIGN sign_table[]=
{
    {PLC, 			    OTHER,      0,   0x80},//载波图标
    {DIP,               NO_GRP,     2,   0x01},//小数点
    {VALVE,			    NO_GRP,     0,   0x40},//阀门图标
    {BREAKDOWN,         OTHER,      0,   0x20},//报警图标
    {LOCK, 			    OTHER,      0,   0x10},
    {MINUS_DECADE, 	    OTHER,      3,   0x04},//负号
    {MINUS_UNIT, 	    OTHER,      1,   0x04},
    {MINUS_SMALL, 	    OTHER,      2,   0x04},
    
    {E, 	            ERR_OR,     3,   0xF4},//E
    {R1, 	            ERR_OR,     2,   0x24},//R
    {R2, 	            ERR_OR,     1,   0x24},//R
    {O, 	            ERR_OR,     5,   0xC6},//O
    {R3, 	            ERR_OR,     4,   0x42},//R

    {_SET_TEMP,         SET_GRP,    6,   0x04},

    {ROOM_TEMP,         POW_ON_GRP, 6,   0x08},
    {SPEED,             POW_ON_GRP, 0,   0x01},//风速图标

    {VENTILATE_MODE,    MODE_GRP,   5,   0x08},//模式选择
    {COOL_MODE,         MODE_GRP,   6,   0x02},
    {HEAT_MODE,         MODE_GRP,   6,   0x01},

    {LOW_REV,           SPEED_GRP,  0,   0x02},//风速
    {MIDDLE_REV,        SPEED_GRP,  0,   0x04},
    {HIGH_REV,          SPEED_GRP,  0,   0x08},
    {AUTO_REV,          SPEED_GRP,  3,   0x01},
	//固定显示图标	
	
	{C,                 ALL_GRP,    1,   0x01},//温度图标
	{RH,                ALL_GRP,    4,   0x08},//湿度图标
};
#define SIGN_TABLE_SZ  (sizeof(sign_table)/sizeof(sign_table[0]))
uint8_t lcd_buf[LCD_BUF_SIZE];

uint8_t lcd_buffer[LCD_BUF_SIZE];
void buffer2buff(uint8_t *dest,uint8_t *source,uint8_t len)//将数组内的数据向右移4位。
{
	uint8_t i,j;
	for(j=0;j<len;j++)
	{
		dest[j]=0x00;
	}
	for(i=0;i<len;i++)
	{
		dest[i]|=(source[6-i]&0x0f)<<4;//取低4位后左移4位
		dest[i]|=(source[5-i]&0xf0)>>4;//取高4位后右移4位
	}
}
void display_fix_signal()	// 显示固定显示的图标
{
	uint8_t i;
	for(i=0;i<SIGN_TABLE_SZ;i++)
	{
		if(ALL_GRP!=sign_table[i].group)continue;
		lcd_buffer[sign_table[i].idx]|=sign_table[i].val;
		buffer2buff(lcd_buf,lcd_buffer,7);//将数据右移4位。
	}
	
}

//显示指定的某一个图标


void display_signal(uint8_t sig)
{
//	lcd_buf[signal_table[0].idx]=signal_table[0].val;
	uint8_t i;
	for(i=0;i<SIGN_TABLE_SZ;i++)
	{
		if(sig==sign_table[i].sig)break;
	}
	if(i>SIGN_TABLE_SZ) return;
//	for(j=0;j<SIGN_TABLE_SZ;j++)
//	{
//		if((sign_table[i].group == sign_table[j].group) && (0xFF != sign_table[i].group))
//		{
//			
//		}	
//	}
	lcd_buffer[sign_table[i].idx]|=sign_table[i].val;
	buffer2buff(lcd_buf,lcd_buffer,7);//将数据右移4位。
}
//清除显示指定的某个图标
void clr_display_signal(uint8_t sig)
{
	uint8_t i;
	for(i=0;i<SIGN_TABLE_SZ;i++)
	{
		if(sig==sign_table[i].sig)break;
	}
	if(i>SIGN_TABLE_SZ) return;
	lcd_buffer[sign_table[i].idx]&=(~sign_table[i].val);
		buffer2buff(lcd_buf,lcd_buffer,7);//将数据右移4位。
}
//显示某一组图标或某一组中的某一个，
//uint8_t group为组号，
//如果uint8_t sig=NULL;为显示一组的图标
void display_group_sig(uint8_t group, uint8_t sig)
{
	uint8_t i;
	for(i=0;i<SIGN_TABLE_SZ;i++)
	{
		if(group==sign_table[i].group)//找到分组号
		{
			if(sig==NULL)//显示组内全部图标
			{
				lcd_buffer[sign_table[i].idx]|=sign_table[i].val;
				buffer2buff(lcd_buf,lcd_buffer,7);//将数据右移4位。
			}
			else//显示指定图标
			{
				if(sign_table[i].sig ==sig)
                {
                    lcd_buffer[sign_table[i].idx]|=sign_table[i].val;
					buffer2buff(lcd_buf,lcd_buffer,7);//将数据右移4位。
                    break;
                }                    
                else continue;
			}
		}
		else
            continue ;
	}
}

void clr_display_group_sig(uint8_t group, uint8_t sig)
{
	uint8_t i;
	for(i=0;i<SIGN_TABLE_SZ;i++)
	{
		
		if(group==sign_table[i].group)
		{
			
			if(sig==NULL)
			{
				lcd_buffer[sign_table[i].idx]&=(~sign_table[i].val);
				buffer2buff(lcd_buf,lcd_buffer,7);//将数据右移4位。
			}
			
		}
        else continue;
	}
}
/*
在相应位置显示数字
在液晶上对应的位置为，   1    2    3
															4 5
uint8_t position 在position位置显示
uint8_t number 要显示的数字。
*/
void num_display_ctrl(uint8_t position, uint16_t number)
{

    if((5!=position)&&(4!=position))
    {
        if(lcd_buffer[4-position]&0x01)
            lcd_buffer[4-position]=digit_table[number]|0x01;//将显示的位置转换成buffer的位置 1,2，3 //当有图标显示时，图标位置位，显示图标
        else
            lcd_buffer[4-position]=digit_table[number];//当没有图标显示时，只显示数字，
    }
    else
    {
        if(lcd_buffer[9-position]&0x08)
            lcd_buffer[9-position]=digit_table2[number]|0x08;//将显示的位置转换成buffer的位置 4,5//当有图标显示时，图标位置位，显示图标
        else
            lcd_buffer[9-position]=digit_table2[number];//将显示的位置转换成buffer的位置 4,5//当没有图标显示时，只显示数字，
    }
}
/****
功能：求10的n次幂，
*****/
uint32_t exponentiation(uint8_t n)
{
	uint8_t i;
	uint32_t temp=1;
	for(i=0;i<n;i++)
		temp*=10;
	return temp;
}
/****
功能：显示一个数 number，共显示n位。
*****/
void num_display(uint32_t number,uint8_t n)
{

    uint8_t i;
    for(i=1;i<=n;i++)
    {
        num_display_ctrl(i,number % exponentiation(6-i) / exponentiation (5-i));
    }
    
}
//清除某个位置的数字显示
void clr_num_display(uint8_t position)
{
	if((5!=position)&&(4!=position))
	{
		if(lcd_buffer[4-position]&0x01)
			lcd_buffer[4-position]=0x00|0x01;//将显示的位置转换成buffer的位置 1,2，3 //当有图标显示时，图标位置位，显示图标
		else
			lcd_buffer[4-position]=0x00;//当没有图标显示时，只显示数字，
	}
	else
	{
		if(lcd_buffer[9-position]&0x08)
			lcd_buffer[9-position]=0x00|0x08;//将显示的位置转换成buffer的位置 4,5//当有图标显示时，图标位置位，显示图标
		else
			lcd_buffer[9-position]=0x00;//将显示的位置转换成buffer的位置 4,5//当没有图标显示时，只显示数字，
	}
}
void clr_position_num(uint8_t position,uint8_t num)
{
    uint8_t i;
    for(i=position;i<=num;i++)
    clr_num_display(i);//清除一次之前显示的数字
//    clr_num_display(1);//清除一次之前显示的数字
//                clr_num_display(2);
//                clr_num_display(3);
}
struct FLASH_STRUCT flash_struct;
void flash_display_delay(void)
{
	if(flash_struct.set_start_flag)
	{
        if(flash_struct.s_counter++>=3)
        {
            flash_struct.s_counter = 0;
            flash_struct.set_start_flag = false;
        }
	}
}
//设置温度闪烁

//void flash_num_display(uint8_t position, uint8_t number,uint8_t fre, uint8_t time)
extern struct TEMP_PARAM tp_param;
extern struct dev dev;//读写FLASH的结构体
uint8_t plc_state_flag;

void temp_display_flash(void)//图标闪烁
{
	static uint8_t counter;
	if(flash_struct.s_counter>=18)//闪烁3下，3*6
	{
        flash_struct.s_counter = 0;
        flash_struct.set_start_flag = false;
		
	}
				
	if(flash_struct.set_start_flag==true)
	{
		flash_struct.ms_counter++;
		
		if(flash_struct.s_counter>=18)
        {
			flash_struct.s_counter = 0;
			flash_struct.set_start_flag = false;
		
        }
		if(flash_struct.ms_counter>=10)//ms_counter使用全局的可以在检测到按键再次按下的时候清除计数
		{
			flash_struct.ms_counter=0;
		}
		if(flash_struct.ms_counter<5)
		{
//			clr_display_group_sig(ERR_OR,NULL);//温湿度传感器错误
            flash_struct.s_counter++;
            if(0==dev.temp_param.set_temp % 1000 / 100)
                clr_num_display(1);
            else
			num_display_ctrl(1,dev.temp_param.set_temp % 1000 / 100);//显示数字
			num_display_ctrl(2,dev.temp_param.set_temp % 100 / 10);//显示数字
			num_display_ctrl(3,dev.temp_param.set_temp % 10);//显示数字
//			num_display_ctrl(4,((uint16_t)si7020_data.humidity) % 1000 / 100);
//			num_display_ctrl(5,((uint16_t)si7020_data.humidity) % 100 / 10);
//			clr_num_display(4);
//			clr_num_display(5);
//			clr_display_signal(RH);//不显示室内温度图标
			display_signal(_SET_TEMP);
			display_signal(DIP);
			clr_display_signal(ROOM_TEMP);//不显示室内温度图标
            
		}
		else if(flash_struct.ms_counter<10)
		{
//			clr_display_group_sig(ERR_OR,NULL);//温湿度传感器错误
            clr_num_display(1);
			clr_num_display(2);
			clr_num_display(3);
//            clr_position_num(1,3);
			display_signal(_SET_TEMP);
			clr_display_signal(DIP);
			clr_display_signal(ROOM_TEMP);//不显示室内温度图标
//            clr_display_group_sig(ERR_OR,NULL);//温湿度传感器错误
		}
	}
	else
	{
		display_signal(ROOM_TEMP);//显示室内温度图标
		clr_display_signal(_SET_TEMP);//清除SET图标显示
	}
	
	counter++;
	if(counter>=10)
			counter=0;
	if(flash_struct.breakdown_start_flag==true)//故障图标闪烁
	{
		if(counter<5)
		{
			display_signal(BREAKDOWN);
		}
		else if(counter<10)
		{
			clr_display_signal(BREAKDOWN);
		}
	}
	else
	{
		clr_display_signal(BREAKDOWN);//不显示
	}
	if(flash_struct.lock_start_flag==true)//锁定图标闪烁
	{
		if(counter<5)
		{
			display_signal(LOCK);
		}
		else if(counter<10)
		{
			clr_display_signal(LOCK);
		}
	}
	else
	{
		clr_display_signal(LOCK);//不显示
	}
}


int16_t temp;
int16_t humi;
uint8_t auto_speed;
uint8_t lcd_temp_update=0;
extern struct TEMPERATURE_C temperature_c;
void update_lcd_data(void)
{
	
	if(flash_struct.breakdown_start_flag!=true)//温湿度传感器正常
    {
		temperature_c.cur_temp=(int)si7020_data.temperature;
        temperature_c.cur_temp = temperature_c.cur_temp > 1000 ? 999 : temperature_c.cur_temp <= 0 ? 0 : temperature_c.cur_temp;
//        if(0==humi % 1000 / 100)
//            clr_num_display(4);
//        else
		if(flash_struct.set_start_flag!=true)
		{
			num_display_ctrl(4,((uint16_t)si7020_data.humidity )% 1000 / 100);
			num_display_ctrl(5,((uint16_t)si7020_data.humidity) % 100 / 10);
//			display_signal(RH);
		}
    }
	if(flash_struct.set_start_flag!=true)
	{

        if(flash_struct.breakdown_start_flag!=true)//温湿度传感器正常
        {
           
            if(si7020_data.temperature>=0)        //温度大于0
           {
//                temp = abs(temp)>999 ? 999 : abs(temp);
//                if(0==temp % 1000 / 100)
//                    clr_num_display(1);
//                else
                num_display_ctrl(1,((uint16_t)si7020_data.temperature) % 1000 / 100);
                num_display_ctrl(2,((uint16_t)si7020_data.temperature) % 100 / 10);
                num_display_ctrl(3,((uint16_t)si7020_data.temperature) % 10);
           }
//           else if( (si7020_data.temperature<0)&&(si7020_data.temperature>(-100)))//温度小于0但大于 -10度
//           {
////                clr_num_display(1);//清除一次之前显示的数字
////                clr_num_display(2);
////                clr_num_display(3);
//               clr_position_num(1,3);
//                temp = abs(temp);
//                display_signal(MINUS_DECADE);//MINUS_SMALL MINUS_UNIT  MINUS_DECADE
//                num_display_ctrl(2,temp % 100 / 10);
//                num_display_ctrl(3,temp % 10);
//           }
//           else if(si7020_data.temperature<=(-100))//温度小于等于 - 10度时显示- - - 
//           {
////                clr_num_display(1);//清除一次之前显示的数字
////                clr_num_display(2);
////                clr_num_display(3);
//               clr_position_num(1,3);
//                display_signal(MINUS_DECADE);//MINUS_SMALL MINUS_UNIT  MINUS_DECADE
//                display_signal(MINUS_UNIT);//MINUS_SMALL MINUS_UNIT  MINUS_DECADE
//                display_signal(MINUS_SMALL);//MINUS_SMALL MINUS_UNIT  MINUS_DECADE
//           }
           display_signal(DIP);
        }
        else                                            //温湿度传感器故障
        {
//            clr_num_display(1);//清除一次之前显示的数字
//            clr_num_display(2);
//            clr_num_display(3);
//            clr_num_display(4);//清除一次之前显示的数字
//            clr_num_display(5);
            clr_position_num(1,5);
            display_group_sig(ERR_OR,NULL);//温湿度传感器错误
        }
	}
	if(dev.temp_param.air_coner_switch==ON)
	{
		display_signal(SPEED);
		if(dev.temp_param.mode==VENTILATE)      //通风模式下显示通风的风速
        {
          speed_sig_display(dev.temp_param.ventilate_speed);
        }
        else                                      //其他模式下显示的风速
        {
            speed_sig_display(dev.temp_param.win_speed);
        }
            
        mode_sig_display(dev.temp_param.mode);
	}
	if(dev.temp_param.air_coner_switch==OFF)
	{
		
		clr_display_group_sig(MODE_GRP,NULL);//模式图标
		clr_display_group_sig(SPEED_GRP,NULL);//分段风速图标
		clr_display_group_sig(POW_ON_GRP,NULL);//室内温度、风速图标
		clr_display_group_sig(NO_GRP,NULL);
    
		display_signal(DIP);//显示小数点
		display_signal(ROOM_TEMP);//显示室内温度图标
	}
    display_fix_signal();// 显示固定显示的图标
    buffer2buff(lcd_buf,lcd_buffer,7);//将数据右移4位。

		
}
void mode_sig_display(uint8_t temp)
{
	switch(temp)
	{
		case COOL: 
		{
			display_signal(COOL_MODE);
			clr_display_signal(HEAT_MODE);    
			clr_display_signal(VENTILATE_MODE);
		}break;
		case HEAT:
		{
			clr_display_signal(COOL_MODE);
			display_signal(HEAT_MODE);    
			clr_display_signal(VENTILATE_MODE);
		}break;
		case VENTILATE:
		{
			clr_display_signal(COOL_MODE);
			clr_display_signal(HEAT_MODE);    
			display_signal(VENTILATE_MODE);
		}break;
		
		default : ;
	}
}
void speed_sig_display(uint8_t temp)
{
    switch(temp)
    {
        case LOW: 
        {
            display_signal(LOW_REV);
            clr_display_signal(MIDDLE_REV);    
            clr_display_signal(HIGH_REV);
            clr_display_signal(AUTO_REV);
        }
        break;
        case MIDDLE: 
        {
            display_signal(LOW_REV);
            display_signal(MIDDLE_REV);    
            clr_display_signal(HIGH_REV);
            clr_display_signal(AUTO_REV);
        }
        break;
        case HIGH: 
        {
            display_signal(LOW_REV);
            display_signal(MIDDLE_REV);    
            display_signal(HIGH_REV);
            clr_display_signal(AUTO_REV);
        }break;
        case AUTO: 
        {
            auto_speed_display();//自动风速时图标显示。
        }
        break;
        default : ;
    }
}


//void mode_sig_display(uint8_t temp)
//{
//	switch(temp)
//	{
//		case COOL: 
//		{
//			display_signal(COOL_MODE);
//			clr_display_signal(HEAT_MODE);    
//			clr_display_signal(VENTILATE_MODE);
//		}break;
//		case HEAT:
//		{
//			clr_display_signal(COOL_MODE);
//			display_signal(HEAT_MODE);    
//			clr_display_signal(VENTILATE_MODE);
//		}break;
//		case VENTILATE:
//		{
//			clr_display_signal(COOL_MODE);
//			clr_display_signal(HEAT_MODE);    
//			display_signal(VENTILATE_MODE);
//		}break;
//		
//		default : ;
//	}
//}

uint8_t bu9796WriteByte(uint8_t add,uint8_t *buf,uint8_t len)
{
//	uint8_t i;
//	status_t reVal = kStatus_Fail;
	if (kStatus_Success == I2C_MasterStart((I2C_Type *)I2C1_BASE, add, kI2C_Write))
	{
//		reVal = I2C_MasterWriteBlocking((I2C_Type *)I2C4_BASE, &add, 1, 1);
//		if (reVal != kStatus_Success)
//		{
//			return -1;
//		}
		I2C_MasterWriteBlocking((I2C_Type *)I2C1_BASE, buf, len, 1);
	}
	return 0;
}

void displayOn(void)
{
	uint8_t cmd_buf[]={SET_IC,0XBF,SET_BLINK_OFF,SET_AP_NORMAL,SET_SHOW_ON};
	bu9796WriteByte(BU9796_ADDR, cmd_buf, sizeof(cmd_buf));
}
void displayOff(void)
{
    uint8_t cmd_buf[] = { SET_IC, SET_SHOW_OFF };

    bu9796WriteByte(BU9796_ADDR,  cmd_buf, sizeof(cmd_buf));
}
void bu9796SoftInit(void)
{
    uint8_t cmd_buf[LCD_BUF_SIZE + 2] = { SET_SOFT_RESET, SET_LOAD_DATA_ADDR };

    memcpy(&cmd_buf[2], lcd_buf, LCD_BUF_SIZE);
    bu9796WriteByte(BU9796_ADDR, cmd_buf, sizeof(cmd_buf));
    displayOn();
}
void showAll(void)
{
    uint8_t cmd_buf[LCD_BUF_SIZE + 6] = { SET_IC, 0xBF, SET_AP_NORMAL, SET_BLINK_OFF, SET_SHOW_ON, SET_LOAD_DATA_ADDR };

    memset(lcd_buf, 0xFF, LCD_BUF_SIZE);
    memcpy(&cmd_buf[6], lcd_buf, LCD_BUF_SIZE);
    
//    static int err_cnt = 0;

    bu9796WriteByte(BU9796_ADDR, cmd_buf,sizeof(cmd_buf));
      
}
void clrAll(void)
{
	uint8_t cmd_buf[LCD_BUF_SIZE + 6] = { SET_IC, 0xBF, SET_AP_NORMAL, SET_BLINK_OFF, SET_SHOW_ON, SET_LOAD_DATA_ADDR };

    memset(lcd_buf, 0, LCD_BUF_SIZE);
    memcpy(&cmd_buf[6], lcd_buf, LCD_BUF_SIZE);
    
//    static int err_cnt = 0;

    bu9796WriteByte(BU9796_ADDR, cmd_buf,sizeof(cmd_buf));
  
}
void lcdReload()
{
	uint8_t cmd_buf[LCD_BUF_SIZE + 6] = { SET_IC, 0xBF, SET_AP_NORMAL, SET_BLINK_OFF, SET_SHOW_ON, SET_LOAD_DATA_ADDR };
	
	memcpy(&cmd_buf[6], lcd_buf, LCD_BUF_SIZE);
	
	uint8_t  error = bu9796WriteByte(BU9796_ADDR, cmd_buf,sizeof(cmd_buf));
}
void bu9796Init(void)
{
	i2c_master_config_t masterConfig;
  
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);
    RESET_PeripheralReset(kFC1_RST_SHIFT_RSTn);
	
	IOCON_PinMuxSet(IOCON,0,23,0X181);
	IOCON_PinMuxSet(IOCON,0,24,0X181);
  
	I2C_MasterGetDefaultConfig(&masterConfig);
	masterConfig.baudRate_Bps = I2C_BAUDRATE;
	masterConfig.enableMaster = 1;
	I2C_MasterInit(EXAMPLE_I2C_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);
}
void backlight_init(void)
{
	GPIO->DIR[1] |= 1U << 11;
	GPIO->B[1][11] = 0;
	IOCON_PinMuxSet(IOCON, 1, 11, IOCON_MODE_PULLUP | IOCON_FUNC0 | IOCON_GPIO_MODE | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);
}
void backligt_on(void)
{
	GPIO->B[1][11] = 1;
}
void backlight_off(void)
{
	GPIO->B[1][11] = 0;
}
void backlight_toggle(void)
{
	GPIO->NOT[1] |= (1 << 11);
}


