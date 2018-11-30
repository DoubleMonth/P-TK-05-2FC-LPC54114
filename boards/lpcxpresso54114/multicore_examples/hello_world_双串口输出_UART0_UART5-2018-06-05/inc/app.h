#ifndef _APP_H_
#define _APP_H_

#define STATE_MACHINE_STATE_WAIT        200 //2s
#define STATE_MACHINE_CACHE_SIZE        0x100
#define MAX_DELAY_TIME                  20               //LCD上电全显延时最大时间
extern uint8_t g_frame_buf [0x400];
typedef enum
{
    STATE_IDLE,
    STATE_RST_PLC,
    STATE_GET_EID,
    STATE_SET_AID,
    STATE_UNLINK,
    STATE_WAIT,
    STATE_SET_REG,
    STATE_SET_PID,
    STATE_GET_GID,
    STATE_GET_SID,
    STATE_SET_REG1,
} state_machine_state_t;

struct sensor
{
	int16_t temp;
    int16_t humi;
};
struct POWER
{
	uint8_t power_on_flag;//上电标志
	uint8_t power_on_delay_flag;//上电延时标志，用于上电后延时1sLCD正常显示
	uint8_t power_on_delay_conter;//上电延时计数
};
extern struct sensor sensor;

void app_init(void);
void app_entry(void *args);
void update_temp_humi(void);
int is_state_machine_idle(void);
void state_machine_change(state_machine_state_t state);
void check_alive0(void);
#endif
