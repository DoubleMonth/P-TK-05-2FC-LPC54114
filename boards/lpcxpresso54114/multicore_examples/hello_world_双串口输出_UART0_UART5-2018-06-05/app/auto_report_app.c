//#include "headfiles.h"
#include "protocol_smart.h"
#include "auto_report_app.h"
#include "device.h"
//#include "dev.h"
//#include "app.h"
//#include "printk.h"
#include "app_dev_ctrl.h"
#include <stdlib.h>
#include "fsl_debug_console.h"
#include "app_si70xx.h"
#include "app_lcd.h"
extern uint8_t plc_communication;
extern struct dev dev;
extern struct sensor sensor;
extern uint8_t g_frame_buffer[0x400];
/* please redefine send function */
int uart_write_report(const void *in,int len)
{
    extern device_t serial;

    PRINTF( "auto report send report frame!\n");

    plc_communication=true;//发送数据指示
    return(uart_write(0, in, len));
}
/* some sensor need report type infor, if you don't use it ,please delete */
int code_intype_body(uint16_t did, int err, const void *data, int len, void *out, int maxlen ,uint8_t type)
{
    int ret = 0;
    uint8_t buf[0x10];
    
    buf[0] = type;
    memcpy(&buf[1], data, len);

    ret = code_body(did, 0, buf, len + 1, out, maxlen);
    return ret;
}

//--------------------------------------------------------------------------------------
//开关机+模式+风速+面板锁定+低温保护
//
#if MAX_ACTOR_NUM
extern u16_relay_data_t u16_relay_data[1];//U16数据类型，为设置温度用
extern uint16_t u16_relay_data_bak[1];
void init_relay_data(void)
{
	uint8_t i = 0;
    relay_data_bak[i] = dev.temp_param.ventilate_speed;             //val      // 通风模式下风速
    relay_data[i++].cur = &dev.temp_param.ventilate_speed;          //pointer
    relay_data_bak[i] = dev.temp_param.win_speed;                   //val      // 风速
    relay_data[i++].cur = &dev.temp_param.win_speed;                //pointer
    relay_data_bak[i] = dev.temp_param.air_coner_switch;            //val  //开关机
    relay_data[i++].cur = &dev.temp_param.air_coner_switch;         //pointer
    relay_data_bak[i] = dev.temp_param.mode;                        //val     //模式
    relay_data[i++].cur = &dev.temp_param.mode;                     //pointer
    relay_data_bak[i] = dev.temp_param.panel_lock;                  //val     //面板锁定
    relay_data[i++].cur = &dev.temp_param.panel_lock;               //pointer
    relay_data_bak[i] = dev.temp_param.low_temp_protect;            //val//温度保护 
    relay_data[i++].cur = &dev.temp_param.low_temp_protect;         //pointer
    u16_relay_data_bak[0]=dev.temp_param.set_temp;                  //val     //设置温度
    u16_relay_data[0].cur = &dev.temp_param.set_temp;               //pointer
    
}
#endif

#if MAX_ALARM_NUM
extern uint8_t low_temp_flag;//低温保护动作标志。
extern struct FLASH_STRUCT flash_struct;
void init_alarm_data(void)
{
	uint8_t i = 0;
    alarm_data_bak[i] = low_temp_flag;                                  //val 低温保护动作标志。
    alarm_data[i++].cur = &low_temp_flag;                               //pointer
//    alarm_data_bak[i] = flash_struct.breakdown_start_flag;              //val  温湿度传感器故障标志
//    alarm_data[i++].cur = &flash_struct.breakdown_start_flag;           //pointer
}
#endif
  
#if MAX_SENSOR_NUM
void init_sensor_data(void)//初始化传感器上报参数
{
    uint8_t i = 0;
    sensor_data[i].sensor_window = TEMP_WINDOW_BIN;
    sensor_data[i].sensor_cur = (int32_t)sensor.temp;
    sensor_data[i].sensor_freq = dev.temp_report_freq;
    sensor_data[i].sensor_step = dev.temp_report_step;
    sensor_data[i++].sensor_type = TEMP;
    
    sensor_data[i].sensor_window =HUMI_WINDOW_BIN;
    sensor_data[i].sensor_cur = (int32_t)sensor.humi;
    sensor_data[i].sensor_freq = dev.humi_report_freq;
    sensor_data[i].sensor_step = dev.humi_report_step;
    sensor_data[i++].sensor_type = HUMI; 
}
#endif

