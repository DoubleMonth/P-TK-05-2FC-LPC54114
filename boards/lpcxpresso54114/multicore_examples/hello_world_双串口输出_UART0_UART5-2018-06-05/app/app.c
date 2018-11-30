#include <stdlib.h>
#include <string.h>
#include <types.h>
//#include <os.h>
//#include <printk.h>
#include "app.h"
#include "app_dev_ctrl.h"
//#include "update.h"
#include "protocol_smart.h"
//#include "drivers/stm32_io.h"
//#include "board.h"
#include "auto_report_app.h"
#include "app_key.h"
#include "app_beep.h"
//#include "dev_ctrl.h"
#include "app_realy.h"
//#include "dev_show.h"
#include "app_lcd.h"
#include "alloter.h"

//#include "dev.h"
//extern void num_display_ctrl(uint8_t position, uint8_t number);
extern struct CAP_KEY cap_key;
extern struct FLASH_STRUCT flash_struct;
extern uint8_t save_dev_flag;
struct TEMPERATURE_C temperature_c;
extern struct TEMP_HUMI_CTRL temp_humi_ctrl;
extern uint8_t plc_state_flag;//PLC图标显示标志
extern struct dev dev;
//extern struct FLASH_STRUCT flash_struct;
#define STATE_TIMER_QUEUE_WAIT  100 /* wait 100 ms max while timer task busy */

#define SIGSTACHG   SIGUSR1 /* state change */
#ifdef USING_FREERTOS	
task_handle_t app_task_handle;
#else
extern void app_handle(struct task *t);
struct task app_task = 
{
	.do_task = app_handle, 
};
task_handle_t app_task_handle = (task_handle_t *)&app_task;
#endif
static timer_handle_t state_machine_timer;
//uint8_t key_val;  //按键值的全局变量
struct POWER power;
device_t serial;
device_t serial1, serial2;
uint8_t g_frame_buf[0x400];
//static uint8_t state_machine_end;
 uint8_t plc_alive_flag;
 uint8_t plc_alive_cntr_ms ;//PLC闪烁计时 
uint8_t plc_communication;//PLC通信标志
//---------------------------------------------------------------------------------------
enum
{
    STATE_CTR_NEXT = -1,     /* change to next state */
    STATE_CTR_WAIT = -2,     /* wait for data event */
    STATE_CTR_RETRY = -3,    /* retry current state */
};
//---------------------------------------------------------------------------------------
typedef struct
{
    state_machine_state_t state, next_state;
    const char *name;
    tick_type_t wait;
    base_t (*op0)(uint8_t *out, size_t maxlen);
    base_t (*op1)(const void *arg);
} state_machine_state_op_t;

typedef struct _state_machine
{
    int cur_state, trycnt;
    int init;
    const state_machine_state_op_t *op;
} state_machine_t;
static state_machine_t state_machine;
//---------------------------------------------------------------------------------------

static base_t local_ack_opt(struct SmartFrame *pframe)
{
    if (!pframe)    /* time out */
        return STATE_CTR_RETRY;

    if (!smart_frame_is_local(pframe)) /* got a dummy frame */
        return STATE_CTR_WAIT;

    uint8_t cmd = pframe->data[0];
    if (cmd == CMD_ACK || cmd == CMD_NAK)
    {
      return STATE_CTR_NEXT;
      
      
    }
    return STATE_CTR_WAIT;
}

/*
 * reset plc
 */
static base_t do_rst_plc0(uint8_t *out, size_t maxlen)
{
	plc_reset_set(0);
    return 0;
}
static base_t do_rst_plc1(const void *arg)
{
	plc_reset_set(1);
    return STATE_CTR_NEXT;
}

/*
 * get eid
 */
static base_t do_get_eid0(uint8_t *out, size_t maxlen)
{
    uint8_t cmd = CMD_GET_EID;

    return code_local_frame(&cmd, sizeof(cmd), out, maxlen);
}



static base_t check_sole_encode(void)
{
	if (!is_all_xx(dev.encode.id, 0x00, AID_LEN)//(!0&&!0)
        && !is_all_xx(dev.encode.id, 0xFF, AID_LEN))
		return (1);//有AID
	return (0);//没有AID
}



static base_t do_get_eid1(const void *arg)
{
    struct SmartFrame *pframe;

    pframe = (struct SmartFrame *)arg;
    if (pframe && pframe->data[0] == CMD_ACK_EID)
    {
		if (!check_sole_encode())
			memcpy(dev.encode.id, &pframe->data[1], sizeof(dev.encode.id));
        write_param_to_flash();
        return STATE_CTR_NEXT;
    }
    return STATE_CTR_RETRY;
}

