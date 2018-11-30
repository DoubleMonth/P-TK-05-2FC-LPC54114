//#include "headfiles.h"
#include "alloter.h"
#include "protocol_smart.h"
#include "auto_report_app.h"
#include "fsl_debug_console.h"
#include "app_dev_ctrl.h"

//#include "printk.h"
//#include "dev.h"
static report_para_t subs_data[MAX_SUBSCRIBER_NUM];
static flags_t flags;
//flags_t flags;
static delay_data_t delay_data;
judge_data_t judge_data;
extern struct dev dev;
extern uint8_t g_frame_buffer[0x400];
#if (SUBSCRIBER_NUM)
static sub_pos_t sub_pos;
#endif

//--------------------------------------------------------------------------------------
struct _CHN_SLOT re_chn[REPORT_NUM];
static void report_alloter_init(void)//上报内存池初始化
{
	struct _CHN_SLOT *pchn;
	for(uint8_t i = POWER_ON;i < REPORT_NUM;i++)
	{
		pchn = &re_chn[i];
		pchn->data_cnt = 0;
		pchn->data_max = 300;
		pchn->tx	   = INVALID_PTR;
		pchn->rx	   = INVALID_PTR;
	}
}

int peek_chn(int chn, uint8_t buf[], int len)
{
	return (peek_chn_bytes(&re_chn[chn], buf, len));
}

int get_chn(int chn, uint8_t buf[], int len)
{
	return (get_chn_bytes(&re_chn[chn], buf, len));
}

int put_chn(int chn, const void *in, int len)
{
	return(put_chn_bytes(&re_chn[chn], (uint8_t *)in, len));
}

void clear_chn(int chn)
{
	get_chn_bytes(&re_chn[chn],g_frame_buffer,sizeof(g_frame_buffer));
}

//--------------------------------------------------------------------------------------
/* just save reliable report using for retry, dst is gw_id, said is my_id */
static void save_subs_infor(uint8_t seq,report_type_t report_type)//保存定阅者信息
{
	uint8_t i;
	for (i = 0; i< MAX_SUBSCRIBER_NUM; i++)
	{
		if (IDLE == subs_data[i].state_flag)/*i have enough space to start report task*/
		{
			goto check_out;
		}
	}
	return;

check_out:
	subs_data[i].state_flag = SETTING;
	subs_data[i].ack_state = NO_ACK;
	subs_data[i].report_seq = seq;
	subs_data[i].report_typ = report_type;
	subs_data[i].time_count = 0;
	subs_data[i].retry_count = 0;

    PRINTF("Save subs infor!!!\n");

}

static void send_report_frame(uint8_t *buff,uint8_t len,report_type_t report_type)//发送上报框架
{
	struct SmartFrame *pframe = (struct SmartFrame *)buff;
	if(is_all_xx(pframe->said,0x00,ID_LEN) || (is_all_xx(pframe->taid,0x00,ID_LEN))) return;
	uart_write_report(buff, len);
	if(RELIABLE == pframe->data[0])
	{
		save_subs_infor(pframe->seq,report_type);
	}
}
//--------------------------------------------------------------------------------------
static report_para_t *find_sub_from_type(report_type_t r_type)//从类型 找出定阅者
{
	for (uint8_t i = 0; i< MAX_SUBSCRIBER_NUM; i++)
	{
		if (r_type == subs_data[i].report_typ)
		{
			return &subs_data[i];
		}
	}
	return NULL;
}

static report_para_t *find_sub_from_seq(uint8_t seq)//从seq找出定阅者
{
	for (uint8_t i = 0; i< MAX_SUBSCRIBER_NUM; i++)
	{
		if ((seq&0x7f) == subs_data[i].report_seq)
		{
			return &subs_data[i];
		}
	}
	return NULL;
}

static void send_retry(report_type_t r_type)//重试
{
    uint8_t len = 0;

	report_para_t *sub = find_sub_from_type(r_type);
	
	if(NULL != sub)
    {
        len = peek_chn(r_type, g_frame_buffer, sizeof(g_frame_buffer));
        if(!len) return;
        len = code_frame(judge_data.my_id,judge_data.gw_id, sub->report_seq,RELIABLE, g_frame_buffer, 
                   len,g_frame_buffer, sizeof(g_frame_buffer));
        
        uart_write_report(g_frame_buffer, len);
        sub->ack_state = NO_ACK;
        sub->retry_count++;
    }
}


