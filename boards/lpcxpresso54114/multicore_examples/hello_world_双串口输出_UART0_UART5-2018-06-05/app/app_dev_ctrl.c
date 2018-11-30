#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "fsl_debug_console.h"
#include "app_dev_ctrl.h"
#include "app_si70xx.h"
#include "app_led.h"
#include "app_lcd.h"
#include "app_spiflash.h"
#include "app_key.h"
#include "app_realy.h"
#include "app_beep.h"
#include "smart_plc.h"
#include "fsl_usart_cmsis.h"
#include "protocol_smart.h"
#include "auto_report.h"
#include "auto_report_app.h"
extern struct SI7020_DATA si7020_data;
volatile uint32_t deltms = 0;
volatile uint32_t task_monitor;
extern uint8_t lcd_buf[LCD_BUF_SIZE];
extern uint8_t lcd_buffer[LCD_BUF_SIZE];
extern struct dev dev;
struct reg reg;
extern struct FLASH_STRUCT flash_struct;
uint8_t save_dev_flag;
extern struct TEMPERATURE_C temperature_c;
static uint8_t temp_ctrl_flag_cool=0;//温度回差标志
static uint8_t temp_ctrl_flag_heat=0;//温度回差标志
extern struct CAP_KEY cap_key;
extern struct sensor sensor;
uint8_t low_temp_flag=0;//低温保护回差标志。
uint8_t g_frame_buffer[0x400];
uint8_t plc_alive_flag;
 uint8_t plc_alive_cntr_ms ;//PLC闪烁计时 
