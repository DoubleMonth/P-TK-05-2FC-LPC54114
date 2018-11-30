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
static uint8_t temp_ctrl_flag_cool=0;//�¶Ȼز��־
static uint8_t temp_ctrl_flag_heat=0;//�¶Ȼز��־
extern struct CAP_KEY cap_key;
extern struct sensor sensor;
uint8_t low_temp_flag=0;//���±����ز��־��
uint8_t g_frame_buffer[0x400];
uint8_t plc_alive_flag;
 uint8_t plc_alive_cntr_ms ;//PLC��˸��ʱ 
uint8_t plc_communication;//PLCͨ�ű�־
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
	dev.temp_param.air_coner_switch=ON;                //�ػ�
	dev.temp_param.backlight_enable=MY_ENABLE;          //����ʹ��
	dev.temp_param.low_temp_protect=MY_ENABLE;          //���±���
	dev.temp_param.mode=VENTILATE;                      //ģʽ--ͨ��ģʽ
	dev.temp_param.panel_lock=OFF;                      //���������
	dev.temp_param.set_temp=200;                        //Ĭ�������¶�20.0��
	dev.temp_param.state_report=MY_ENABLE;              //״̬�ϱ���
	dev.temp_param.win_speed=AUTO;                      //����--�Զ�
    dev.temp_param.ventilate_speed=LOW;                 //ͨ��ģʽ�µķ���Ĭ��Ϊ�ͷ硣
	dev.temp_param.max_cool_temp=300;                   //Ĭ����������¶�30��
	dev.temp_param.max_heat_temp=300;                   //Ĭ����������¶�30��
	dev.temp_param.mix_cool_temp=160;                   //Ĭ����С�����¶�16��
	dev.temp_param.mix_heat_temp=160;                   //Ĭ����С�����¶�16��
	dev.temp_param.max_set_temp=300;                    //��������¶�30��
	dev.temp_param.mix_set_temp=50;                     ///��С�����¶�5��
	dev.temp_param.limit_t=10;                          //�ز��¶�1��
    
    dev.power_on_delay = 60;                //��������Чʱ��
    dev.temp_report_step = 5;              //Ĭ�ϲ���Ϊ1��
    dev.temp_report_freq=0X0A;                 //Ĭ�Ϲرն�Ƶ�ϱ�
    dev.humi_report_step = 20;              //Ĭ�ϲ���Ϊ5%
    dev.humi_report_freq=0X0A;                 //Ĭ�Ϲرն�Ƶ�ϱ�
    dev.humi_reduction = 30;                //ʪ��Ĭ�ϼ�3%
    dev.temp_reduction = -15;               //�¶�Ĭ�ϼ�1.5��
    dev.active_report=0x01;                 //�����ϱ����غ��豸
    dev.auto_speed_ctrl_mode=0x01;          //Ĭ���Զ���ʱ�رշ����
	
	
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
		dev.head_magic=HEAD_MAGIC_NUM;          //��ͷMAGIC
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
	
  if(dev.temp_param.mode==VENTILATE)//ͨ��ģʽ��
  {
    dev.temp_param.ventilate_speed++;
    if(dev.temp_param.ventilate_speed>=3)
    dev.temp_param.ventilate_speed=0;
  }
  else	    //���������ģʽ��
  {
    dev.temp_param.win_speed++;
    if(dev.temp_param.win_speed>=4)
      dev.temp_param.win_speed=0;
  }
	
	flash_struct.set_start_flag=false;//�ر������¶���˸
	save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
//    write_param_to_flash();//д��Flash
	
}
void add_temp_handle(void)
{
	dev.temp_param.set_temp +=5;
	if(dev.temp_param.mode==HEAT||dev.temp_param.mode==VENTILATE)//����ģʽ��ͨ��ģʽʱ  Ĭ�������¶�
	{
		if(dev.temp_param.set_temp>=dev.temp_param.max_heat_temp)
		dev.temp_param.set_temp=dev.temp_param.max_heat_temp;//����¶�
	}
	else if(dev.temp_param.mode==COOL)//����ģʽ
	{
		if(dev.temp_param.set_temp>=dev.temp_param.max_cool_temp)
		dev.temp_param.set_temp=dev.temp_param.max_cool_temp;//����¶�
	}
	flash_struct.set_start_flag=true;
	flash_struct.ms_counter=0;
	flash_struct.s_counter=0;
	save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
//    write_param_to_flash();//д��Flash
}