static void finish_power_on_report(void)
{
#if MAX_SENSOR_NUM
    uint8_t len = get_chn(POWER_ON,g_frame_buffer,sizeof(g_frame_buffer));
    report_finish_refresh_infor(g_frame_buffer,len);
#endif
    clear_chn(POWER_ON);
    flags.power_on_report = FINISHED;
}

static void send_max_retry(report_para_t sub)//发送最大重试
{
    if(POWER_ON != sub.report_typ) return;

    finish_power_on_report();
}

static uint32_t get_time_from_trycnt(uint8_t trycnt, uint8_t wait_type)//从次数中得到时间
{
	switch (trycnt)
	{
	case 0:
		return (WAIT_CHANNEL_ABILITY == wait_type) ? CHANNEL_COMMUNICATE_ABILITY : FIRST_RETRY_INTERVAL;//2:10
	case 1:
		return (WAIT_CHANNEL_ABILITY == wait_type) ? (CHANNEL_COMMUNICATE_ABILITY + FIRST_RETRY_INTERVAL) : (FIRST_RETRY_INTERVAL + SECOND_RETRY_INTERVAL);
	case 2:
		return (WAIT_CHANNEL_ABILITY == wait_type) ? (CHANNEL_COMMUNICATE_ABILITY + FIRST_RETRY_INTERVAL + SECOND_RETRY_INTERVAL) : 0xffffffff;
	default:
		return 0xffffffff;
	}
}

static void report_time_hook(void)//上报时间
{
	uint8_t i = 0;

	for (i = 0; i < MAX_SUBSCRIBER_NUM; i++)
	{
		if (SETTING == subs_data[i].state_flag)
		{
			subs_data[i].time_count++;
			if (subs_data[i].time_count < get_time_from_trycnt(subs_data[i].retry_count, WAIT_CHANNEL_ABILITY))//在规定时间内收到ACK——结束，
			{
				if (YES_ACK == subs_data[i].ack_state)/*get the ack*/
				{

                    PRINTF("RECEIVE ACK!\n");

                    goto stop;
				}
			}
			else if (MAX_TRY_CNT <= subs_data[i].retry_count)//如果重试次数>=最大次数
			{
				send_max_retry(subs_data[i]);

                PRINTF( "MAX_TRY!\n");

                goto stop;
			}
			else if (subs_data[i].time_count >= get_time_from_trycnt(subs_data[i].retry_count, WAIT_FOR_TIME))//大于最大时间
			{
				send_retry(subs_data[i].report_typ);//发送重试。

                PRINTF( "REPORT RETRY!\n");


			}
		}
	}
	return;
stop:
	stop_report_para(i);
    
	return;
}

//--------------------------------------------------------------------------------------
/* it will be used when receive gw ack */
void report_frame_ack(uint8_t seq, uint8_t source_addr[], uint8_t *fbd)//上报框架应答
{
	report_para_t *sub = find_sub_from_seq(seq);
	
	uint8_t len = 0,buf_ack[0x100];

	PRINTF("Receive gw ack!\n");

	if(NULL != sub)
    {
        len = peek_chn(sub->report_typ,buf_ack,sizeof(buf_ack));
        if(!len) return;
        if ((!memcmp(buf_ack, fbd, len)) && (!memcmp(source_addr, judge_data.gw_id, ID_LEN)))
        {
            report_finish_refresh_infor(fbd,len);
            sub->ack_state = YES_ACK;
            //clear_chn(sub->report_typ);

            PRINTF("Receive gw ack! the report_type is %d\n",sub->report_typ);

            if(POWER_ON == sub->report_typ)
                flags.power_on_report = FINISHED;
        }
    }
}
//--------------------------------------------------------------------------------------
#if (SUBSCRIBER_NUM)
static void insert_said_to_queue(void *s, void * gw_id)//将AID放入队列
{
	uint8_t i;

	if (0x00 == memcmp(gw_id, s, ID_LEN))
	{
		return;
	}
	for (i = 0; i < SUBSCRIBER_NUM; i++)
	{
		if (0x00 == memcmp(&judge_data.subs_id[ID_LEN*i], s, ID_LEN))
		{
			break;
		}
	}
	if (i >= SUBSCRIBER_NUM)
	{
		memcpy(&judge_data.subs_id[ID_LEN * judge_data.subs_pos], s, ID_LEN);
		judge_data.subs_pos++;
		if (judge_data.subs_pos >= SUBSCRIBER_NUM)
		{
			judge_data.subs_pos = 0;
		}
	}
}
#endif