uint8_t plc_communication;//PLC通信标志
static const uint8_t dev_type[8] = { 0xFF, 0xFF, 0x2B, 0x00, 0x07, 0x00, 0x00, 0x00 };
static const char *softver = "ESTK-05-2FC-A(v1.0)-20180305";//P-TK-05-2FC-PWR(V1.11),P-TK-05-2FC-PWR(V1.03),P-TK-05-2FC-A-63-MB(V1.02)
static const uint8_t sn[]={0x31, 0x71, 0x20,0x10, 0x00, 0x71, 0x74, 0x71, 0x99, 0x00, 0x00, 0x01};
static const uint8_t dkey[]={0x30, 0x30, 0x30, 0x30, 0x4B, 0x59, 0x54, 0x53};
static const uint8_t aid[]={0x4A,0xEE,0x08, 0x00};	// A 585290
static const uint8_t pwd[]={0x37,0xAF};				// P 44855
//uint8_t gid[]={0x38, 0x34,0x00,0x00};
//uint8_t panid[]={0x3C,0X37};
void temp_param_init(void)
{	
	dev.temp_param.air_coner_switch=ON;                //关机
	dev.temp_param.backlight_enable=MY_ENABLE;          //背光使能
	dev.temp_param.low_temp_protect=MY_ENABLE;          //低温保护
	dev.temp_param.mode=VENTILATE;                      //模式--通风模式
	dev.temp_param.panel_lock=OFF;                      //面板锁定关
	dev.temp_param.set_temp=200;                        //默认设置温度20.0度
	dev.temp_param.state_report=MY_ENABLE;              //状态上报开
	dev.temp_param.win_speed=AUTO;                      //风速--自动
    dev.temp_param.ventilate_speed=LOW;                 //通风模式下的风速默认为低风。
	dev.temp_param.max_cool_temp=300;                   //默认最大制冷温度30度
	dev.temp_param.max_heat_temp=300;                   //默认最大制热温度30度
	dev.temp_param.mix_cool_temp=160;                   //默认最小制冷温度16度
	dev.temp_param.mix_heat_temp=160;                   //默认最小制热温度16度
	dev.temp_param.max_set_temp=300;                    //最大设置温度30度
	dev.temp_param.mix_set_temp=50;                     ///最小设置温度5度
	dev.temp_param.limit_t=10;                          //回差温度1度
    
    dev.power_on_delay = 60;                //传感器无效时间
    dev.temp_report_step = 5;              //默认步长为1度
    dev.temp_report_freq=0X0A;                 //默认关闭定频上报
    dev.humi_report_step = 20;              //默认步长为5%
    dev.humi_report_freq=0X0A;                 //默认关闭定频上报
    dev.humi_reduction = 30;                //湿度默认加3%
    dev.temp_reduction = -15;               //温度默认减1.5度
    dev.active_report=0x01;                 //主动上报网关和设备
    dev.auto_speed_ctrl_mode=0x01;          //默认自动风时关闭风机。
	
	
	memcpy(&dev.encode.sn,sn,12);
	memcpy(&dev.encode.dkey,dkey,8);
	memcpy(&dev.encode.id,aid,4);
	memcpy(&dev.encode.pwd,pwd,2);
//	memcpy(&dev.gid,gid,4);
//	memcpy(&dev.panid,panid,2);
	flash_struct.breakdown_start_flag=false;
	
		
//	mx25r_cmd_write(&mx25r,PARAM_ADDR+offsetof(struct dev, magic),(uint8_t *)&dev,sizeof(dev));
//	mx25r_cmd_read(&mx25r,0x00000f00U,flash_buf,0x08);
	
}
extern struct PLC_MACHINE  plc_state;
void system_init(void)
{
	memset(&plc_state, 0x00, sizeof(plc_state));
	memset(&reg, 0x00, sizeof(reg));
	reg.type = PASSWORD_REG;
	chg_state(RST_PLC);
	temperature_c.ctrl_delay=10;
}
void dev_init(void)
{

//	mx25r_cmd_read(&mx25r,PARAM_ADDR+offsetof(struct dev, magic),(uint8_t *)&dev.magic,MAGIC_NUM_LEN);
	mx25r_cmd_read(&mx25r,PARAM_ADDR,(uint8_t *)&dev.head_magic,MAGIC_NUM_LEN);
	PRINTF("read magic first one =%d\r\n",dev.head_magic);
	if (HEAD_MAGIC_NUM != dev.head_magic)
	{
		memset((uint8_t *)&dev, 0x00, sizeof(dev));
		dev.head_magic=HEAD_MAGIC_NUM;          //开头MAGIC
		dev.magic = MAGIC_NUM;
		temp_param_init();
		mx25r_cmd_wren(&mx25r);
		mx25r_cmd_sector_erase(&mx25r,PARAM_ADDR);
		mx25r_cmd_write(&mx25r,PARAM_ADDR,(uint8_t *)&dev,sizeof(dev));
		mx25r_cmd_wrdi(&mx25r);
		chg_state(RST_PLC);
	}
	
	memset((uint8_t *)&dev, 0x00, sizeof(dev));
	mx25r_cmd_read(&mx25r,PARAM_ADDR,(uint8_t *)&dev,sizeof(dev));
	PRINTF("read magic after write=%d\r\n",dev.head_magic);
	auto_report_init(60, dev.gid, dev.encode.id, dev.sid, &dev.active_report);
////	uint32_t magic,read_magic;
////	magic=HEAD_MAGIC_NUM;
////	mx25r_cmd_read(&mx25r,PARAM_ADDR,(uint8_t *)&read_magic,4);
////	PRINTF("read magic first one =%d\r\n",read_magic);
//////	mx25r_cmd_write(&mx25r,PARAM_ADDR,(uint8_t *)&magic,4);
////	mx25r_cmd_read(&mx25r,PARAM_ADDR,(uint8_t *)&read_magic,4);
////	PRINTF("read magic after write=%d\r\n",read_magic);
}
void write_to_flash(void)
{
	mx25r_cmd_wren(&mx25r);
	mx25r_cmd_sector_erase(&mx25r,PARAM_ADDR);
	mx25r_cmd_write(&mx25r,PARAM_ADDR,(uint8_t *)&dev,sizeof(dev));
	mx25r_cmd_wrdi(&mx25r);
}