//--------------------------------------------------------------------------------------
extern uint8_t key_pressed;
void get_report_data_callback(report_type_t r_type)
{
    uint8_t len = 0;
    clear_chn(r_type);
#if MAX_SENSOR_NUM
    uint8_t buf[2] = {0x00,0x00};
    if((POWER_ON == r_type) || (REGISTER == r_type) || (FREQUENCY == r_type) || (FIXED_LENGTH == r_type))
    {
        hex2bcd(abs(sensor.temp), buf, TEMP_LEN);
        sensor.temp < 0 ?  set_bit(buf[1], 7) : clr_bit(buf[1], 7);
		len += code_intype_body(0xB691,0,buf,TEMP_LEN,g_frame_buffer+len,sizeof(g_frame_buffer), TEMP);
        hex2bcd(sensor.humi, buf, HUMI_LEN);
		len += code_intype_body(0xB691,0,buf,HUMI_LEN,g_frame_buffer+len,sizeof(g_frame_buffer),HUMI);
        hex2bcd(abs(sensor.temp), buf, TEMP_LEN);
        sensor.temp < 0 ?  set_bit(buf[1], 7) : clr_bit(buf[1], 7);
		len += code_intype_body(0xB701,0,buf,TEMP_LEN,g_frame_buffer+len,sizeof(g_frame_buffer), TEMP);
		hex2bcd(sensor.humi, buf, HUMI_LEN);
		len += code_intype_body(0xB701,0,buf,HUMI_LEN,g_frame_buffer+len,sizeof(g_frame_buffer),HUMI);
    }
#endif
	
#if MAX_ALARM_NUM
    if((POWER_ON == r_type) || (REGISTER == r_type) || (ALARM == r_type))
    {
		len += code_intype_body(0xD105,0,&low_temp_flag,1,g_frame_buffer+len,sizeof(g_frame_buffer),LOW_PROT_TEMP_ALARM);
		len += code_intype_body(0xD105,0,&flash_struct.breakdown_start_flag,1,g_frame_buffer+len,sizeof(g_frame_buffer),TEMP_HUMI_ALARM);
    }
#endif
	
#if MAX_ACTOR_NUM
    if((POWER_ON == r_type) || (REGISTER == r_type) || (STATE_CHANGE == r_type))
    {
      len+=code_body(0xE013,0,&dev.temp_param.air_coner_switch,1,g_frame_buffer+len,sizeof(g_frame_buffer));//开关机
      len+=code_body(0xE012,0,&dev.temp_param.mode,1,g_frame_buffer+len,sizeof(g_frame_buffer));//模式
      if(dev.temp_param.mode==VENTILATE)
        len+=code_body(0xE011,0,&dev.temp_param.ventilate_speed,1,g_frame_buffer+len,sizeof(g_frame_buffer));//通风模式下的风速
      else if (dev.temp_param.mode==HEAT||dev.temp_param.mode==COOL)
        len+=code_body(0xE011,0,&dev.temp_param.win_speed,1,g_frame_buffer+len,sizeof(g_frame_buffer));//风速
      len+=code_body(0xE014,0,&dev.temp_param.panel_lock,1,g_frame_buffer+len,sizeof(g_frame_buffer));//面板锁定
      len+=code_body(0xE010,0,&dev.temp_param.low_temp_protect,1,g_frame_buffer+len,sizeof(g_frame_buffer));//低温保护
      hex2bcd(dev.temp_param.set_temp,buf,2);//转为BCD
      len+=code_body(0xE004,0,buf,2,g_frame_buffer+len,sizeof(g_frame_buffer));//设置温度
      hex2bcd(dev.temp_param.set_temp/10,buf,1);//转为BCD
      len+=code_body(0xE002,0,buf,1,g_frame_buffer+len,sizeof(g_frame_buffer));//设置温度
      if((POWER_ON == r_type) || (REGISTER == r_type))
          memcpy(judge_data.taker_id,dev.encode.id,ID_LEN);//保证上电上报和注册上报时使用的是自身的AID
      len += code_body(0xC01A,0,judge_data.taker_id,ID_LEN,g_frame_buffer+len,sizeof(g_frame_buffer));//
    }
#endif
	len = put_chn(r_type,g_frame_buffer,len);

    PRINTF("Report type: %d,put data len:%d\n",r_type,len);

}


//--------------------------------------------------------------------------------------
#if MAX_SENSOR_NUM
static uint8_t get_sensor_type(struct Body* fbd)
{
    /* B701 ACK, get sensor type */
    if ((fbd->did[0] == 0x01) && (fbd->did[1] == 0xB7))
    {
        return(fbd->data[0]);
    }
	/* please add you did and sensor type */
    return UNKNOWN;
}
#endif

void report_finish_refresh_infor(uint8_t *data, uint8_t len)
{
    #if MAX_SENSOR_NUM
    uint8_t i,type;
    int8_t j = -1;
    struct Body *body;
    for (i = 0; i < len; i += FBD_FRAME_HEAD)
    {
        body = (struct Body*)&data[i];
        type = get_sensor_type(body); 
        switch (type)
        {
            
            case TEMP:
                j = get_sensor_from_type(TEMP);
                if (j >= 0)
                {
                    sensor_data[j].sensor_base = (int32_t)bcd2hex(&body->data[1],TEMP_LEN);
                    sensor_data[j].last_report = sensor_data[j].sensor_base;
                }
                break;
            case HUMI:
                j = get_sensor_from_type(HUMI);
                if (j >= 0)
                {
                   sensor_data[j].sensor_base = (int32_t)bcd2hex(&body->data[1],HUMI_LEN);
                   sensor_data[j].last_report = sensor_data[j].sensor_base;
                }
                
                break;
            default:
                break;
        }
        i += body->ctrl;
    }
    #endif
}
