#ifndef	_PROTOCOL_H_
#define	_PROTOCOL_H_

#include "smart_plc.h"

//通道
#define PWRON_PSR   50
#define CHN_1       0x01
#define CHN_2       0x02
#define ON       0x01
#define OFF      0x00

//上电继电器状态
#define OPEN_STATUS  0x00//继电器常开
#define CLOSE_STATUS 0x01
#define LAST_STATUS  0x02

extern uint8 act_delay_flag;


//PLC地址
struct EEP_PARAM
{
    uint8_t   panid[PANID_LEN];   //网关AID
    uint8_t   panid_flag;         //0x05 is valid , or is 0x0A
    uint8_t   password[PWD_LEN];
    uint8_t   pwd_magic;          //0x05 is valid , or is 0x0A
    uint8_t   id[ID_LEN];
    uint8_t   gateway_id[ID_LEN];
//  uint8   bps;                //0:2400, 2:9600
    uint8_t   sid[SID_LEN];      //网关分配，广播报文,2B
    uint8_t   update[2];
};

//功能20151026
struct EEP_FUNCTION
{
    uint8   relay_mode;//继电器状态
	  uint8   report_enable;//状态同步
    uint8   close_delay_time1;
    uint8   open_delay_time1;
    uint8   close_delay_time2_1;
    uint8   open_delay_time2_1; 
    uint8   close_delay_time2_2;
    uint8   open_delay_time2_2; 
    uint8   write_time_flag;
};

struct func_ops
{
    uint8	di[2];                       //数据标志
	  uint8	(*read)(uint8 *buff, uint8 max_len);//读功能执行函数
    uint8	(*write)(uint8 *buff, uint8 w_len);//写功能执行函数
};

struct FBD_Frame
{
    uint8 did[2];
    uint8 ctrl;
    uint8 data[1];
};
#define FBD_FRAME_HEAD   offset_of(struct FBD_Frame, data)

struct EVENT_INFOR
{
   uint8 type   :6; 
   uint8 report :1;
   uint8 invalid:1;
};

struct RAM
{
    uint8 relay_stat;//继电器状态
#if DIMMER|COLOR_DIMMER
    uint8   target_percent;
    uint8   relay_state;
#endif
#if COLOR_DIMMER
    uint8   rgb_percent[3];
    uint8   target_rgb_percent[3];
    uint8   color_change_flag;
#endif
#if DIMMER
    uint8   percent;  //just for illuminate dimmer
#endif
    uint8   relay_act_flag;//每一位代表相应继电器是否操作 
    uint8   relay_state_bak;//状态同步，继电器状态是否变化
	  
		uint8   switch_last_stat;//20151210
    uint8   switch_stat;
    uint8   nodzp_cnt;
    uint8   dzp_cnt;
};

struct SHS_frame
{
    uint8 stc;
    uint8 said[ADDRESS_LEN];
    uint8 taid[ADDRESS_LEN];
    uint8 seq;
    uint8 length;
    uint8 infor[1];
};

#define SHS_FRAME_HEAD       offset_of(struct SHS_frame, infor)
extern const struct func_ops func_items[];
extern struct RAM  ram;

#if DIMMER
void dimmer_100ms_hook(void);
#endif
#if COLOR_DIMMER 
void clour_dimmer_hook(void);
#endif

struct SHS_frame *get_smart_frame(uint8 rxframe_raw[],uint8 rxlen);
void _get_dev_type(uint8* buff);
void save_relay_status(void);
uint8 _get_dev_soft_ver(uint8* buff);
uint8 set_group_parameter(uint8 data[], uint8 len);
uint8 set_parameter(uint8 data[], uint8 len);
uint8 read_parameter(uint8 data[], uint8 len);
uint8 is_gid_equal(uint8 data[]);
//uint8 read_group_parameter(uint8 data[], uint8 len);
uint8 compare_soft_ver(uint8 *buff, uint8 len);
#endif