int dev_type_get(uint8_t *out, size_t maxlen)
{
    if (maxlen < sizeof(dev_type)) return -1;

    memcpy(out, dev_type, sizeof(dev_type)); 
    return sizeof(dev_type);
}

int dev_ver_get(uint8_t *out, size_t maxlen)
{
    int verlen = strlen(softver);

    if (maxlen < verlen) return -1;

    memcpy(out, softver, verlen);
    return verlen;
}

int dev_ver_cmp(const uint8_t *ver, size_t len)
{
    int verlen = strlen(softver);

    if (len != verlen) return -1;

    return memcmp(ver, softver, verlen);
}
void win_speed_handle(void)
{
	
  if(dev.temp_param.mode==VENTILATE)//通风模式下
  {
    dev.temp_param.ventilate_speed++;
    if(dev.temp_param.ventilate_speed>=3)
    dev.temp_param.ventilate_speed=0;
  }
  else	    //制冷或制热模式。
  {
    dev.temp_param.win_speed++;
    if(dev.temp_param.win_speed>=4)
      dev.temp_param.win_speed=0;
  }
	
	flash_struct.set_start_flag=false;//关闭设置温度闪烁
	save_dev_flag=SAVE_DEV;//结构体数据存储到FLASH标志置为有效
	write_to_flash();
//    write_param_to_flash();//写入Flash
	
}
void add_temp_handle(void)
{
	dev.temp_param.set_temp +=5;
	if(dev.temp_param.mode==HEAT||dev.temp_param.mode==VENTILATE)//制热模式和通风模式时  默认设置温度
	{
		if(dev.temp_param.set_temp>=dev.temp_param.max_heat_temp)
		dev.temp_param.set_temp=dev.temp_param.max_heat_temp;//最高温度
	}
	else if(dev.temp_param.mode==COOL)//制冷模式
	{
		if(dev.temp_param.set_temp>=dev.temp_param.max_cool_temp)
		dev.temp_param.set_temp=dev.temp_param.max_cool_temp;//最高温度
	}
	flash_struct.set_start_flag=true;
	flash_struct.ms_counter=0;
	flash_struct.s_counter=0;
	save_dev_flag=SAVE_DEV;//结构体数据存储到FLASH标志置为有效
	write_to_flash();
//    write_param_to_flash();//写入Flash
}

extern void flash_num_display(uint8_t position, uint8_t number,uint8_t fre, uint8_t time);
void dec_temp_handle(void)
{
	dev.temp_param.set_temp  -=5;//温度--;
	
		if(dev.temp_param.mode==HEAT||dev.temp_param.mode==VENTILATE)//制热模式和通风模式时  默认设置温度
	{
		if(dev.temp_param.set_temp<=dev.temp_param.mix_heat_temp)
		dev.temp_param.set_temp=dev.temp_param.mix_heat_temp;//最低温度
	}
	else if(dev.temp_param.mode==COOL)//制冷模式
	{
		if(dev.temp_param.set_temp<=dev.temp_param.mix_cool_temp)
		dev.temp_param.set_temp=dev.temp_param.mix_cool_temp;//最低温度
	}
	flash_struct.set_start_flag=true;
	flash_struct.ms_counter=0;
	flash_struct.s_counter=0;
	save_dev_flag=SAVE_DEV;//结构体数据存储到FLASH标志置为有效
	write_to_flash();
}
void mode_handle(void)
{
	dev.temp_param.mode++;//模式;
	if(dev.temp_param.mode>=3)
		dev.temp_param.mode=0;
	flash_struct.set_start_flag=false;//关闭设置温度闪烁
	save_dev_flag=SAVE_DEV;//结构体数据存储到FLASH标志置为有效
	write_to_flash();

}
void on_off_handle(void)
{
	dev.temp_param.air_coner_switch ++;//开关机++;
	if(2==dev.temp_param.air_coner_switch )
		dev.temp_param.air_coner_switch =OFF;
	flash_struct.set_start_flag=false;//关闭设置温度闪烁
	save_dev_flag=SAVE_DEV;//结构体数据存储到FLASH标志 置为有效

	flash_struct.set_start_flag=false;//关闭设置温度闪烁
	if(dev.temp_param.air_coner_switch==OFF)
	{
		speed_realy_ctrl(0XFF);
        relay_ctrl(REALY4,REALY_OFF);
//        lcd_backlight_off();
		backlight_off();
		temperature_c.backlight=0;
	}
	write_to_flash();
}
/*
制冷模式下：
设定温度比当前温度低3度时，高风
设定温度比当前温度低1-3度时，中风
设定温度比当前温度低1度时，低风
制热模式下：
设定温度比当前温度高3度时，高风
设定温度比当前温度高1-3度时，中风
设定温度比当前温度高1度时，低风
*/
extern int16_t temp;
extern uint16_t humi;
extern uint8_t auto_speed;