#if MAX_ACTOR_NUM

static uint8_t get_message_source(void *source_addr, void * dst_id, void * gw_id)//得到信息资源
{
	if(is_all_xx(dst_id,0xff,ID_LEN))
	{
		if(judge_data.find_myself)
        {
            memset(judge_data.last_id, 0x00, ID_LEN);
            memcpy(judge_data.taker_id, source_addr, ID_LEN); 
            return (BROADCAST);
        }
        return (FREE);
	}
  else
  {
		#if SUBSCRIBER_NUM
      insert_said_to_queue(source_addr, gw_id);
		#endif
      memcpy(judge_data.last_id, source_addr, ID_LEN);
      memcpy(judge_data.taker_id, source_addr, ID_LEN); 
      return (P2P);
  }
}

void save_subscriber_infor(uint8_t cmd,uint8_t *said,uint8_t *taid)//保存定阅者信息
{
    if(REPORT_NO == *judge_data.report_object) return;
    if(CMD_SET != cmd) return;
    get_message_source(said, taid, judge_data.gw_id);
}
void clear_equipment_gid_flag(void)//清除设备GID信息
{
   	judge_data.equipment_gid = 0; 
    judge_data.find_myself = 0;
}

void local_key_report(void)
{
    memset(judge_data.last_id,0x00,ID_LEN);
    memset(judge_data.taker_id,0x00,ID_LEN);
    judge_data.equipment_gid = 0;
	judge_data.find_myself = 0;

}

uint8_t get_1byte_bit1_number(uint8_t data, uint8_t pos)//得到1字节1位数据
{
	uint8_t i;
	uint8_t k = 0;

	for (i = 0; i < pos; i++)
	{
		if (0x01 == (0x01 & data))
		{
			k++;
		}
		data >>= 0x01;
	}
	return k;
}
#endif
//--------------------------------------------------------------------------------------
static void clear_subs(report_type_t type)//清除定阅者
{
	uint8_t i;
	for (i = 0; i< MAX_SUBSCRIBER_NUM; i++)
	{
		if (type == subs_data[i].report_typ)
		{
			memset(&subs_data[i], 0x00, sizeof(subs_data[i]));
		}
	}
}

static void stop_report(report_type_t type)//停止上报--具体实现，清空信息
{
	switch (type)
	{
        
    case POWER_ON:
    case REGISTER:
    clear_subs(type);
    clear_chn(type);
    break;
#if MAX_SENSOR_NUM 
	case FIXED_LENGTH://结束定长上报
		flags.sensor_fixed_length_reporting = FALSE;
		flags.sensor_fixed_length_report_start = FALSE;
        void stop_fixed_reporting(void);
		stop_fixed_reporting();
        //clear_subs(FREQUENCY);
		clear_subs(FIXED_LENGTH);
        clear_chn(FIXED_LENGTH);
		break;
	case FREQUENCY://结束 定频上报
		flags.sensor_fixed_freq_report_start = FALSE;
		clear_subs(FIXED_LENGTH);
		clear_subs(FREQUENCY);
        clear_chn(FREQUENCY);
		break;
#endif

#if MAX_ACTOR_NUM
	case STATE_CHANGE:
		flags.state_report_start = FALSE;
		clear_subs(STATE_CHANGE);
        clear_chn(STATE_CHANGE);
		break;
#endif

#if MAX_ALARM_NUM
	case ALARM:
		flags.alarm_report_start = FALSE;
		clear_subs(ALARM);
        clear_chn(ALARM);
		break;
#endif
	default:
		break;
	}
}

void stop_report_para(uint8_t pos)//停止上报。。。。。。。
{
	stop_report((report_type_t)subs_data[pos].report_typ);
}

