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
#define max_low_temp_protect 80//���±����¶�  5~8��
#define mix_low_temp_protect 50
//#define max_set_temp 300//�¶��趨��Χ5-30��
//#define mix_set_temp 50
enum MODE_ENUM//ģʽ
{
	COOL =0,
	HEAT,
	VENTILATE,
};
enum WIND_SPEED_ENUM//����
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
struct TEMP_PARAM//����д��EEPROM�У���ֹ�ϵ綪ʧ
{
	uint8_t  mode;                      //ģʽ
	uint8_t win_speed;                  //����
    uint8_t ventilate_speed;            //ͨ��ģʽ�µķ��٣���ģʽ��û���Զ���
	uint16_t set_temp;                  //�����¶�
	
	uint8_t low_temp_protect;           //���±������أ�
	uint8_t panel_lock;                 //�������
	uint8_t air_coner_switch;           //�յ�����
	uint8_t state_report;               //״̬�ϱ�
	uint8_t  backlight_enable;          //����
	uint8_t   limit_t;                  //�ز��¶�
	uint8_t   low_t;                    //�ز��¶�
	uint16_t max_set_temp;              //��������¶�
	uint16_t mix_set_temp;              //��С�����¶�
	uint16_t max_heat_temp;             //��������¶�
	uint16_t mix_heat_temp;             //��С�����¶�
	uint16_t max_cool_temp;             //��������¶�
	uint16_t mix_cool_temp;             //��С�����¶�

};
struct TEMPERATURE_C
{
    struct TEMP_PARAM temp_param;
    uint16_t  set_temp;

    int16_t   cur_temp;                 //��ǰ�¶�
    uint8_t   temp_valid;               //��δʹ��
    uint8_t   ctrl_delay;               //��ʱһ��ʱ���ٽ��м̵�����������ֹ�ϵ���¶�δ����ʱ�̵�������
    uint8_t   remote_ctrl_flag;         //��δʹ��
    uint32_t  backlight;                //�������ʱ��
    uint8_t   auto_ctrl_flag;           //��δʹ��
    uint8_t   prot_sw;                  //��δʹ��
    uint8_t   sync_delay;               //״̬ͬ������//��δʹ��
    uint8_t   sync_flag;                //��δʹ��
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
	int16_t humi_reduction;//ʪ�Ȳ���
	int16_t temp_reduction;//�¶Ȳ���
//	uint16_t dark_threshold;
	uint8_t active_report;//�����ϱ�
	struct TEMP_PARAM temp_param;
    uint8_t auto_speed_ctrl_mode;//�Զ�����ʱ������Ƿ�رձ�־��
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