void temp_auto_ctrl()
{
	if(dev.temp_param.air_coner_switch!=ON)
	{
		return;
	}
	switch(dev.temp_param.mode)
	{
		case COOL:
		{
            if(temp>dev.temp_param.set_temp+30)//高风
            {
                auto_speed=HIGH;
                speed_realy_ctrl(HIGH);
                relay_ctrl(REALY4,REALY_ON);
            }
            else if((temp>=dev.temp_param.set_temp+10)&&(temp<=dev.temp_param.set_temp+30))//中风
            {
                auto_speed=MIDDLE;
                speed_realy_ctrl(MIDDLE);
                relay_ctrl(REALY4,REALY_ON);
            }
            else if(temp<=dev.temp_param.set_temp+10)//低风
            {
                auto_speed=LOW;
                speed_realy_ctrl(LOW);
                relay_ctrl(REALY4,REALY_ON);
            }
		}break;
		case HEAT:
		{
            if(temp<dev.temp_param.set_temp-30)//高风
            {
                auto_speed=HIGH;
                speed_realy_ctrl(HIGH);
                relay_ctrl(REALY4,REALY_ON);
            }
            else if((temp<=dev.temp_param.set_temp-10)&&(temp>=dev.temp_param.set_temp-30))//中风
            {
                auto_speed=MIDDLE;
                speed_realy_ctrl(MIDDLE);
                relay_ctrl(REALY4,REALY_ON);
            }
            else if(temp>=dev.temp_param.set_temp-10)//低风
            {
                auto_speed=LOW;
                speed_realy_ctrl(LOW);
                relay_ctrl(REALY4,REALY_ON);
            }
			
		}break;
		default : ;
	}
}
void win_speed_ctrl_realy(uint8_t speed)
{
    switch(speed)
    {
        case LOW: 
        {
            speed_realy_ctrl(LOW);
            relay_ctrl(REALY4,REALY_ON);
        }break;
        case MIDDLE:
        {
            speed_realy_ctrl(MIDDLE);
            relay_ctrl(REALY4,REALY_ON);
        }break;
        case HIGH:
        {
            speed_realy_ctrl(HIGH);
            relay_ctrl(REALY4,REALY_ON);
        }break;
        case AUTO:
        {
            temp_auto_ctrl();
        }
        default: ;
    }
}
void temp_ctrl_realy(void )
{
	if(temperature_c.ctrl_delay>0)//防止设备刚上电，传感器数据还没有读出时继电器动作。当传感器告警时也不进行控制
    {
        relay_ctrl(REALY4,REALY_OFF);//关闭阀门
		speed_realy_ctrl(0XFF);//关闭风机
        return;
    }
    if(dev.temp_param.air_coner_switch!=ON)//关机
	{

        if(dev.temp_param.low_temp_protect!=MY_ENABLE)//低温保护关时，关闭阀门和风机，防止在低温保护后再关闭低温保护功能时，阀门和风机处于开的状态。
        {
            relay_ctrl(REALY4,REALY_OFF);//关闭阀门
			speed_realy_ctrl(0XFF);//关闭风机
            low_temp_flag=0;//低温保护动作标志清除
            return;
        }
		else if(temperature_c.cur_temp<=mix_low_temp_protect)//低温保护功能
		{
			low_temp_flag=1;//低温保护回差标志。
			relay_ctrl(REALY4,REALY_ON);//打开阀门
			speed_realy_ctrl(MIDDLE);//中风
		}
		else if(low_temp_flag==1&&temperature_c.cur_temp>=max_low_temp_protect)
		{
			relay_ctrl(REALY4,REALY_OFF);//关闭阀门
			speed_realy_ctrl(0XFF);//关闭风机
			low_temp_flag=0;
		}
		else if(low_temp_flag==0)
		{
			speed_realy_ctrl(0XFF);
			relay_ctrl(REALY4,REALY_OFF);
		}
		return;
	}
	else
	{
		low_temp_flag=0;//清除低温保护回差标志
		if(dev.temp_param.mode==COOL)//制冷模式下
		{
            temp_ctrl_flag_heat=0;
            if(temperature_c.cur_temp<=dev.temp_param.set_temp-10)
			{
                if(dev.temp_param.win_speed==AUTO)//自动风模式下,默认不关闭风机，选择低风
                {
                    if(dev.auto_speed_ctrl_mode==false)//默认不关闭风机，选择低风
                    {
                        speed_realy_ctrl(LOW);
                        auto_speed=LOW;
                    }
                    else if(dev.auto_speed_ctrl_mode==true)//设置为关闭风机时，关闭风机
                    {
                        speed_realy_ctrl(0XFF);
                        auto_speed=LOW;
                    }
                }
                else//手动模式时关闭风机。
                {
                    speed_realy_ctrl(0XFF);
                }
				relay_ctrl(REALY4,REALY_OFF);//关闭阀门
				temp_ctrl_flag_cool=0;	

				return;
                
			} 
            else if(temperature_c.cur_temp<dev.temp_param.set_temp+10)
            {
                if(temp_ctrl_flag_cool==0)
                {
                    if(dev.temp_param.win_speed==AUTO)//自动风模式下,默认不关闭风机，选择低风
                    {
                        if(dev.auto_speed_ctrl_mode==false)//默认不关闭风机，选择低风
                        {
                            speed_realy_ctrl(LOW);
                            auto_speed=LOW;
                        }
                        else if(dev.auto_speed_ctrl_mode==true)//设置为关闭风机时，关闭风机
                        {
                            speed_realy_ctrl(0XFF);
                            auto_speed=LOW;
                        }
                    }
                    else//手动模式时关闭风机。
                    {
                        speed_realy_ctrl(0XFF);
                    }
                    relay_ctrl(REALY4,REALY_OFF);//关闭阀门
                }
                else 
                {
                    win_speed_ctrl_realy(dev.temp_param.win_speed);
                }
                
            }
            else if(temperature_c.cur_temp>=dev.temp_param.set_temp+10)
            {
                win_speed_ctrl_realy(dev.temp_param.win_speed);
                temp_ctrl_flag_cool=1;	
            }
		}
		else if(dev.temp_param.mode==HEAT)
		{
            temp_ctrl_flag_cool=0;	
            if(temperature_c.cur_temp>=dev.temp_param.set_temp+10)
            {
				if(dev.temp_param.win_speed==AUTO)//自动风模式下,默认不关闭风机，选择低风
                {
                    if(dev.auto_speed_ctrl_mode==false)
                    {
                        speed_realy_ctrl(LOW);
                        auto_speed=LOW;
                    }
                    else if(dev.auto_speed_ctrl_mode==true)
                    {
                        speed_realy_ctrl(0XFF);
                        auto_speed=LOW;
                    }
                }
                else//手动模式时关闭风机。
                {
                  speed_realy_ctrl(0XFF);
                }
                relay_ctrl(REALY4,REALY_OFF);//关闭阀门
                temp_ctrl_flag_heat=0;
             
                return;
            }
            else if (temperature_c.cur_temp>dev.temp_param.set_temp-10)
            {
                if(temp_ctrl_flag_heat==0)
                {
                    if(dev.temp_param.win_speed==AUTO)//自动风模式下,默认不关闭风机，选择低风
                    {
                        if(dev.auto_speed_ctrl_mode==false)//默认不关闭风机，选择低风
                        {
                            speed_realy_ctrl(LOW);
                            auto_speed=LOW;
                        }
                        else if(dev.auto_speed_ctrl_mode==true)//设置为关闭风机时，关闭风机
                        {
                            speed_realy_ctrl(0XFF);
                            auto_speed=LOW;
                        }
                    }
                    else//手动模式时关闭风机。
                    {
                        speed_realy_ctrl(0XFF);
                    }
                    relay_ctrl(REALY4,REALY_OFF);//关闭阀门
                }
                else 
                {
                    win_speed_ctrl_realy(dev.temp_param.win_speed);
                }
            }
            else if(temperature_c.cur_temp<=dev.temp_param.set_temp-10)
            {
                win_speed_ctrl_realy(dev.temp_param.win_speed);
                temp_ctrl_flag_heat=1;
            }
		}
		else if(dev.temp_param.mode==VENTILATE)//通风模式下关闭阀门，根据设定风速控制风机
		{
            temp_ctrl_flag_cool=0;	
            temp_ctrl_flag_heat=0;	
//            log_d(MODULE_APP, "dev.temp_param.mode==VENTILATE\n");
            relay_ctrl(REALY4,REALY_OFF);
			switch(dev.temp_param.ventilate_speed)
			{
				case LOW: 
				{
					speed_realy_ctrl(LOW);
				}break;
				case MIDDLE:
				{
					speed_realy_ctrl(MIDDLE);
				}break;
				case HIGH:
				{
					speed_realy_ctrl(HIGH);
				}break;
				default: ;
			}
			
		}
	}
}
void speed_realy_ctrl(uint8_t ctrl)
{
	switch(ctrl)
	{
		case 0XFF: 
		{
			relay_ctrl(REALY1,REALY_OFF);
			relay_ctrl(REALY2,REALY_OFF);
			relay_ctrl(REALY3,REALY_OFF);
		}break;
		case LOW: 
		{
			relay_ctrl(REALY1,REALY_ON);
			relay_ctrl(REALY2,REALY_OFF);
			relay_ctrl(REALY3,REALY_OFF);
		}break;
		case MIDDLE: 
		{
			relay_ctrl(REALY1,REALY_OFF);
			relay_ctrl(REALY2,REALY_ON);
			relay_ctrl(REALY3,REALY_OFF);
		}break;
		case HIGH: 
		{
			relay_ctrl(REALY1,REALY_OFF);
			relay_ctrl(REALY2,REALY_OFF);
			relay_ctrl(REALY3,REALY_ON);
		}break;
		default : ;
	}
}
extern uint8_t auto_speed;
void auto_speed_display(void)
{
	display_signal(AUTO_REV);//显示自动图标
	switch(auto_speed)
	{
		case HIGH://高风
		{
			display_signal(LOW_REV);
			display_signal(MIDDLE_REV);    
			display_signal(HIGH_REV);
		} break;
		case MIDDLE://中风
		{
			display_signal(LOW_REV);
			display_signal(MIDDLE_REV);    
			clr_display_signal(HIGH_REV);
		}break;
		case LOW://低风
		{
			display_signal(LOW_REV);
			clr_display_signal(MIDDLE_REV);    
			clr_display_signal(HIGH_REV);
		}
		default : ;
	}
}
extern struct UART_Infor uart_infor[];
int uart_write(int chn, const void *in, int len)
{
    int i;
//	uint8_t c;
	uint8_t buffer[0x400];
	int ret = put_chn_bytes(&uart_infor[chn].tx_slot, (uint8_t *)in, len);
	PRINTF("\r\nsend data to UART5\r\n");
	uart_pop_tx(0, buffer, len);
	USART_WriteBlocking(USART5, buffer,len);
	for(i=0;i<len;i++)
	{
		PRINTF("%2X ",buffer[i]);
	}		
    return (ret);
}