/*
 * set plc aid
 */
static base_t do_set_aid0(uint8_t *out, size_t maxlen)
{
    uint8_t tmp[5] = {0};

    tmp[0] = CMD_SET_AID;
    memcpy(&tmp[1], dev.encode.id, sizeof(dev.encode.id));
    return code_local_frame(tmp, sizeof(tmp), out, maxlen);
}
static base_t do_set_aid1(const void *arg)
{
    return local_ack_opt((struct SmartFrame *)arg);
}

/*
 * notify plc to unlink
 */
static base_t do_set_unlink0(uint8_t *out, size_t maxlen)
{
    uint8_t cmd = CMD_UNLINK;

    return code_local_frame(&cmd, sizeof(cmd), out, maxlen);
}
static base_t do_set_unlink1(const void *arg)
{
    return local_ack_opt((struct SmartFrame *)arg);
}

/*
 * wait one second
 */
static base_t do_wait_sec0(uint8_t *out, size_t maxlen)
{
    return 0;
}
static base_t do_wait_sec1(const void *arg)
{
    return STATE_CTR_NEXT;
}

/*
 * set register
 */
static base_t do_set_reg0(uint8_t *out, size_t maxlen)
{
    uint8_t tmp[2] = {0};

    tmp[0] = CMD_SET_REG;
    tmp[1] = reg.type | reg.last_status;
    reg.last_status = 0;
    return code_local_frame(tmp, sizeof(tmp), out, maxlen);
}
static base_t do_set_reg1(const void *arg)
{
    return local_ack_opt((struct SmartFrame *)arg);
}

/*
 * set panid
 */
static base_t do_set_panid0(uint8_t *out, size_t maxlen)
{
    if (!dev.panid_valid) return STATE_CTR_NEXT;

    uint8_t tmp[3] = {0};

    tmp[0] = CMD_SET_PANID;
    memcpy(&tmp[1], dev.panid, sizeof(dev.panid));
    return code_local_frame(tmp, sizeof(tmp), out, maxlen);
}
static base_t do_set_panid1(const void *arg)
{
    return local_ack_opt((struct SmartFrame *)arg);
}

/*
 * get gateway aid
 */
static base_t do_get_gid0(uint8_t *out, size_t maxlen)
{
    uint8_t cmd = CMD_GET_GWAID;

    return code_local_frame(&cmd, sizeof(cmd), out, maxlen);
}
static base_t do_get_gid1(const void *arg)
{
    if (!arg)    /* time out */
        return STATE_CTR_RETRY;

    struct SmartFrame *pframe = (struct SmartFrame *)arg;

    if (smart_frame_is_local(pframe) && pframe->data[0] == CMD_GET_GWAID)
    {
        uint8_t *gid = &pframe->data[1];
        if (memcmp(dev.gid, gid, sizeof(dev.gid)))
        {
            memcpy(dev.gid, gid, sizeof(dev.gid));
            write_param_to_flash();
        }
        return STATE_CTR_NEXT;
    }
    return STATE_CTR_WAIT;
}
/*
 * get short id
 */
static base_t do_get_sid0(uint8_t *out, size_t maxlen)
{
    uint8_t cmd = CMD_GET_SID;

    return code_local_frame(&cmd, sizeof(cmd), out, maxlen);
}
static base_t do_get_sid1(const void *arg)
{
    if (!arg)    /* time out */
    return STATE_CTR_RETRY;

    struct SmartFrame *pframe = (struct SmartFrame *)arg;

    if (smart_frame_is_local(pframe) && pframe->data[0] == CMD_ACK_SID)
    {
        uint8_t *sid = &pframe->data[1];
        if (memcmp(dev.sid, sid, sizeof(dev.sid)))
        {
            memcpy(dev.sid, sid, sizeof(dev.sid));
            write_param_to_flash();
        }
        //auto_report_parameter_refresh(dev.gid, dev.encode.id, dev.sid, REPORT_GW, FREQ_CHANGE);
        register_report(dev.gid);//获取到SID后开始注册上报
        return STATE_CTR_NEXT;
    }
    return STATE_CTR_WAIT;
}


/*
 * do idle
 */