static void start_report(report_type_t type)//开始上报 
{
	switch (type)
	{
#if MAX_SENSOR_NUM 
	case FIXED_LENGTH:
		flags.sensor_fixed_length_reporting = TRUE;
		flags.sensor_fixed_length_report_start = TRUE;
        #if SUBSCRIBER_NUM
		sub_pos.fixed_length_pos = 0;
        #endif
		break;
	case FREQUENCY:
		flags.sensor_fixed_freq_report_start = TRUE;
        #if SUBSCRIBER_NUM
		sub_pos.fixed_freq_pos = 0;
        #endif
        int32_t get_min_freq(void);
		delay_data.freq = get_min_freq();
		break;
#endif

#if MAX_ACTOR_NUM
	case STATE_CHANGE:
		flags.state_report_start = TRUE;
        #if SUBSCRIBER_NUM
		sub_pos.state_change_pos = 0;
        #endif
		delay_data.relay_time_count = 2 + 2 * judge_data.equipment_gid;
		break;
#endif

#if MAX_ALARM_NUM
	case ALARM:
        #if SUBSCRIBER_NUM
		sub_pos.alarm_pos = 0;
        #endif
		flags.alarm_report_start = TRUE;
		break;
#endif
	default:
		break;
	}
}

static void send_over(report_type_t type)//发送结束
{
	switch (type)
	{
#if MAX_SENSOR_NUM 
	case FIXED_LENGTH:
		flags.sensor_fixed_length_report_start = FALSE;
		break;
	case FREQUENCY:
		flags.sensor_fixed_freq_report_start = FALSE;
		break;
#endif

#if MAX_ACTOR_NUM
	case STATE_CHANGE:
		flags.state_report_start = FALSE;
		break;
#endif

#if MAX_ALARM_NUM
	case ALARM:
		flags.alarm_report_start = FALSE;
		break;
#endif
	default:
		break;
	}
}

//--------------------------------------------------------------------------------------
static uint8_t sub_report_or_not(uint8_t * sub_id,report_type_t r_type)//是否上报定阅者
{
    
        
    #if SOFT_ACTOR
        return TRUE;
    #endif
   
	if((is_all_xx(sub_id,0x00,ID_LEN)))
    return FALSE;
    
    #if MAX_ACTOR_NUM
    if(STATE_CHANGE == r_type)
    {
        if(!memcmp(sub_id,judge_data.last_id,ID_LEN))
            return FALSE;
    }
    #endif
    return TRUE;
}
extern uint8_t gid[];
static void send_to_gateway(report_type_t r_type)//发送给网关
{
    send_over(r_type);
//	memcpy(judge_data.gw_id,gid,ID_LEN);
    if(!sub_report_or_not(judge_data.gw_id,r_type)) return;
    uint8_t len = peek_chn(r_type,g_frame_buffer,sizeof(g_frame_buffer));
    if(!len) return;
    len = code_frame(judge_data.my_id,judge_data.gw_id,-1,RELIABLE,g_frame_buffer,len,g_frame_buffer,sizeof(g_frame_buffer));//组发送报文。
    send_report_frame(g_frame_buffer,len,r_type);//通过UART发送
    

    PRINTF( "running send_to_gateway!\n");

}

#if (SUBSCRIBER_NUM && MAX_ACTOR_NUM)
static void send_to_only_subs(report_type_t r_type,uint8_t pos)//仅发送给定阅者
{
	if (REPORT_SB != *judge_data.report_object)
    {
        return;
    }

    if (SUBSCRIBER_NUM == pos)
    {
        send_over(r_type);
        return;
    }
    if(!sub_report_or_not(&judge_data.subs_id[ID_LEN * pos],r_type)) return;
	uint8_t len = peek_chn(r_type,g_frame_buffer,sizeof(g_frame_buffer));
    if(!len) return;
    #if MAX_ACTOR_NUM
    if(STATE_CHANGE == r_type) len -= TAKER_DATA;
    #endif
	len = code_frame(judge_data.my_id,&judge_data.subs_id[ID_LEN * pos],-1,UNRELIABLE,g_frame_buffer,len,g_frame_buffer,sizeof(g_frame_buffer));
	send_report_frame(g_frame_buffer,len,r_type);

    PRINTF( "running send_to_only_subs!\n");

}