void showTempHumi(void)
{
	si7020Measure(&si7020_data.temperature,&si7020_data.humidity);
	num_display_ctrl(1,((uint16_t)si7020_data.temperature) % 1000 / 100);
	num_display_ctrl(2,((uint16_t)si7020_data.temperature) % 100 / 10);
    num_display_ctrl(3,((uint16_t)si7020_data.temperature) % 10);
	
	num_display_ctrl(4,((uint16_t)si7020_data.humidity) % 1000 / 100);
    num_display_ctrl(5,((uint16_t)si7020_data.humidity) % 100 / 10);
	
	display_signal(DIP);//显示小数点
	display_signal(ROOM_TEMP);//显示室内温度图标
	display_signal(RH);//显示室内温度图标
	display_signal(C);//显示室内温度图标
}
static void sys_tick(void *args)
{
    static uint8_t _msec = 0, _sec = 0, tick_cnt = 0;

    if (deltms >= 20)
    {
        deltms -= 20;
        notify(EV_20MS);
        if (++tick_cnt < 5)
            return;
        tick_cnt = 0;
        notify(EV_100MS);
        if (++_msec < 10)
            return;
        _msec = 0;
        notify(EV_SEC);
        if (++_sec < 60)
            return;
        _sec = 0;
        notify(EV_MIN);
    }
}
static void task_20ms(void *args)
{
	//printf("task_20ms test \r\n");
//	HAL_GPIO_TogglePin(CTRL1_GPIO_Port,CTRL1_Pin);
	//memset(lcd_buf, 0xFF, LCD_BUF_SIZE);
//	num_display_ctrl(1,2);
	key_scan();
	buffer2buff(lcd_buf,lcd_buffer,7);
	lcdReload();
}
void beep_ctrl(void)
{
	if(cap_key.key_chn>0)//关闭蜂鸣器
    {
        cap_key.key_chn--;
        if(cap_key.key_chn==0)
        {
            beep_off();
        }
    }
}
void backlight_ctrl(void)
{
	if(temperature_c.backlight>0)//背光点亮时间控制
    {
        temperature_c.backlight--;
        if(temperature_c.backlight)
			backligt_on();
		else
			backlight_off();
    }
}
void plc_communication_handle()
{
  if(plc_alive_flag==1)
  {
    if(plc_communication==true)
    {
      if(plc_alive_cntr_ms<5)
      {
        clr_display_signal(PLC);
      }
      else 
      {
        display_signal(PLC);
        plc_alive_cntr_ms=0;
        plc_communication=false;//清除标志
      }
    }
  }
}
void check_alive0()
{
    uint8_t tmp[2] = {0};
    uint8_t ret=0;
    tmp[0] = CMD_SET_REG;
    tmp[1] = reg.type | reg.last_status;
    reg.last_status = 0;
    ret=code_local_frame(tmp, sizeof(tmp), g_frame_buffer, sizeof(g_frame_buffer));//组报文
    uart_write( 0, g_frame_buffer, ret);//发送报文 
//    plc_communication=true;//发送数据指示
}
// 检测PLC是否在线
void check_alive()
{
  static uint32_t counter;
//  counter++;
  if(counter++>=60)//一分钟检测一次
  {
    check_alive0();
    if(plc_alive_flag==1)
    {
      display_signal(PLC);//显示图标
      //log_d(MODULE_APP, "PLC is right?????\n");
      plc_alive_flag=0;
		PRINTF("PLC ALIVE\r\n");
    }
    else
    {
      clr_display_signal(PLC);//关闭图标
		PRINTF("PLC IS NOT ALIVE\r\n");
   //   log_d(MODULE_APP, "PLC is ERR!!!\n");
    }
    counter=0;
  }
    
}
static void task_100ms(void *args)
{
//	led_toggle(0);
	buffer2buff(lcd_buf,lcd_buffer,7);
	update_lcd_data();
	lcdReload();
	temp_display_flash();//设置温度闪烁
	beep_ctrl();
	backlight_ctrl();
	notify(EV_STATE);
	uart_tick_hook();
//	plc_communication_handle();
}
extern void read_si7020_data(void);
static void task_sec(void *args)
{
//	uint8_t ret,i;
//	uint8_t flash_buf[8];
//	uint8_t flash_wirte[]="abcdefg";
//	showTempHumi();
	if(temperature_c.ctrl_delay>0)
		temperature_c.ctrl_delay--;
	read_si7020_data();
////////////	si7020Measure(&si7020_data.temperature,&si7020_data.humidity);

	
////////	sensor.temp=si7020_data.temperature;
////////	sensor.humi=si7020_data.humidity;
//	ret = spiflash_init();
//	if(ret == 1)
//	{
//		realy_on(0);
//	}
//	else
//	{
//		realy_toggle(1);
//	}
//	mx25r_cmd_write(&mx25r,0x00000f00U,flash_wirte,sizeof(flash_wirte));
//	mx25r_cmd_read(&mx25r,0x00000f00U,flash_buf,0x08);
	temp_ctrl_realy();
	plc_state.wait_t++; //状态机用
	check_alive();//PLC是否在线检测
	auto_report_sec_task();
	if(dev.temp_param.panel_lock==ON)//面板锁定指示。
      flash_struct.lock_start_flag=true;
    else if(dev.temp_param.panel_lock==OFF)
      flash_struct.lock_start_flag=false;
//	uart_send_test();
//	for(i=0;i<8;i++)
//	PRINTF("flash_buf=%c\r\n",flash_buf[i]);
//	showAll();
}
static void task_key(void *args)
{
	
}
extern volatile bool txOnGoing;
extern volatile bool rxOnGoing;
extern uint8_t g_rxBuffer[];
static void on_uart_rxchar(void *args)
{
	struct SmartFrame *pframe;
    uint8_t idx, len;
	uint8_t i;
    idx    = uart_peek(0, g_frame_buffer, sizeof(g_frame_buffer));
    pframe = get_smart_frame(g_frame_buffer, idx);
    if (!pframe)
        return;
	idx = (uint8_t *)pframe - g_frame_buffer;
    len = pframe->len + SMART_FRAME_HEAD + 1;
	PRINTF ("\r\nget smart frame:\r\n");
	for(i=0;i<len;i++)
	{
		PRINTF("%02X ",*(g_frame_buffer+i));
	}
    clear_uart(0, idx + len);
    memmove(&g_frame_buffer[0], &g_frame_buffer[idx], len);
    pframe = (struct SmartFrame *)g_frame_buffer;
	
//	sensor_infor.have_communication = 1;
//	sensor_infor.net_blink_cnt = 5;
//    //增加2小时无通讯复位载波芯片功能
//    clear_rst_time(pframe);
    plc_machine_opt(pframe);
}
static void state_machine(void *args)
{
	plc_machine_opt(NULL);
}
struct task tasks[] =
{
    { EV_PLC,    0,            NULL, on_uart_rxchar  },
    { EV_TICK,   0,            NULL, sys_tick        },
    { EV_20MS,   0,            NULL, task_20ms       },
    { EV_100MS,  0,            NULL, task_100ms      },
    { EV_SEC,    0,            NULL, task_sec        },
    { EV_KEY,    0,            NULL, task_key        },
    { EV_STATE,  0,            NULL, state_machine   },
};

void task_handle(void)
{
    uint8_t i;

    for (i = 0; i < array_size(tasks); ++i)
    { 
        struct task *t = &tasks[i];

        if (is_task_set(t->id) || is_task_always_alive(t->flags))
        {
            reset_task(t->id);
            t->handle(t->args);
        }
    }
}
void app_init(void)
{
	temperature_c.backlight=BL_on_time;//背光点亮一段时间
}