static base_t do_end0(uint8_t *out, size_t maxlen)
{
  plc_alive_flag=1;//载波流程走完，PLC在线标志为有效
  display_signal(PLC);//显示图标
	return 0;
}
static base_t do_end1(const void *arg)
{
    extern int smart_frame_handle(struct SmartFrame * pframe);//判断是本地报文还是远程报文

    int ret = smart_frame_handle((struct SmartFrame *)arg);
   
    if (ret > 0)
    {
        device_write(serial, 0, arg, ret);
//        plc_communication=true;//发送数据指示
    }
    return STATE_CTR_WAIT;
}
void check_alive0()
{
    uint8_t tmp[2] = {0};
    uint8_t ret=0;
    tmp[0] = CMD_SET_REG;
    tmp[1] = reg.type | reg.last_status;
    reg.last_status = 0;
    ret=code_local_frame(tmp, sizeof(tmp), g_frame_buf, sizeof(g_frame_buf));//组报文
    device_write(serial, 0, g_frame_buf, ret);//发送报文 
//    plc_communication=true;//发送数据指示
}


//有远程通信时PLC图标闪烁 
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
    }
    else
    {
      clr_display_signal(PLC);//关闭图标
   //   log_d(MODULE_APP, "PLC is ERR!!!\n");
    }
    counter=0;
  }
    
}
#ifdef SHOW_SN


