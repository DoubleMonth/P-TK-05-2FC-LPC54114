#ifndef __HAL_LCD_H_
#define __HAL_LCD_H_
#include <stdio.h>
#include <stdint.h>
#define LCD_BUF_SIZE        7
#define BU9796_ADDR         0x3E
#define SET_LOAD_DATA_ADDR  0x00            //数据装载起始地址0x00
#define SET_SOFT_RESET      0xEA            //ICSET
#define SET_IC              0xE8
#define SET_BLINK_OFF       0xF0            //BLINK
#define SET_BLINK_ON        0xF1            //BLINK 0.5Hz
#define SET_AP_OFF          0xFD            //ALL PIX
#define SET_AP_NORMAL       0xFC
#define SET_SHOW_ON         0xC8            //MODE SET
#define SET_SHOW_OFF        0xC0            //MODE SET
#define SET_DISPLAY         0xA2            //DISCTRL
enum
{
	/*模式*/
    COOL_MODE = 1,
    HEAT_MODE,
    VENTILATE_MODE,

/*风速*/
    LOW_REV,   
    MIDDLE_REV,
    HIGH_REV,
    AUTO_REV,  

/*定时开关*/
    TIMER_ON,
    TIMER_OFF,

/*夜间模式*/
    NIGHTTIME,

/*温度*/
    ROOM_TEMP,// 室内温度图标
    _SET_TEMP,//温度设置图标

/*时间*/
//    AM,
//    PM,

/*其他*/
//    COL,
//    R_CENTIGRADE,
//    S_CENTIGRADE,
//    SCALE,
//    H,
			PLC,//载波图标
			VALVE,//阀门图标
			BREAKDOWN,//故障图标
			LOCK,//按键锁定图标
		//	TM,
			//SET,
			RH,//湿度图标
			C,//温度图标
			SPEED,//风速图标
			DIP,//小数点图标
            MINUS_DECADE,//负号
            MINUS_UNIT,//负号
            MINUS_SMALL,//负号
            E,
            R1,//R1
            R2,
            O,
            R3,

/**/
//    RT,
//    TH,
//    TM,
//    ST,
};
enum//图标分组
{
	NO_GRP,
	SET_GRP,
	POW_ON_GRP,
	SPEED_GRP,
	MODE_GRP,
	ALL_GRP,
    OTHER,
    ERR_OR,
};
struct LCD_SHOW
{
    unsigned char sig;
    unsigned char idx;
};


struct LCD_DIGIT
{
    unsigned char num;
    unsigned char val;
};

struct LCD_ALPHA
{
    unsigned char alpha;
    unsigned char val;
};

struct LCD_SIGN
{
    unsigned char sig;
    unsigned char group;
    unsigned char idx;
    unsigned char val;
};

struct DISP_AP
{
    unsigned char A_array[8];
    unsigned char P_array[4];

    unsigned char ap_show;
    unsigned char ap_show_order;
	
    unsigned char GW_array[8];
    unsigned char PANID_array[4];
    unsigned char SID_array[4];

    unsigned char gps_show;
    unsigned char gps_show_order;
	
    unsigned char ntc_show;
    unsigned char ntc_show_time;
	unsigned char is_ntc_connect;
	
	unsigned char dew_show;
    unsigned char dew_show_time;
};

struct FLASH_STRUCT
{
	uint8_t set_start_flag;
	uint8_t plc_alive_flag;
	uint8_t lock_start_flag;
	uint8_t breakdown_start_flag;//温湿度传感器故障标志
	uint8_t ms_counter;//100ms延时计数，多少秒闪烁一次
	uint8_t fre;
	uint8_t s_counter;//1s延时计数，闪烁多少秒
};

uint8_t bu9796WriteByte(uint8_t add,uint8_t *buf,uint8_t len);
void displayOn(void);
void displayOff(void);
void bu9796SoftInit(void);
void showAll(void);
void clrAll(void);
void buffer2buff(uint8_t *dest,uint8_t *source,uint8_t len);
void display_fix_signal(void);
void display_signal(uint8_t sig);
void clr_display_signal(uint8_t sig);
void display_group_sig(uint8_t group, uint8_t sig);
void clr_display_group_sig(uint8_t group, uint8_t sig);
void num_display_ctrl(uint8_t position, uint16_t number);
void num_display(uint32_t number,uint8_t n);
void clr_num_display(uint8_t position);
void clr_position_num(uint8_t position,uint8_t num);
void lcdReload(void);
void bu9796Init(void);
void speed_sig_display(uint8_t temp);
void mode_sig_display(uint8_t temp);
void update_lcd_data(void);
void temp_display_flash(void);//图标闪烁

void backlight_init(void);
void backligt_on(void);
void backlight_off(void);
void backlight_toggle(void);
#endif