static void send_to_both_subs_gateway(report_type_t r_type,uint8_t pos)//发送给定阅者和网关
{
//    uint8_t i,sub_counter=0;
    if (!(*judge_data.report_object & REPORT_GW) || !(*judge_data.report_object & REPORT_SB))
    {
        return;
    }
	
	uint8_t len = peek_chn(r_type,g_frame_buffer,sizeof(g_frame_buffer));
    if(!len) return;
    
    if (SUBSCRIBER_NUM == pos)
    {
        send_over(r_type);
        if(!sub_report_or_not(judge_data.gw_id,r_type)) return;
        len = code_frame(judge_data.my_id,judge_data.gw_id,-1,RELIABLE,g_frame_buffer,len,g_frame_buffer,sizeof(g_frame_buffer));
		send_report_frame(g_frame_buffer,len,r_type);

        PRINTF( "running send_to_both_subs_gateway to gateway!\n");

        return;
    }

    else
    {
        if(!sub_report_or_not(&judge_data.subs_id[ID_LEN * pos],r_type)) return;
        #if MAX_ACTOR_NUM
        if(STATE_CHANGE == r_type) len -= TAKER_DATA;
        #endif
        len = code_frame(judge_data.my_id,&judge_data.subs_id[ID_LEN * pos],-1,UNRELIABLE,g_frame_buffer,len,g_frame_buffer,sizeof(g_frame_buffer));
		send_report_frame(g_frame_buffer,len,r_type);

        PRINTF( "running send_to_both_subs_gateway to subscribe!\n");

    }
    
}

static void send_to_only_gateway(report_type_t r_type)//仅发送给网关
{
	if (REPORT_GW != *judge_data.report_object) return;
    send_to_gateway(r_type);
    
}
#endif

static void send_power_or_register_report(report_type_t r_type)//发送上电或注册上报
{
	get_report_data_callback(r_type);
	uint8_t len = peek_chn(r_type,g_frame_buffer,sizeof(g_frame_buffer));
    if(!len) return;
	len = code_frame(judge_data.my_id,judge_data.gw_id,-1,RELIABLE,g_frame_buffer,len,g_frame_buffer,sizeof(g_frame_buffer));
	send_report_frame(g_frame_buffer,len,r_type);
	if((POWER_ON == r_type) && (is_all_xx(judge_data.gw_id,0x00,ID_LEN)))
	{
		finish_power_on_report();
	}
}

//--------------------------------------------------------------------------------------
void register_report(uint8_t gateway_id[])//注册上报
{
#if MAX_SENSOR_NUM
	delay_data.register_time_count = 15;
	flags.register_report = TRUE;
#else
#if MAX_ACTOR_NUM
	delay_data.power_on_time_count = 15;
    flags.power_on_report = UNFINISHED;
    flags.dev_state= IDLE;

#endif
    
#endif
	memset(subs_data,0x00,sizeof(subs_data));
#if SUBSCRIBER_NUM
    memset(judge_data.subs_id,0x00,sizeof(judge_data.subs_id));
	judge_data.subs_pos = 0;
#endif
    memcpy(judge_data.gw_id, gateway_id, ID_LEN);
}

static void actor_alarm_data_init(void)
{
	#if MAX_ACTOR_NUM
	    init_relay_data();
	#endif
                         
	#if MAX_ALARM_NUM
	    init_alarm_data();                
	#endif
}

static void register_time_count(void)//注册时间计数
{
	if (TRUE != flags.register_report)
	{
		return;
	}
	delay_data.register_time_count--;
	if (delay_data.register_time_count <= 0)
	{
		actor_alarm_data_init();
        send_power_or_register_report(REGISTER);
		flags.register_report = FALSE;
	}
}

//--------------------------------------------------------------------------------------
static void poweron_time_count(void)//上电时间计数
{
	if (UNFINISHED != flags.power_on_report)
	{
		return;
	}
	delay_data.power_on_time_count--;
	if ((delay_data.power_on_time_count <= 0) && (IDLE == flags.dev_state))//上电计数<=0&&设备状态为空闲
	{
		stop_report(REGISTER);//停止注册上报
        flags.dev_state = WAITING;
		actor_alarm_data_init();
		send_power_or_register_report(POWER_ON);
	}
}
//--------------------------------------------------------------------------------------
#if MAX_ACTOR_NUM
relay_data_t relay_data[MAX_ACTOR_NUM];//u8数据类型
uint8_t relay_data_bak[MAX_ACTOR_NUM];

