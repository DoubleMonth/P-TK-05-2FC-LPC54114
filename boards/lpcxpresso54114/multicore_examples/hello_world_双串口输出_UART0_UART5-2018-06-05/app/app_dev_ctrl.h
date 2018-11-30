#ifndef __APP_DEV_CTRL_
#define __APP_DEV_CTRL_
#include "stdio.h"
#include "string.h"
#include <stdint.h>
#include "alloter.h"
#include "protocol_smart.h"
extern volatile uint32_t task_monitor;
#define DisableInterrupts() __disable_irq()
#define EnableInterrupts()  __enable_irq()
#define OS_CPU_SR  uint32_t
#define OS_ENTER_CRITICAL() do { \
        DisableInterrupts(); \
} while (0)

//#define OS_EXIT_CRITICAL()  __set_PRIMASK(cpu_sr)
#define OS_EXIT_CRITICAL()  EnableInterrupts()

#define set_bit_security(x, bit)     \
do {\
    OS_ENTER_CRITICAL();\
    set_bit(x, bit);\
    OS_EXIT_CRITICAL();\
} while(0)
#define clr_bit_security(x, bit)     \
do {\
    OS_ENTER_CRITICAL();\
    clr_bit(x, bit);\
    OS_EXIT_CRITICAL();\
} while(0)

//---------------------------------------------------------------------------------------
#define min(a, b) ((a)<(b) ? (a):(b))
#define max(a, b) ((a)>(b) ? (a):(b))
//#define bcd2bin(val) (((val) & 0x0f) + ((val) >> 4) * 10)
//#define bin2bcd(val) ((((val) / 10) << 4) + (val) % 10)
//#define BCD2BIN(x)  ((((x)>>4)*10)+((x) & 0x0F))
#define BIN2BCD(x)  ((((unsigned char)(x)/10)<<4)+((unsigned char)(x) % 10))
//#define bin2bcd(x)  ((((((x/10)/10)<<4)+((x/10) % 10))<<4)+(((((x%10)/10)<<4)+((x%10) % 10))))
//#define bcd2bin(x)  ((BCD2BIN((x)>>8)*100)+(BCD2BIN((x) & 0xFF)))
//---------------------------------------------------------------------------------------

#define is_bit_set(x, bit)     ((x) & (1 << (bit)))
#define clr_bit(x, bit) ((x) &= ~(1 << (bit)))
//---------------------------------------------------------------------------------------
#define set_bit(x, bit) ((x) |= 1 << (bit))
#define clr_bit(x, bit) ((x) &= ~(1 << (bit)))
#define tst_bit(x, bit) ((x) & (1 << (bit)))
#define get_bits(val,x1,x2)   (((val)>>(x1))&((1<<((x2)-(x1)+1))-1))
#define hole(val, start, end) ((val) & (~(((1 << ((end) - (start) + 1)) - 1) << (start))))
#define fill(set, start, end) (((set) & ((1 << ((end) - (start) + 1)) - 1))<< (start))
#define set_bits(val, set, start, end) (val = hole(val, start, end) | fill(set, start, end))
//---------------------------------------------------------------------------------------
#define array_size(array) (sizeof(array)/sizeof(*array))
//---------------------------------------------------------------------------------------
#define notify(task_bit)                 set_bit_security(task_monitor, task_bit)
#define is_task_set(task_bit)            tst_bit(task_monitor, task_bit)
#define is_task_always_alive(flags)      (flags & ALWAYS_ALIVE)
#define reset_task(task_bit)             clr_bit_security(task_monitor, task_bit)

enum
{
    EV_CLRDOG,
    EV_PLC,
    EV_DEBUG,
    EV_TICK,
    EV_20MS,
    EV_100MS,
    EV_SEC,
    EV_MIN,
    EV_KEY,
    EV_STATE
};
enum
{
    ALWAYS_ALIVE = 0x01
};
struct task
{
    uint8_t id;
    uint8_t flags;
    void *args;
    void (*handle) (void *args);
};
struct KEY_INFOR
{
	uint8_t key_chn;
};

#define SAVE_DEV 0xAA
#define MACHINE_ON 0X01
#define MACHINE_OFF 0X00
#define max_low_temp_protect 80//低温保护温度  5~8度
#define mix_low_temp_protect 50
//#define max_set_temp 300//温度设定范围5-30度
//#define mix_set_temp 50
enum MODE_ENUM//模式
{
	COOL =0,
	HEAT,
	VENTILATE,
};
enum WIND_SPEED_ENUM//风速
{
	LOW=0,
	MIDDLE,
	HIGH,
	AUTO,
};
enum ON_OFF
{
	