extern struct DISP_AP display_ap;
void check_display_ap(void)
{
//	if(display_ap.ntc_show_time > 0)
//	{
//		display_ap.ntc_show_time--;
//		if(0 == display_ap.ntc_show_time)
//			display_ap.ntc_show = 0;
//	}
//	if(display_ap.dew_show_time > 0)
//	{
//		display_ap.dew_show_time--;
//		if(0 == display_ap.dew_show_time)
//			display_ap.dew_show = 0;
//	}
//	
//    if(ON == dev.temp_param.air_coner_switch)
//    {
//        return;
//    }
    
    if(1 == display_ap.ap_show)
    {
        display_ap.ap_show_order++;
    }
		
    if(1 == display_ap.gps_show)
    {
//        get_gps_array();
        display_ap.gps_show_order++;
    }
}
#endif
//---------------------------------------------------------------------------------------
#define state_machine_state(state, next_state, wait, handler) \
{state, next_state, #state, wait, handler##0, handler##1}

static const state_machine_state_op_t state_machine_states[] =
{
    state_machine_state(STATE_RST_PLC, STATE_GET_EID, pdMS_TO_TICKS(1000), do_rst_plc),
    state_machine_state(STATE_GET_EID, STATE_SET_AID, pdMS_TO_TICKS(2000), do_get_eid),
    state_machine_state(STATE_SET_AID, STATE_UNLINK,  pdMS_TO_TICKS(2000), do_set_aid),
    state_machine_state(STATE_UNLINK,  STATE_WAIT,    pdMS_TO_TICKS(2000), do_set_unlink),
    state_machine_state(STATE_WAIT,    STATE_SET_REG, pdMS_TO_TICKS(1000), do_wait_sec),
    state_machine_state(STATE_SET_REG, STATE_SET_PID, pdMS_TO_TICKS(2000), do_set_reg),
    state_machine_state(STATE_SET_PID, STATE_IDLE,    pdMS_TO_TICKS(2000), do_set_panid),

    state_machine_state(STATE_GET_GID, STATE_SET_REG1, pdMS_TO_TICKS(2000), do_get_gid),
    state_machine_state(STATE_SET_REG1, STATE_GET_SID, pdMS_TO_TICKS(2000), do_set_reg),
    state_machine_state(STATE_GET_SID, STATE_IDLE,    pdMS_TO_TICKS(2000), do_get_sid),

    state_machine_state(STATE_IDLE,    STATE_IDLE,    portMAX_DELAY,  do_end),
};
static const state_machine_state_op_t *get_state_op(state_machine_state_t state)
{
    int i;

    for (i = 0; i < array_size(state_machine_states); i++)
    {
        if (state_machine_states[i].state == state)
            return &state_machine_states[i];
    }
    return NULL;
}
//---------------------------------------------------------------------------------------
int is_state_machine_idle(void)
{
    return state_machine.cur_state == STATE_IDLE;
}
//---------------------------------------------------------------------------------------
void state_machine_change(state_machine_state_t state)
{
    const state_machine_state_op_t *op = get_state_op(state);
    if (op)
    {
        state_machine_t *sm = &state_machine;

        log_d(MODULE_APP, "change state to %s\n", op->name);

        taskENTER_CRITICAL();
        sm->init = 1;
        if (sm->cur_state != state)
        {
            sm->cur_state = state;
            sm->trycnt    = 0;
            sm->op        = op;
        }
        taskEXIT_CRITICAL();

        sigsend(app_task_handle, SIGSTACHG);
    }
}
//---------------------------------------------------------------------------------------
static void state_machine_timer_handle(timer_handle_t timer)
{
    sigsend(app_task_handle, SIGTO);
}
//---------------------------------------------------------------------------------------
static void state_machine_schedule(void *arg)
{
    int ret;
    state_machine_t *sm = &state_machine;

redo:
    if (sm->init)
    {
        sm->init = 0;
        ret = sm->op->op0(g_frame_buf, sizeof(g_frame_buf));
        if (ret > 0)
        {
            device_write(serial, 0, g_frame_buf, ret);
        }
        if (sm->op->wait == 0) goto redo;
        timer_change_period(state_machine_timer, sm->op->wait,
                            pdMS_TO_TICKS(STATE_TIMER_QUEUE_WAIT));
    }
    else
    {
        ret = state_machine.op->op1(arg);
      
    }

    if (ret == STATE_CTR_NEXT)
    {
      #ifdef USING_FREERTOS  
      timer_stop(state_machine_timer, pdMS_TO_TICKS(STATE_TIMER_QUEUE_WAIT));
      #endif
        state_machine_change(state_machine.op->next_state);
    }
    else if (ret == STATE_CTR_RETRY)
    {
        state_machine.trycnt++;
        state_machine_change(state_machine.op->state);
    }
}
//---------------------------------------------------------------------------------------

static err_t serial_open(void)
{
#ifdef USING_SERIAL
    err_t err;
    struct serial_configure cfg;

    serial = device_find("uart1");
    assert(serial);

    err = device_control(serial, SERIAL_CTRL_GETCFG, &cfg);
    if (err != 0) return err;

    cfg.baud_rate = BAUD_RATE_9600;
    cfg.data_bits = DATA_BITS_8;
    cfg.stop_bits = STOP_BITS_1;
    err = device_control(serial, SERIAL_CTRL_SETCFG, &cfg);
    if (err != 0) return err;

    err = device_open(serial, DEVICE_FLAG_FASYNC | DEVICE_FLAG_INT_RX | DEVICE_FLAG_INT_TX);
    if (err != 0) return err;
#endif
    return 0;
}

static size_t check_smart_frame(uint8_t *data, size_t len)
{
    struct SmartFrame *pframe = get_smart_frame(data, len);//接收到的有效数据包
    if (!pframe) return 0;

    len = smart_frame_len(pframe);//整个报文的长度

    log_d(MODULE_UART, "got smart frame:\n");

    print_debug_array((uint8_t *)pframe, len);
    state_machine_schedule(pframe);
    len += ((uint8_t *)pframe - data);

    return len;
}

static void do_uart_data(void)
{
    int len = device_peek(serial, 0, g_frame_buf, sizeof(g_frame_buf));
    if (len)
    {
		int ret = check_smart_frame(g_frame_buf, len);
		if (ret)
		{
			device_read(serial, 0, g_frame_buf, ret);
			if (len > ret)
                task_notify(&app_task, SIGUART, eSetBits);
		}
		else
		{
			if (len > 0xFF)
				device_read(serial, 0, g_frame_buf, len);
		}
    }
}

static void state_machine_init(void)
{
  #ifdef USING_FREERTOS  
  state_machine_timer = timer_create("sm",
                                       pdMS_TO_TICKS(STATE_TIMER_QUEUE_WAIT),
                                       pdFALSE,
                                       NULL,
                                       state_machine_timer_handle);
  #else
    static struct soft_timer timer_state_machine;
    memset(&timer_state_machine, 0x00, sizeof(struct soft_timer));
    timer_state_machine.reload = 1;
    timer_state_machine.period = STATE_TIMER_QUEUE_WAIT / 10;
    timer_state_machine.expires = INITIAL_JIFFIES + STATE_TIMER_QUEUE_WAIT / 10;
	timer_state_machine.timer_cb = state_machine_timer_handle;
    state_machine_timer = &timer_state_machine;
    timer_add(state_machine_timer);
#endif
    assert(state_machine_timer);

    state_machine_change(STATE_RST_PLC);
}
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
device_t key;
static err_t key_open(void)
{
#ifdef USING_KEY
    err_t err;
    key = device_find("key");
    assert(key);
    err = device_open(key, DEVICE_FLAG_INT_RX | DEVICE_FLAG_FASYNC);
    if (err != 0) return err;
#endif
    return 0;
}
//struct sensor test_sensor;
struct sensor sensor;
int run_state = 0;
void update_temp_humi(void)
{
#ifdef USING_HDC1080
    uint8_t th_val[4];
    device_t hdc1080 = device_find("h1080");
    assert(hdc1080);
	err_t err = device_control(hdc1080, HDC1080_CTRL_GET_TH, th_val);
	if (err != 0)
    {
        run_state = -1;
        log_d(MODULE_APP, "hdc1080 read TH ERROR!\n");
    }
	else
    {
        double tmp = (double)get_be_val(th_val, 2);
        tmp = (tmp / 65536.0f) * 165.0f - 40.0f;
		sensor.temp = (int16_t)(tmp * 10 - dev.temp_reduction);  //reduce 1 ℃
        tmp = (double)get_be_val(&th_val[2], 2);
        tmp = (tmp / 65536.0f) * 100.0f;
        tmp = tmp < 0 ? 0 : tmp;
        tmp = tmp > 100 ? 100 : tmp;
		sensor.humi = (uint16_t)(tmp * 10);
		log_d(MODULE_APP, "hdc1080 temp:%d humi:%d\n", sensor.temp, sensor.humi);
        run_state = 0;
    }
#else
    uint8_t th_val[4];
    device_t si7020 = device_find("si7020");
    assert(si7020);
    err_t err;
    if(temp_humi_ctrl.temp_humi_read_conter==2) 
    {
        err=device_control(si7020, SI7020_CTRL_TEMP_CMD, th_val);
    } 
    else if(temp_humi_ctrl.temp_humi_read_conter==4) 
    {
        err=device_control(si7020, SI7020_CTRL_GET_TEMP, th_val);
        if (err != 0) 
        {
            run_state = -1;

            log_d(MODULE_APP, "si7020 read TEMP ERROR!\n");

//            display_group_sig(ERR_OR,NULL);
            flash_struct.breakdown_start_flag=true;
        } 
        else 
        {
            flash_struct.breakdown_start_flag=false;
            double tmp = (double)get_be_val(th_val, 2);
            tmp = (tmp * 175.72f / 65536.0f) - 46.85f;
            sensor.temp = (int16_t)(tmp * 10 + dev.temp_reduction);
           // log_d(MODULE_APP, "Temp:%d Humi:%d\n", sensor.temp, sensor.humi);
            run_state = 0;
        }
    }
    else if(temp_humi_ctrl.temp_humi_read_conter==6)
    {
        err=device_control(si7020, SI7020_CTRL_GET_HUMI, &th_val[2]);
        if (err != 0) 
        {
            run_state = -1;

            log_d(MODULE_APP, "si7020 read HUMI ERROR!\n");

//            display_group_sig(ERR_OR,NULL);
            flash_struct.breakdown_start_flag=true;
        } 
        else 
        {
            flash_struct.breakdown_start_flag=false;
            double tmp = (double)get_be_val(th_val, 2);
            tmp = (tmp * 175.72f / 65536.0f) - 46.85f;
            tmp = (double)get_be_val(&th_val[2], 2);
            tmp = (tmp * 125.0f / 65536.0f) - 6.0f;//
//            tmp=(uint16_t)(tmp*10+dev.humi_reduction);
//            tmp = tmp < 0 ? 0 : tmp;
//            sensor.humi = (int16_t)(tmp * 10+dev.humi_reduction);
//            tmp = tmp > 99 ? 99 : tmp;//
            tmp = (tmp*10+dev.humi_reduction);//进行温度补偿
            tmp = tmp < 0 ? 0 : tmp;          //判断补偿会的值
            tmp = tmp > 999 ? 999 : tmp;
            sensor.humi = (uint16_t)tmp;        
            run_state = 0;
        }
    }

#endif
}
//extern struct TEMP_PARAM tp_param;
//extern struct TEMPERATURE_C temperature_c;
//uint8_t lcd_conter;
static void lcd_poweron_init(void)
{
    
	device_t bu9796 = device_find("b9796");
    assert(bu9796);
	device_open(bu9796, 0);
}
static void lcd_display_update(void)
{
    uint8_t data[6];
    device_t bu9796 = device_find("b9796");
    assert(bu9796);

    put_le_val(sensor.temp, data, 2);//温度
    put_le_val(sensor.humi, &data[2], 2);		//湿度
    data[4] = is_state_machine_idle();
    data[5] = run_state < 0 ? 1 : 0;
    err_t err = device_write(bu9796, 0, data, sizeof(data));
    if (err != 0)
    {

        log_d(MODULE_APP, "bu9796 write ERROR!\n");

    }
       
    if(dev.temp_param.panel_lock==ON)//面板锁定指示。
      flash_struct.lock_start_flag=true;
    else if(dev.temp_param.panel_lock==OFF)
      flash_struct.lock_start_flag=false;
}


static void ms60_tmr_cb(timer_handle_t tmr)
{
	sigsend(app_task_handle, SIGMSFOUR);//
}

static void ms60_tmr_init(void)
{
  #ifdef USING_FREERTOS  
  timer_handle_t ms60_tmr = timer_create("40_ms", pdMS_TO_TICKS(60), pdTRUE, NULL, ms40_tmr_cb);
  
  #else
    static struct soft_timer timer_ms60;
    memset(&timer_ms60, 0x00, sizeof(struct soft_timer));
    timer_ms60.reload = 1;
    timer_ms60.period = HZ / 10;
    timer_ms60.expires = INITIAL_JIFFIES + HZ / 10;
	timer_ms60.timer_cb = ms60_tmr_cb;
    static timer_handle_t ms60_tmr = &timer_ms60;
  #endif
	assert(ms60_tmr);
	timer_start(ms60_tmr, 6);
}

static void ms100_tmr_cb(timer_handle_t tmr)
{
	sigsend(app_task_handle, SIGMSTO);
}

static void ms100_tmr_init(void)
{
  #ifdef USING_FREERTOS  
  timer_handle_t ms100_tmr = timer_create("100_ms", pdMS_TO_TICKS(100), pdTRUE, NULL, ms100_tmr_cb);
  #else
    static struct soft_timer timer_ms100;
    memset(&timer_ms100, 0x00, sizeof(struct soft_timer));
    timer_ms100.reload = 1;
    timer_ms100.period = HZ / 10;
    timer_ms100.expires = INITIAL_JIFFIES + HZ / 10;
	timer_ms100.timer_cb = ms100_tmr_cb;
    static timer_handle_t ms100_tmr = &timer_ms100;
#endif
	assert(ms100_tmr);
	timer_start(ms100_tmr, 10);
}


static void second_tmr_cb(timer_handle_t tmr)
{
	sigsend(app_task_handle, SIGALARM);
}

static void second_tmr_init(void)
{
  #ifdef USING_FREERTOS  
  timer_handle_t sec_tmr = timer_create("second", pdMS_TO_TICKS(1000), pdTRUE, NULL, second_tmr_cb);
  #else
    static struct soft_timer timer_sec;
    memset(&timer_sec, 0x00, sizeof(struct soft_timer));
    timer_sec.reload = 1;
    timer_sec.period = HZ;
    timer_sec.expires = INITIAL_JIFFIES + HZ;
	timer_sec.timer_cb = second_tmr_cb;
    static timer_handle_t sec_tmr = &timer_sec;
#endif
    assert(sec_tmr);
    timer_start(sec_tmr, HZ);
}

//100ms测试
//#define toggle_pin(ADDRESS,BIT)    (GPIO_WriteBit(ADDRESS,BIT,(BitAction)((1-GPIO_ReadOutputDataBit(ADDRESS,BIT)))))
//void gpio_test()
//{
//	GPIO_InitTypeDef GPIO_InitStructure;
//   
//    
//    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
//   
//    
//    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_15;
//    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
//    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOB, &GPIO_InitStructure);
//	//GPIO_SetBits(GPIOB,GPIO_Pin_15);
//}

////static void on_second_time(void)
////{
////	
//////	update_temp_humi();
//////	TIM_Cmd(TIM2, ENABLE);
////	lcd_display_update();
////    auto_report_sec_task();
////	temp_ctrl_realy();
////	
////	extern uint16_t photosensor_value;
//////	set_backlight_brightness(photosensor_value > dev.dark_threshold ? dev.brightness_min : dev.brightness_max);
//////	set_backlight_brightness(100);
//////	log_d(MODULE_APP, "photosensor_value is %d!\n", photosensor_value);
////}

static void factory_reset(void)
{
    uint32_t key_state;

    device_control(key, KEY_CTRL_GET_KEY, &key_state);

    log_d(MODULE_APP, "key pressed now!\n");

    if (tst_bit(key_state, 16))
    {
        memset(dev.gid, 0x00, offsetof(struct dev, power_on_delay) - offsetof(struct dev, gid));
//        dev.power_on_delay = 60;
//		dev.temp_report_step = 10;//默认步长为1度
//        dev.temp_report_freq = 0;//默认关闭定频上报
//		dev.humi_report_step = 50;//默认步长为5%
//        dev.humi_report_freq = 0;//默认关闭定频上报
//        dev.humi_reduction = 30;//湿度默认加3%
//        dev.temp_reduction = -15;//温度默认减1.5度
//        dev.active_report=0x03;//主动上报网关和设备
//        dev.auto_speed_ctrl_mode=0x00;//默认自动风时不关闭风机。
        temp_param_init();
        write_param_to_flash();

        log_d(MODULE_APP, "Factory reset now!\n");

        board_reset();
    }
}


//extern void scan_handle();
extern void temp_display_flash(void);
extern void flash_display_delay(void);
//extern uint8_t temp_humi_read_conter;
//extern uint8_t read_temp_flag;
//extern uint8_t read_humi_flag;
extern struct CAP_KEY cap_key;
extern uint8_t plc_state_flag;
extern uint8_t lcd_temp_update;//温度刷新控制
#ifdef USING_FREERTOS
void app_entry(void *args) 
{
	// uint8_t key_val;  
	sigset_t sigmap, signal;
	sigempty(sigmap);
	dev_init();
	dev_show();
	init_chn_pool_mgr();
	state_machine_init();
	serial_open();
	key_open();
	ms40_tmr_init();
	ms100_tmr_init();
	second_tmr_init();
	lcd_poweron_init();
	//gpio_test();/////////////////////测试时间使用，控制PB15引脚翻转
	realy_init();
	temperature_c.backlight=BL_on_time;
	//背光点亮一段时间
//	timer2_init();
	power.power_on_delay_flag=1;
	//上电延时标志置位
	buzzer_init();
	//
	//lcd_backlight_on();
	//lcd_backlight_off();
	for (;;) 
	{
		sigwait(sigmap, &signal);
		/* check data received event */
		if (sigget(signal, SIGUART)) 
		{
			do_uart_data();
		}
		/* check state change or time out event */
		if (sigget(signal, SIGSTACHG) || sigget(signal, SIGTO)) 
		{
			state_machine_schedule(NULL);
		}
		//40ms任务
		if(sigget(signal,SIGMSFOUR)) 
		{
			temp_humi_ctrl.temp_humi_read_conter++;
			if(temp_humi_ctrl.temp_humi_read_conter>10)
									temp_humi_ctrl.temp_humi_read_conter=0;
			update_temp_humi();
			//          dev_search_param.cur_time++;
		}
		/* check 100ms timer event */
		if (sigget(signal, SIGMSTO)) 
		{
			#if DEV_SHOW
			      dev_search_param.cur_time++;
			#endif     
			      board_feed_wdg();
			//            set_report_100ms_task();
			if(power.power_on_delay_flag==1)//上电1秒内液晶不显示 
			{
				power.power_on_delay_conter++;
				if(power.power_on_delay_conter>10) 
				{
					power.power_on_delay_flag=0;
					power.power_on_delay_conter=10;
				}
			}
			if(power.power_on_delay_flag==0&&power.power_on_delay_conter==10)
									lcd_display_update();
			//从秒任务中改为100ms中
			key_scan();
			//scan_handle();
			if(cap_key.key_chn>0)//关闭蜂鸣器 
			{
				cap_key.key_chn--;
				if(cap_key.key_chn==0) 
				{
					buzzer_off();
				}
			}
			if(cap_key.response_chn>0) 
			{
				cap_key.response_chn--;
			}
			#if DEV_SHOW
								check_dev_show();
			//设备搜索指示
			#endif 
			//          log_d(MODULE_APP, "temperature_c.backlight is %d!\n", temperature_c.backlight);
			temp_display_flash();
			//设置温度闪烁
			plc_communication_handle();
			if(plc_communication==true)
			          plc_alive_cntr_ms++;
			//PLC通信闪烁计数
		}
		/* check alarm event */
		if (sigget(signal, SIGALARM)) 
		{
			on_second_time();
			#if DEV_SHOW
			        check_search_delay();
			//
			#endif 
			if(SAVE_DEV== save_dev_flag)//结构体数据存储到FLASH标志有效时 
			{
				save_dev_flag=0x00;
				write_param_to_flash();
				//写入FLASH
			}
			lcd_temp_update++;
			//      check_alive();
		}
		/* check key event */
		if (sigget(signal, SIGKEY)) 
		{
			factory_reset();
		}
	}
}
#else
static void on_60ms_time(void)
{
    temp_humi_ctrl.temp_humi_read_conter++;
    if(temp_humi_ctrl.temp_humi_read_conter>10)
    temp_humi_ctrl.temp_humi_read_conter=0;
    update_temp_humi();
}
//extern uint8_t key_counter;
extern uint8_t press_order[13];
extern uint8_t press_validity;
static void on_100ms_time(void)
{
    #if DEV_SHOW
        dev_search_param.cur_time++; 
    #endif     
    board_feed_wdg();
    if(power.power_on_delay_flag==true)//上电2秒内液晶全显
    {
        power.power_on_delay_conter++;
        if(power.power_on_delay_conter>MAX_DELAY_TIME)
        {
            power.power_on_delay_flag=false;
            power.power_on_delay_conter=MAX_DELAY_TIME;
        }
    }
    if(power.power_on_delay_flag==false)
    {
        lcd_display_update();//从秒任务中改为100ms中
    }
//    key_scan();
    if(cap_key.key_chn>0)//关闭蜂鸣器
    {
        cap_key.key_chn--;
        if(cap_key.key_chn==0)
        {
            buzzer_off();
        }
    }	
    if(temperature_c.backlight>0)//背光点亮时间控制
    {
        temperature_c.backlight--;
      //  log_d(MODULE_APP, "temperature_c.backlight%d\n",temperature_c.backlight);
        lcd_backlight_on(temperature_c.backlight);
    }
//    else if (temperature_c.backlight==0)
//    {
//        lcd_backlight_on(temperature_c.backlight);
//    }
    if(cap_key.response_chn>0) 
    {
        cap_key.response_chn--;
    }
    #if DEV_SHOW
    check_dev_show();//设备搜索指示
    #endif 
    temp_display_flash();//设置温度闪烁
    plc_communication_handle();
    if(plc_communication==true)
        plc_alive_cntr_ms++;//PLC通信闪烁计数
    #ifdef SHOW_SN
    show_sn_panid();
    
    #endif
}
static void on_1s_time(void)
{
//    on_second_time();
    
 //   lcd_display_update();
    auto_report_sec_task();
	temp_ctrl_realy();
	
	extern uint16_t photosensor_value;
    
    #if DEV_SHOW
    check_search_delay();//
    #endif 
    if(SAVE_DEV== save_dev_flag)//结构体数据存储到FLASH标志有效时
    {
        save_dev_flag=0x00;
        write_param_to_flash();//写入FLASH 
    }
    lcd_temp_update++;
    if(temperature_c.ctrl_delay>0)//上电后延时一段时间再进行继电器动作
    {
        temperature_c.ctrl_delay--;
    }   
    check_alive();//PLC是否在线检测
    check_display_ap();
}
extern sigset_t task_sigget(task_handle_t t);
void app_handle(struct task *t)
{
    sigset_t signal = task_sigget(t);

    /* check data received event */
    if (sigget(signal, SIGUART))
    {
        do_uart_data();
    }
    /* check state change or time out event */
    if (sigget(signal, SIGSTACHG) || sigget(signal, SIGTO))
    {
        state_machine_schedule(NULL);
    }
    //60ms任务
    if(sigget(signal,SIGMSFOUR))
    {
        on_60ms_time();
    }
    /* check 100ms timer event */
    if (sigget(signal, SIGMSTO))
    {
        on_100ms_time();
    }
    /* check alarm event */
    if (sigget(signal, SIGALARM))
    {
        on_1s_time();
    }
     /* check key event */
		if (sigget(signal, SIGKEY))
    {
        factory_reset();
    }
}
void app_init(void)
{
    dev_init();
    dev_show();
    init_chn_pool_mgr();
    state_machine_init();
    serial_open();
    key_open();
    ms60_tmr_init();
    ms100_tmr_init();//
    second_tmr_init();//
    lcd_poweron_init();
    realy_init();
    temperature_c.backlight=BL_on_time;//背光点亮一段时间
    temperature_c.ctrl_delay=CTRL_DELAY_TIME;//开机延时一段时间(5s)后再进行继电器动作，
    timer2_init();
    power.power_on_delay_flag=true;//上电延时标志置位
    buzzer_init();//
    task_add(&soft_timer_task);
    task_add(&app_task);
    __enable_irq();
}
#endif
//---------------------------------------------------------------------------------------