u16_relay_data_t u16_relay_data[1];//U16数据类型
uint16_t u16_relay_data_bak[1];
static void send_actor_count(void)
{
//	if(FINISHED!=flags.power_on_report) return;
    if (TRUE != flags.state_report_start) return;

	delay_data.relay_time_count--;
	if (delay_data.relay_time_count > 0)
	{
		return;
	}
	
	#if SUBSCRIBER_NUM
    send_to_both_subs_gateway(STATE_CHANGE, sub_pos.state_change_pos);
    send_to_only_gateway(STATE_CHANGE);
    send_to_only_subs(STATE_CHANGE, sub_pos.state_change_pos);
    sub_pos.state_change_pos++;

    PRINTF( "sub_pos.state_change_pos is %d\n",sub_pos.state_change_pos);

	#else
    send_to_gateway(STATE_CHANGE);
	#endif
}

static void check_actor_data(void)
{
	uint8_t k = 0;
    
	for(uint8_t i = 0; i < MAX_ACTOR_NUM-1;i++)
	{
		if (*relay_data[i].cur == relay_data_bak[i]) 
			k++;
		else
			relay_data_bak[i] = *relay_data[i].cur;
	}
    for(uint8_t i =0; i < 1;i++)
	{
		if (*u16_relay_data[i].cur == u16_relay_data_bak[i]) 
			k++;
		else
			u16_relay_data_bak[i] = *u16_relay_data[i].cur;
	}
  
	if(MAX_ACTOR_NUM == k)	return;
	
	stop_report(STATE_CHANGE);
	start_report(STATE_CHANGE);
    get_report_data_callback(STATE_CHANGE);


}
#endif

//--------------------------------------------------------------------------------------
#if MAX_ALARM_NUM
relay_data_t alarm_data[MAX_ALARM_NUM];
uint8_t alarm_data_bak[MAX_ALARM_NUM];
static void send_alarm_count(void)
{
	if (TRUE != flags.alarm_report_start) return;

	
	#if (SUBSCRIBER_NUM && ALARM_REPORT_SB)
    send_to_both_subs_gateway(ALARM, sub_pos.state_change_pos);
    send_to_only_gateway(ALARM);
    send_to_only_subs(ALARM, sub_pos.state_change_pos);
    sub_pos.alarm_pos++;
	#else
    send_to_gateway(ALARM);
	#endif
}

static void check_alarm_data(void)
{
	uint8_t k = 0;
    
	for(uint8_t i = 0; i < MAX_ALARM_NUM;i++)
	{
		if (*alarm_data[i].cur == alarm_data_bak[i]) 
			k++;
		else
			alarm_data_bak[i] = *alarm_data[i].cur;
	}

	if(MAX_ALARM_NUM == k)	return;
	
	stop_report(ALARM);
	start_report(ALARM);
    get_report_data_callback(ALARM);
}
#endif

//--------------------------------------------------------------------------------------
#if MAX_SENSOR_NUM

sensor_data_t sensor_data[MAX_SENSOR_NUM];

void stop_fixed_reporting(void)//停止固定上报
{
	for (uint8_t i = 0; i < MAX_SENSOR_NUM; i++)
	{
		if (UNKNOWN != sensor_data[i].sensor_type)
		{
			sensor_data[i].fixed_len_reporting = FALSE;
		}
	}
}

int8_t get_sensor_from_type(data_type_t data_type)//取传感器类型
{
    uint8_t i;
    for (i = 0; i < MAX_SENSOR_NUM; i++)
    {
        if (data_type == sensor_data[i].sensor_type)
            return i;
    }
    return -1;
}

void send_fixed_len_count(void)//second task  定长上报
{
    if (TRUE != flags.sensor_fixed_length_report_start)
    {
        return;
    }
    
#if (SUBSCRIBER_NUM && SENSOR_REPORT_SB)
    send_to_both_subs_gateway(FIXED_LENGTH, sub_pos.fixed_length_pos);
    send_to_only_gateway(FIXED_LENGTH);
    send_to_only_subs(FIXED_LENGTH, sub_pos.fixed_length_pos);
    sub_pos.fixed_length_pos++;
#else
	send_to_gateway(FIXED_LENGTH);
#endif
}

static void start_fixed_report_save(uint8_t sensor_pos,int32_t val)//开始固定上报存储
{
    sensor_data[sensor_pos].last_report = val;
    sensor_data[sensor_pos].fixed_len_reporting = TRUE;
}