extern void flash_num_display(uint8_t position, uint8_t number,uint8_t fre, uint8_t time);
void dec_temp_handle(void)
{
	dev.temp_param.set_temp  -=5;//�¶�--;
	
		if(dev.temp_param.mode==HEAT||dev.temp_param.mode==VENTILATE)//����ģʽ��ͨ��ģʽʱ  Ĭ�������¶�
	{
		if(dev.temp_param.set_temp<=dev.temp_param.mix_heat_temp)
		dev.temp_param.set_temp=dev.temp_param.mix_heat_temp;//����¶�
	}
	else if(dev.temp_param.mode==COOL)//����ģʽ
	{
		if(dev.temp_param.set_temp<=dev.temp_param.mix_cool_temp)
		dev.temp_param.set_temp=dev.temp_param.mix_cool_temp;//����¶�
	}
	flash_struct.set_start_flag=true;
	flash_struct.ms_counter=0;
	flash_struct.s_counter=0;
	save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
}
void mode_handle(void)
{
	dev.temp_param.mode++;//ģʽ;
	if(dev.temp_param.mode>=3)
		dev.temp_param.mode=0;
	flash_struct.set_start_flag=false;//�ر������¶���˸
	save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();

}
void on_off_handle(void)
{
	dev.temp_param.air_coner_switch ++;//���ػ�++;
	if(2==dev.temp_param.air_coner_switch )
		dev.temp_param.air_coner_switch =OFF;
	flash_struct.set_start_flag=false;//�ر������¶���˸
	save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־ ��Ϊ��Ч

	flash_struct.set_start_flag=false;//�ر������¶���˸
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
����ģʽ�£�
�趨�¶ȱȵ�ǰ�¶ȵ�3��ʱ���߷�
�趨�¶ȱȵ�ǰ�¶ȵ�1-3��ʱ���з�
�趨�¶ȱȵ�ǰ�¶ȵ�1��ʱ���ͷ�
����ģʽ�£�
�趨�¶ȱȵ�ǰ�¶ȸ�3��ʱ���߷�
�趨�¶ȱȵ�ǰ�¶ȸ�1-3��ʱ���з�
�趨�¶ȱȵ�ǰ�¶ȸ�1��ʱ���ͷ�
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
            if(temp>dev.temp_param.set_temp+30)//�߷�
            {
                auto_speed=HIGH;
                speed_realy_ctrl(HIGH);
                relay_ctrl(REALY4,REALY_ON);
            }
            else if((temp>=dev.temp_param.set_temp+10)&&(temp<=dev.temp_param.set_temp+30))//�з�
            {
                auto_speed=MIDDLE;
                speed_realy_ctrl(MIDDLE);
                relay_ctrl(REALY4,REALY_ON);
            }
            else if(temp<=dev.temp_param.set_temp+10)//�ͷ�
            {
                auto_speed=LOW;
                speed_realy_ctrl(LOW);
                relay_ctrl(REALY4,REALY_ON);
            }
		}break;
		case HEAT:
		{
            if(temp<dev.temp_param.set_temp-30)//�߷�
            {
                auto_speed=HIGH;
                speed_realy_ctrl(HIGH);
                relay_ctrl(REALY4,REALY_ON);
            }
            else if((temp<=dev.temp_param.set_temp-10)&&(temp>=dev.temp_param.set_temp-30))//�з�
            {
                auto_speed=MIDDLE;
                speed_realy_ctrl(MIDDLE);
                relay_ctrl(REALY4,REALY_ON);
            }
            else if(temp>=dev.temp_param.set_temp-10)//�ͷ�
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
	if(temperature_c.ctrl_delay>0)//��ֹ�豸���ϵ磬���������ݻ�û�ж���ʱ�̵������������������澯ʱҲ�����п���
    {
        relay_ctrl(REALY4,REALY_OFF);//�رշ���
		speed_realy_ctrl(0XFF);//�رշ��
        return;
    }
    if(dev.temp_param.air_coner_switch!=ON)//�ػ�
	{

        if(dev.temp_param.low_temp_protect!=MY_ENABLE)//���±�����ʱ���رշ��źͷ������ֹ�ڵ��±������ٹرյ��±�������ʱ�����źͷ�����ڿ���״̬��
        {
            relay_ctrl(REALY4,REALY_OFF);//�رշ���
			speed_realy_ctrl(0XFF);//�رշ��
            low_temp_flag=0;//���±���������־���
            return;
        }
		else if(temperature_c.cur_temp<=mix_low_temp_protect)//���±�������
		{
			low_temp_flag=1;//���±����ز��־��
			relay_ctrl(REALY4,REALY_ON);//�򿪷���
			speed_realy_ctrl(MIDDLE);//�з�
		}
		else if(low_temp_flag==1&&temperature_c.cur_temp>=max_low_temp_protect)
		{
			relay_ctrl(REALY4,REALY_OFF);//�رշ���
			speed_realy_ctrl(0XFF);//�رշ��
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
		low_temp_flag=0;//������±����ز��־
		if(dev.temp_param.mode==COOL)//����ģʽ��
		{
            temp_ctrl_flag_heat=0;
            if(temperature_c.cur_temp<=dev.temp_param.set_temp-10)
			{
                if(dev.temp_param.win_speed==AUTO)//�Զ���ģʽ��,Ĭ�ϲ��رշ����ѡ��ͷ�
                {
                    if(dev.auto_speed_ctrl_mode==false)//Ĭ�ϲ��رշ����ѡ��ͷ�
                    {
                        speed_realy_ctrl(LOW);
                        auto_speed=LOW;
                    }
                    else if(dev.auto_speed_ctrl_mode==true)//����Ϊ�رշ��ʱ���رշ��
                    {
                        speed_realy_ctrl(0XFF);
                        auto_speed=LOW;
                    }
                }
                else//�ֶ�ģʽʱ�رշ����
                {
                    speed_realy_ctrl(0XFF);
                }
				relay_ctrl(REALY4,REALY_OFF);//�رշ���
				temp_ctrl_flag_cool=0;	

				return;
                
			} 
            else if(temperature_c.cur_temp<dev.temp_param.set_temp+10)
            {
                if(temp_ctrl_flag_cool==0)
                {
                    if(dev.temp_param.win_speed==AUTO)//�Զ���ģʽ��,Ĭ�ϲ��رշ����ѡ��ͷ�
                    {
                        if(dev.auto_speed_ctrl_mode==false)//Ĭ�ϲ��رշ����ѡ��ͷ�
                        {
                            speed_realy_ctrl(LOW);
                            auto_speed=LOW;
                        }
                        else if(dev.auto_speed_ctrl_mode==true)//����Ϊ�رշ��ʱ���رշ��
                        {
                            speed_realy_ctrl(0XFF);
                            auto_speed=LOW;
                        }
                    }
                    else//�ֶ�ģʽʱ�رշ����
                    {
                        speed_realy_ctrl(0XFF);
                    }
                    relay_ctrl(REALY4,REALY_OFF);//�رշ���
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
				if(dev.temp_param.win_speed==AUTO)//�Զ���ģʽ��,Ĭ�ϲ��رշ����ѡ��ͷ�
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
                else//�ֶ�ģʽʱ�رշ����
                {
                  speed_realy_ctrl(0XFF);
                }
                relay_ctrl(REALY4,REALY_OFF);//�رշ���
                temp_ctrl_flag_heat=0;
             
                return;
            }
            else if (temperature_c.cur_temp>dev.temp_param.set_temp-10)
            {
                if(temp_ctrl_flag_heat==0)
                {
                    if(dev.temp_param.win_speed==AUTO)//�Զ���ģʽ��,Ĭ�ϲ��رշ����ѡ��ͷ�
                    {
                        if(dev.auto_speed_ctrl_mode==false)//Ĭ�ϲ��رշ����ѡ��ͷ�
                        {
                            speed_realy_ctrl(LOW);
                            auto_speed=LOW;
                        }
                        else if(dev.auto_speed_ctrl_mode==true)//����Ϊ�رշ��ʱ���رշ��
                        {
                            speed_realy_ctrl(0XFF);
                            auto_speed=LOW;
                        }
                    }
                    else//�ֶ�ģʽʱ�رշ����
                    {
                        speed_realy_ctrl(0XFF);
                    }
                    relay_ctrl(REALY4,REALY_OFF);//�رշ���
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
		else if(dev.temp_param.mode==VENTILATE)//ͨ��ģʽ�¹رշ��ţ������趨���ٿ��Ʒ��
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
	display_signal(AUTO_REV);//��ʾ�Զ�ͼ��
	switch(auto_speed)
	{
		case HIGH://�߷�
		{
			display_signal(LOW_REV);
			display_signal(MIDDLE_REV);    
			display_signal(HIGH_REV);
		} break;
		case MIDDLE://�з�
		{
			display_signal(LOW_REV);
			display_signal(MIDDLE_REV);    
			clr_display_signal(HIGH_REV);
		}break;
		case LOW://�ͷ�
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
	
	display_signal(DIP);//��ʾС����
	display_signal(ROOM_TEMP);//��ʾ�����¶�ͼ��
	display_signal(RH);//��ʾ�����¶�ͼ��
	display_signal(C);//��ʾ�����¶�ͼ��
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
	if(cap_key.key_chn>0)//�رշ�����
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
	if(temperature_c.backlight>0)//�������ʱ�����
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
        plc_communication=false;//�����־
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
    ret=code_local_frame(tmp, sizeof(tmp), g_frame_buffer, sizeof(g_frame_buffer));//�鱨��
    uart_write( 0, g_frame_buffer, ret);//���ͱ��� 
//    plc_communication=true;//��������ָʾ
}
// ���PLC�Ƿ�����
void check_alive()
{
  static uint32_t counter;
//  counter++;
  if(counter++>=60)//һ���Ӽ��һ��
  {
    check_alive0();
    if(plc_alive_flag==1)
    {
      display_signal(PLC);//��ʾͼ��
      //log_d(MODULE_APP, "PLC is right?????\n");
      plc_alive_flag=0;
		PRINTF("PLC ALIVE\r\n");
    }
    else
    {
      clr_display_signal(PLC);//�ر�ͼ��
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
	temp_display_flash();//�����¶���˸
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
	plc_state.wait_t++; //״̬����
	check_alive();//PLC�Ƿ����߼��
	auto_report_sec_task();
	if(dev.temp_param.panel_lock==ON)//�������ָʾ��
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
//    //����2Сʱ��ͨѶ��λ�ز�оƬ����
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
	temperature_c.backlight=BL_on_time;//�������һ��ʱ��
}