	OFF=0,
	ON=0x01,
};
enum DI_ABLE
{
	MY_DISABLE=0,
	MY_ENABLE=1,
};
struct TEMP_PARAM//将此写入EEPROM中，防止断电丢失
{
	uint8_t  mode;                      //模式
	uint8_t win_speed;                  //风速
    uint8_t ventilate_speed;            //通风模式下的风速，此模式下没有自动风
	uint16_t set_temp;                  //设置温度
	
	uint8_t low_temp_protect;           //低温保护开关，
	uint8_t panel_lock;                 //面板锁定
	uint8_t air_coner_switch;           //空调开关
	uint8_t state_report;               //状态上报
	uint8_t  backlight_enable;          //背光
	uint8_t   limit_t;                  //回差温度
	uint8_t   low_t;                    //回差温度
	uint16_t max_set_temp;              //最大设置温度
	uint16_t mix_set_temp;              //最小设置温度
	uint16_t max_heat_temp;             //最大制热温度
	uint16_t mix_heat_temp;             //最小制热温度
	uint16_t max_cool_temp;             //最大制冷温度
	uint16_t mix_cool_temp;             //最小制冷温度

};
struct TEMPERATURE_C
{
    struct TEMP_PARAM temp_param;
    uint16_t  set_temp;

    int16_t   cur_temp;                 //当前温度
    uint8_t   temp_valid;               //暂未使用
    uint8_t   ctrl_delay;               //延时一段时间再进行继电器动作，防止上电后温度未读出时继电器动作
    uint8_t   remote_ctrl_flag;         //暂未使用
    uint32_t  backlight;                //背光点亮时间
    uint8_t   auto_ctrl_flag;           //暂未使用
    uint8_t   prot_sw;                  //暂未使用
    uint8_t   sync_delay;               //状态同步降速//暂未使用
    uint8_t   sync_flag;                //暂未使用
};


//#define STC             0x7E
//#define DID_LEN         0x02
//#define SID_LEN         0x02
//#define AID_LEN         0x04
//#define ID_LEN          0X04//
//#define PW_LEN          0x02
//#define PANID_LEN		0x02
//#define BUF_LEN			0xFF
//#define EID_LEN			0x08
//#define PW_LEN          0x02
//#define PSK_LEN 		0x08
//#define SN_LEN          0x0C
//#define DKEY_LEN        0x08
//#define MAGIC_NUM_LEN	0x04
//#define WDATA_NUM		0x0A
//#define WDATA_LEN		0x40
//#define HEAD_MAGIC_NUM	0x44CCA5A5
//#define MAGIC_NUM	0x55AA2B2B
#define PARAM_ADDR 0x00000400U
#define MAGIC_NUM_LEN	0x04

struct ENCODE_PARAM
{
    uint8_t sn[SN_LEN];//12
    uint8_t dkey[DKEY_LEN];//8
    uint8_t id[AID_LEN];//4
    uint8_t pwd[PW_LEN];//2
};

#define HEAD_MAGIC_NUM	0x44CCA5A5
#define MAGIC_NUM	0x55AA2B2B
struct dev
{
	
    uint32_t head_magic;
    struct ENCODE_PARAM encode;
	uint8_t gid[AID_LEN];//4
	uint8_t panid[PANID_LEN];//2
	uint8_t sid[SID_LEN];//2
	uint8_t panid_valid;
	uint16_t power_on_delay;//3c
	uint16_t temp_report_freq;
	uint16_t temp_report_step;
	uint16_t humi_report_freq;
	uint16_t humi_report_step;
//	uint8_t brightness_min;
	int16_t humi_reduction;//湿度补偿
	int16_t temp_reduction;//温度补偿
//	uint16_t dark_threshold;
	uint8_t active_report;//主动上报
	struct TEMP_PARAM temp_param;
    uint8_t auto_speed_ctrl_mode;//自动风速时，风机是否关闭标志。
	uint32_t magic;

};
enum
{
	REPORT_OFF=0,
	REPORT_GATEWAY,
	REPORT_DEVICE,
	REPORT_GATW_DEV,
};
struct reg
{
    uint8_t type;
    uint8_t last_status;
};
struct UART_Infor
{
    int              busy_rxing;
    //int over_time_tick;
    struct _CHN_SLOT tx_slot, rx_slot;
};
void showTempHumi(void);
void task_handle(void);
void win_speed_handle(void);
void temp_param_init(void);
void auto_speed_display(void);
void speed_realy_ctrl(uint8_t ctrl);
void on_off_handle(void);
void mode_handle(void);
void add_temp_handle(void);
void dec_temp_handle(void);
void dev_init(void);
void write_to_flash(void);
void app_init(void);
int dev_type_get(uint8_t *out, size_t maxlen);
int dev_ver_get(uint8_t *out, size_t maxlen);
int dev_ver_cmp(const uint8_t *ver, size_t len);
int uart_write(int chn, const void *in, int len);
void system_init(void);
#endif