static void stop_fixed_report_save(uint8_t sensor_pos)//停止定长上报保存
{
    sensor_data[sensor_pos].fixed_len_reporting = FALSE;
}

static uint8_t start_new_report_or_not(int32_t cur_val, int32_t step_val, int32_t base_val, int32_t window)//是否开始新的上报
{
    if(DIGITAL_WIN == window)
    {
      if(cur_val != base_val)
      {

        PRINTF( "Digital data step report new start!!\n");

        return 1;
      }        
    }
    else
    {
        
        if(((cur_val >= base_val + step_val) || (cur_val <= base_val - step_val)))
        {

          PRINTF( "Analog data step report new start!\n");

          return 1;
        }
    }
    return  0;
}

static report_process_t sensor_need_report_or_not(uint8_t pos)//传感器是否需要上报
{
    int32_t temp_max,temp_min;
    int32_t cur = sensor_data[pos].sensor_cur;
    int32_t step = sensor_data[pos].sensor_step;
   // if(FINISHED!=flags.power_on_report) return IDLE;
    if(0x00 == step) return STOP;
    if (((TRUE == sensor_data[pos].fixed_len_reporting))&&(TRUE == flags.sensor_fixed_length_reporting))
    {
        if(DIGITAL_WIN == sensor_data[pos].sensor_window)
        {
          if(cur == sensor_data[pos].sensor_base) 
            {

                PRINTF( "Digital data step report need stop!\n");

                stop_fixed_report_save(pos);
                return STOP;
            }
            if(cur != sensor_data[pos].last_report)
            {                

                PRINTF( "Digital data step report new start!\n");

                start_fixed_report_save(pos,cur);
                return SETTING;
            }
        }            
        else
        {
            temp_max = max(cur, sensor_data[pos].last_report);
            temp_min = min(cur, sensor_data[pos].last_report);
            if ((cur < sensor_data[pos].sensor_base + step) && (cur > sensor_data[pos].sensor_base - step))
            {

                PRINTF( "Analog data step report need stop!\n");

                stop_fixed_report_save(pos);
                return STOP;
            }
            else if (sensor_data[pos].sensor_window < temp_max - temp_min)
            {

                PRINTF( "Analog data step report new start!\n");

                start_fixed_report_save(pos,cur);
                return SETTING;
            }
            
        } 
        return IDLE;        
    }
    else
    {
        if(start_new_report_or_not(cur,step,sensor_data[pos].sensor_base,sensor_data[pos].sensor_window))
        {
            start_fixed_report_save(pos,cur);
            return SETTING;
        }
    }
    stop_fixed_report_save(pos);
    return STOP;
}

void check_fixed_len_data(void)//检查定长数据
{
    uint8_t i, j = 0;
    static uint8_t array[MAX_SENSOR_NUM];

    for (i = 0; i < MAX_SENSOR_NUM; i++)
    {
        if (0x00 == sensor_data[i].sensor_step) j++;
    }
    if (MAX_SENSOR_NUM == j)    //步长为0，为关闭上报
    {
        clear_subs(FIXED_LENGTH);
        clear_chn(FIXED_LENGTH);
        return;/*it means step = 0,no need to report by step*/
    }

    for (i = 0; i < MAX_SENSOR_NUM; i++)//MAX_SENSOR_NUM=2
    {
        array[i] = sensor_need_report_or_not(i);//检测传感器是否需要上报
    }
    
    for (i = 0; i < MAX_SENSOR_NUM; i++)
    {
        if (SETTING == array[i])
        {
            clear_subs(FREQUENCY);
			clear_chn(FREQUENCY);
            clear_subs(FIXED_LENGTH);
            clear_chn(FIXED_LENGTH);
            start_report(FIXED_LENGTH);
            get_report_data_callback(FIXED_LENGTH);
            return;
        }
    }
    for (i = 0; i < MAX_SENSOR_NUM; i++)
    {
        if (IDLE == array[i])
        {
            return;
        }
    }
    stop_report(FIXED_LENGTH);
}
//--------------------------------------------------------------------------------------
int32_t get_min_freq(void)//得到最小上报频率
{
    uint8_t i;
    int32_t freq = 0,sensor_freq = 0;

    for (i = 0; i < MAX_SENSOR_NUM; i++)
    {
	    sensor_freq = sensor_data[i].sensor_freq;
        if ((0 == sensor_freq)||(0 == freq))
        {
            freq = max(freq, sensor_freq);
        }
        else
        {
            freq = min(freq, sensor_freq);
        }
    }
    return freq;
}

void reload_freq_infor(void)//重载频率
{
    init_sensor_data();
    delay_data.freq = get_min_freq();
}

void send_fixed_fre_count(void)//task
{
    if (TRUE != flags.sensor_fixed_freq_report_start)
    {
        return;
    }
#if (SUBSCRIBER_NUM && SENSOR_REPORT_SB)
    send_to_both_subs_gateway(FREQUENCY, sub_pos.fixed_freq_pos);
    send_to_only_gateway(FREQUENCY);
    send_to_only_subs(FREQUENCY, sub_pos.fixed_freq_pos);
    sub_pos.fixed_freq_pos++;
#else
	send_to_gateway(FREQUENCY);
#endif
}

void check_fixed_fre_data(void)//检测固定频率数据
{
    uint8_t i, j = 0;

    for (i = 0; i < MAX_SENSOR_NUM; i++) 
    {
        if (0x00 == sensor_data[i].sensor_freq)
        {
            j++;
        }
    }
    if (MAX_SENSOR_NUM == j)/*it means freq = 0,no need to report by freq*/
    {
		clear_subs(FREQUENCY);
        clear_chn(FREQUENCY);
        return;
    }
    delay_data.freq--;
    if (delay_data.freq > 0)
    {
        return;
    }

    PRINTF( "new frequecy report start!\n");

    stop_report(FREQUENCY);
    start_report(FREQUENCY);
    get_report_data_callback(FREQUENCY);
}

#endif

//--------------------------------------------------------------------------------------
static uint16_t get_different_delay_time(uint8_t sid[])//得到不同的延时时间
{
	uint16_t group_sid;

	group_sid = (sid[1] << 8) + sid[0];
	if ((delay_data.freq > 0) && (delay_data.freq < POWER_ON_COMPARE_DATA))
	{
		return (group_sid % delay_data.freq);
	}
	return (group_sid % POWER_ON_COMPARE_DATA);
}

void auto_report_init(uint32_t invalid_time, uint8_t gateway_id[], uint8_t my_id[], uint8_t sid[], uint8_t *report_object)//自动上报初始化
{

#if MAX_ACTOR_NUM
    init_relay_data();
#endif
                         
#if MAX_ALARM_NUM
    init_alarm_data();                
#endif

#if MAX_SENSOR_NUM
    init_sensor_data();
    delay_data.freq = get_min_freq();
#endif
    
    judge_data.default_report = REPORT_GW;
    invalid_time=dev.power_on_delay;
    delay_data.power_on_time_count = invalid_time + get_different_delay_time(sid);//无效时间+根据SID计算的不同的延时时间   上电上报时间
//    delay_data.power_on_time_count = 10;//无效时间+根据SID计算的不同的延时时间，测试用。
    flags.power_on_report = UNFINISHED;
    flags.dev_state = IDLE;
//	memcpy(gateway_id,gid,ID_LEN);
    memcpy(judge_data.gw_id, gateway_id, ID_LEN);
    memcpy(judge_data.my_id, my_id, ID_LEN);
    judge_data.report_object = (!report_object) ? &judge_data.default_report : report_object;
    report_alloter_init();
}
//--------------------------------------------------------------------------------------
static void judge_process(void)//判定处理
{
    if(REPORT_NO == *judge_data.report_object)
		return;

//    if(UNFINISHED == flags.power_on_report)
//        return;
#if MAX_ACTOR_NUM
	check_actor_data();
#endif

#if MAX_SENSOR_NUM
    init_sensor_data();
    check_fixed_len_data();
    check_fixed_fre_data();
#endif

#if MAX_ALARM_NUM
	check_alarm_data();
#endif

	return;
}

void auto_report_sec_task(void)//自动上报秒任务
{
	report_time_hook();
	judge_process();
	poweron_time_count();
    register_time_count();

#if MAX_ACTOR_NUM
	send_actor_count();
#endif

#if MAX_SENSOR_NUM
	send_fixed_len_count();//定长上报
	send_fixed_fre_count();//定频上报
#endif

#if MAX_ALARM_NUM
	send_alarm_count();
#endif
}
