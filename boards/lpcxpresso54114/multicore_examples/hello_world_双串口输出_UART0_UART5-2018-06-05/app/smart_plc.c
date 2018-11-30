#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "alloter.h"
#include <utils.h>
#include <stddef.h>
#include <board.h>
#include "app_dev_ctrl.h"
#include "smart_plc.h"
#include "pin_mux.h"
#include "fsl_usart.h"
#include "fsl_device_registers.h"
#include "fsl_flexcomm.h"
#include "fsl_usart_cmsis.h"
#include "fsl_debug_console.h"
#include "fsl_iocon.h"
#include "protocol_smart.h"
#include "app_spiflash.h"
#include "app_lcd.h"
#include "auto_report.h"
#include "auto_report_app.h"
extern uint8_t g_frame_buffer[0x400];
extern struct dev dev; 
struct PLC_MACHINE  plc_state;
//static uint8_t rst_plc_t = 0;
extern uint8_t plc_alive_flag;
#define next_state()            do { chg_state(plc_state.pstate->next_state); } while (0)
#define try_state_again()       do { chg_state(plc_state.pstate->cur_state); plc_state.trycnt++; } while (0)
#define clear_rst_time(pframe)  do { if (CMD_ACK_EID == pframe->infor[0]) rst_plc_t = 0; } while (0)

extern struct REG reg;
#define MAX_OVERTIME    (plc_state.trycnt + 5)
#define MAX_TRYCNT      2
#define MAX_RST_TIME    3
#define MIN_PLC_RUN 2

void send_local_frame(uint8_t buffer[], uint8_t len)
{
    struct SmartFrame *pframe = (struct SmartFrame *)buffer;

    memmove(&buffer[offsetof(struct SmartFrame, data)], buffer, len);
    pframe->stc = STC;
    memset(pframe->said, 0x00, ID_LEN);
    memset(pframe->taid, 0x00, ID_LEN);
    pframe->seq        = 0;
    pframe->len        = len;
    pframe->data[len] = checksum((uint8_t *)pframe, SMART_FRAME_HEAD + len);
    uart_write(0, buffer, SMART_FRAME_HEAD + len + 1);
	
}
uint8_t local_ack_opt(struct SmartFrame *pframe)
{
    if (pframe)
    {
        if (!is_all_xx(pframe->said, 0x00, ID_LEN) || !is_all_xx(pframe->taid, 0x00, ID_LEN))
            return (0);

        uint8_t cmd = pframe->data[0];
        if (cmd == CMD_ACK || cmd == CMD_NAK)
        {
            next_state();
			PRINTF("NEXT STATE!\r\n");
            return (1);
        }
    }
    if (plc_state.wait_t > MAX_OVERTIME)
	{
		try_state_again();
		PRINTF("TIME OUT TRY STATE AGAIN!\r\n");
	}
        
    return (0);
}

static uint8_t reset_plc(uint8_t init, void *args)
{
	if(init)
	{
		plc_reset_off();
		PRINTF("plc_reset_off\r\n");
		return 0;
	}
	if(plc_state.wait_t>=MIN_PLC_RUN)
	{
		next_state();
		PRINTF("from plc reset on to mext state\r\n");
	}
		
	else if(plc_state.wait_t >= 1)
	{
		plc_reset_on();
		PRINTF("plc_reset_on\r\n");
	}
		
	return 0;
}
void get_id(uint8_t type, uint8_t buffer[])
{
    buffer[0] = type;
    send_local_frame(buffer, 1);
	
}
static int check_sole_encode(void)
{
    if (!is_all_xx(dev.encode.id, 0x00, ID_LEN)
        && !is_all_xx(dev.encode.id, 0xFF, ID_LEN))
	{
		PRINTF("already have AID\r\n");
		return (1);
	}
    return (0);
}
static uint8_t rd_plc_eid(uint8_t init, void *args)
{
	struct SmartFrame *pframe = (struct SmartFrame *)args;
	if(init)
	{
		if(check_sole_encode())
		{
			next_state();
			PRINTF("dev has eid or aid.\r\n");
            return (0);
		}
		get_id(CMD_GET_EID, g_frame_buffer);
		PRINTF("send get eid protocol.\r\n");
	}
	else
	{
		if ((pframe != NULL) && (CMD_ACK_EID == pframe->data[0]))
        {
			if (0x00 != memcmp(dev.encode.id, &pframe->data[1], ID_LEN))
            {
                memcpy(dev.encode.id, &pframe->data[1], ID_LEN);
				PRINTF("arealdy get eid.\r\n");
            }
            next_state();
            return (0);
		}
		if (plc_state.wait_t > MAX_OVERTIME)
            try_state_again();
	}
	return (0);
}
static void set_aid(uint8_t buf[])
{
    uint8_t len = 0;

    buf[len++] = CMD_SET_AID;
    memcpy(buf + len, dev.encode.id, ID_LEN);
    len += ID_LEN;
    send_local_frame(buf, len);
	PRINTF("send set_aid protocol.\r\n");
}
static uint8_t wr_plc_aid(uint8_t init, void *args)
{
	if (init)
    {
        set_aid(g_frame_buffer);
        return (0);
    }

    local_ack_opt((struct SmartFrame *)args);
	PRINTF("receive write plc aid ack.\r\n");
    return (0);
}
static uint8_t do_set_unlink(uint8_t buffer[])
{
	buffer[0] = CMD_UNLINK;

    send_local_frame(buffer, 1);
	PRINTF("send set unlink protocol.\r\n");

}
static uint8_t set_unlink(uint8_t init, void *args)
{
    if (init)
    {
        do_set_unlink(g_frame_buffer);
        return (0);
    }

    local_ack_opt((struct SmartFrame *)args);
	PRINTF("receive set unlink ack\r\n");
    return (0);
}
static uint8_t wait_sec(uint8_t init, void *args)
{
	if (plc_state.wait_t > 1)
	{
		next_state();
		PRINTF("WAIT SEC FINISH, CHANGE TO NEXT SATAE\r\n");
	}
        
    return (0);
}
static void do_plc_register(uint8_t buffer[])
{
    uint8_t len = 0;

    buffer[len++]   = CMD_SET_REG;
    buffer[len++]   = reg.type | reg.last_status;
    reg.last_status = 0;
    send_local_frame(buffer, len);
	PRINTF("SEND DID=%02X PROTOCOL!\r\n");
	PRINTF("do_plc_register\r\n");
}
static uint8_t set_register(uint8_t init, void *args)
{
	if (init)
    {
        do_plc_register(g_frame_buffer);
        return (0);
    }
    local_ack_opt((struct SmartFrame *)args);
	PRINTF("receive set register ack\r\n");
//		sensor_infor.networking_state = _ON;
    return (0);
}
static uint8_t set_panid(uint8_t buf[])
{
    uint8_t len = 0;

    if (VALID_DATA != dev.panid_valid)
	{
		PRINTF("VALID_DATA != dev.panid_valid \r\n");
		return (0);
	}
        

    buf[len++] = CMD_SET_PANID;
    memcpy(buf + len, dev.panid, PANID_LEN);
    len += PANID_LEN;
    send_local_frame(buf, len);
	PRINTF("SEND SET PANID PROTOCOL\r\n");
    return (1);
}
static uint8_t wr_plc_panid(uint8_t init, void *args)
{
	if (init)
    {
        uint8_t ret = set_panid(g_frame_buffer);
        if (!ret)
		{
			next_state();
			PRINTF("FROM WIRTE PLC PANID TO NEX STATE\r\n");
			plc_alive_flag=1;//载波流程走完，PLC在线标志为有效
			display_signal(PLC);//显示图标
		}
            
        return (0);
    }
	plc_alive_flag=1;//载波流程走完，PLC在线标志为有效
	display_signal(PLC);//显示图标
    local_ack_opt((struct SmartFrame *)args);
	PRINTF("RECEIVE WIRTE PLC PANID PROTOCOL\r\n");
    return (0);
}
static uint8_t rd_gw_aid(uint8_t init, void *args)
{
	struct SmartFrame *pframe = (struct SmartFrame *)args;
//	uint8_t i;
    if (init)
    {
        get_id(CMD_GET_GWAID, g_frame_buffer);
		PRINTF("SEND GET  GW AID PROTOCOL\r\n");
        return (0);
    }

    if (pframe && pframe->data[0] == CMD_GET_GWAID)
    {
        uint8_t *gid = &pframe->data[1];
        if (memcmp(dev.gid, gid, ID_LEN))
        {
            memcpy(dev.gid, gid, ID_LEN);
//			for(i=0;i<ID_LEN;i++)
//			{
//				PRINTF("GATEWAY GID =%02X \r\n",*(gid+i));
//			}
            write_to_flash();
			mx25r_cmd_read(&mx25r,PARAM_ADDR,(uint8_t *)&dev,sizeof(dev));
//			for(i=0;i<ID_LEN;i++)
//			{
//				PRINTF("GATEWAY GID =%02X \r\n",*(gid+i));
//			}
        }
		PRINTF("FROM READ GW AID TO NEX STATE\r\n");
        next_state();
        return (0);
    }
    if (plc_state.wait_t > MAX_OVERTIME)
        try_state_again();
    return (0);
}
static uint8_t rd_plc_sid(uint8_t init, void *args)
{
	struct SmartFrame *pframe = (struct SmartFrame *)args;

    if (init)
    {
        get_id(CMD_GET_SID, g_frame_buffer);
		PRINTF("SEND GET SID PROTOCOL\r\n");
        return (0);
    }

//    flash_read(PARAM_ADDR, (uint8_t *)&eep_param, sizeof(eep_param));
	mx25r_cmd_read(&mx25r,PARAM_ADDR,(uint8_t *)&dev,sizeof(dev));
    if (pframe && pframe->data[0] == CMD_ACK_SID)
    {
        uint8_t *sid = &pframe->data[1];
        if (memcmp(dev.sid, sid, SID_LEN))
        {
            memcpy(dev.sid, sid, SID_LEN);
            write_to_flash();
        }
		register_report(dev.gid);//注册上报
        next_state();
		PRINTF("FROM READ PLC SID TO END\r\n");
		
        return (0);
    }
	plc_alive_flag=1;//载波流程走完，PLC在线标志为有效
	display_signal(PLC);//显示图标
    if (plc_state.wait_t > MAX_OVERTIME)
        try_state_again();
    return (0);
}
static uint8_t get_reg(struct SmartFrame *pframe)
{
    uint8_t need_wr = 1;

    if (PASSWORD_REG == reg.type)
    {
        if (VALID_DATA != dev.panid_valid)
        {
            // first time register, do not check password
        }
        else if ((VALID_DATA == dev.panid_valid) && (!memcmp(dev.panid, &pframe->data[1], PANID_LEN)))
        {
            // reregister, check panid
            need_wr = 0;
        }
        else if (memcmp(dev.encode.pwd, &pframe->data[PANID_LEN + 1], PW_LEN))
        {
            reg.last_status = PASSWORD_ERR;
            chg_state(UNLINK2);
			PRINTF("FROM get_reg TO UNLINK2\r\n");
            return (0);
        }
    }

    reg.type = PASSWORD_REG;
    chg_state(G_GWID);
    if (need_wr)
    {
        memcpy(dev.panid, &pframe->data[1], PANID_LEN);
        dev.panid_valid = VALID_DATA;
        write_to_flash();
		PRINTF("WRITE PADID TO FALSH\r\n");
    }

    return (0);
}

uint8_t set_ret_frame(struct SmartFrame *pframe, uint8_t len)
{
    memcpy(pframe->taid, pframe->said, ID_LEN);
    memcpy(pframe->said, dev.encode.id, ID_LEN);

    pframe->seq |= 0x80;

    pframe->len        = len;
    pframe->data[len] = checksum(pframe, pframe->len + SMART_FRAME_HEAD);
    return (pframe->len + SMART_FRAME_HEAD + 1);
}
uint8_t is_gid_equal(const uint8_t *data)
{
    uint8_t dlen       = get_bits(data[0], 0, 5);
    uint8_t group_type = get_bits(data[0], 6, 7);
    uint16_t gid       = get_le_val(dev.sid, sizeof(dev.sid));
    
	#if MAX_ACTOR_NUM
    uint8_t k,connt_size = 0;
    uint16_t j = 0;
    k = (gid%8) ? (gid%8):8;
	#endif
	
    data++;

    if (group_type == 0) /* bit type */
    {
        gid--;
        if (dlen < (gid >> 3) + 1)
	    {
			#if MAX_ACTOR_NUM
				goto deal_find_none;
			#else
			    return (0);
			#endif
		}         
        if (tst_bit(data[gid >> 3], gid & 0x07))
		{
		#if MAX_ACTOR_NUM
			for (j = 0;j < (gid>>3);j++)
			{
				connt_size += get_1byte_bit1_number(data[j],8);
			}
			connt_size += get_1byte_bit1_number(data[(gid>>3)],k);
			if (0 == judge_data.find_myself) 
			{
				judge_data.equipment_gid = connt_size; 
			}
			judge_data.find_myself = 1;
		#endif
			return (1); 
		}
    }
    else                 /* bytes type */
    {
        uint8_t i;
        uint8_t gid_unit_len = (group_type == 1 ? 1 : 2);

        for (i = 0; i < dlen; i += gid_unit_len)
        {
            uint16_t _gid = get_le_val(data + i, gid_unit_len);
            if (_gid == gid)
			{
			#if MAX_ACTOR_NUM
				if(0 == judge_data.find_myself)
				{
					judge_data.equipment_gid = connt_size + 1;
				}
				judge_data.find_myself = 1;
			#endif
				return (1);
			} 
            else if(_gid == 0)			
			{
            #if MAX_ACTOR_NUM
			    judge_data.equipment_gid += gid; 
			    judge_data.find_myself = 1;
		    #endif
				return(1);
			}
            #if MAX_ACTOR_NUM
                connt_size++;
            #endif
        }		
    }
#if MAX_ACTOR_NUM
deal_find_none:
	if(0 == judge_data.find_myself)
	{
	    if(0x00 == group_type)
	    {
			for (j = 0;j < dlen;j++)
			{
				judge_data.equipment_gid += get_1byte_bit1_number(data[j],8);
			}
		}
		else if(0x01 == group_type)
		{
			judge_data.equipment_gid += dlen;
		}
		else if(0x02 == group_type)
		{
			judge_data.equipment_gid += dlen/2;
		}
	}
#endif
  return (0);
}
uint8_t set_group_parameter(uint8_t data[], uint8_t len)
{
    uint8_t inidx = 0, gid_len;

    gid_len = get_bits(data[inidx], 0, 5) + 1;
    while (len >= FBD_FRAME_HEAD + gid_len)
    {
        struct Body *body = (struct Body *)&data[inidx + gid_len];
        uint8_t body_len  = gid_len + FBD_FRAME_HEAD + get_bits(body->ctrl, 0, 6);

        if (len < body_len)
            break;

        if (is_gid_equal(&data[inidx]))
        {
            struct func_ops *op = get_option(body->did);
            if (op && op->write)
                op->write(body->data, get_bits(body->ctrl, 0, 6), body->data, 128);
        }

        inidx += body_len;
        len   -= body_len;

        gid_len = get_bits(data[inidx], 0, 5) + 1;
    }
	if(0 == judge_data.find_myself)
    {
        judge_data.equipment_gid = 0;
    }
    return (0);
}

void local_frame_opt(struct SmartFrame *pframe)
{
    uint8_t cmd = pframe->data[0];

    switch (cmd)
    {
    case CMD_REQ_AID:
        /* plc get aid if reset */
        chg_state(S_AID);
        break;
    case CMD_UNLINK:
        /* gateway unlink */
        chg_state(S_PWDREG);
        break;
    case CMD_REGINFOR:
        get_reg(pframe);
        break;
    case CMD_GET_GWAID:
        if (memcmp(dev.gid, &pframe->data[1], ID_LEN))
        {
            memcpy(dev.gid, &pframe->data[1], ID_LEN);
            write_to_flash();
        }
        break;
	case CMD_ACK:
    case CMD_NAK:
      plc_alive_flag =true;
      break;
    default:
        break;
    }
}
//typedef enum _report_attribute_t
//{
//	UNRELIABLE = 0,
//	RELIABLE
//}report_attribute_t;
//void report_frame_ack(uint8_t seq, uint8_t source_addr[], uint8_t *fbd)
//{
//	
//}
uint8_t remote_frame_opt(struct SmartFrame *pframe)
{
    uint8_t ret = 0;
    uint8_t cmd, dlen, *body;

    if (pframe->len == 0)
    {
        return (0);
    }
//    if(TRUE == dev_search_param.dev_show_flag) 
//	{
//		return (0);
//	}
    if (pframe->seq & 0x80)
    {
        if(RELIABLE == (pframe->data[0] & 0x07))
        {
            report_frame_ack(pframe->seq,pframe->said,&pframe->data[1]);
//            printf("REPORT_ACK: ");
//            uart_write(UART_CHN_DEBUG, pframe, (pframe->len + SHS_FRAME_HEAD));
            return (0);
        }
    }
        

    dlen = pframe->len - 1;
    cmd  = pframe->data[0] & 0x07;
    body = pframe->data + 1;
    switch (cmd)
    {
    case CMD_SET:
		#if MAX_ACTOR_NUM
        clear_equipment_gid_flag();
        #endif
    case CMD_GET:
        if (CMD_SET == cmd && is_all_xx(pframe->taid, 0xFF, ID_LEN))
		{
            set_group_parameter(body, dlen);
//            #if MAX_ACTOR_NUM
//            save_subscriber_infor(cmd,pframe->said,pframe->taid);
//            #endif
		}
        else
		{
            ret = do_cmd(cmd, body, dlen);
//            #if MAX_ACTOR_NUM
//            save_subscriber_infor(cmd,pframe->said,pframe->taid);
//            #endif
		}
        break;
    case CMD_UPDATE:
//        ret = update_frame_opt(body, dlen);
        break;
	case CMD_SHOW:
//		ret = search_frame_opt(pframe);
		break;
    }
    return (ret + 1);             //add cmd
}

static uint8_t frame_handle(uint8_t init, void *args)
{
	    struct SmartFrame *pframe = (struct SmartFrame *)args;
	
    if (!pframe)
        return (0);

    if (is_all_xx(pframe->said, 0x00, ID_LEN))
    {
        /* we got a frame from plc */
        local_frame_opt(pframe);
		PRINTF("LOCAL PROTOCOL \r\n");
    }
    else
    {
		
        /* we got a frame from gateway or else remote device */
        uint8_t len = remote_frame_opt(pframe);
//        printf("MCU_R: ");
//        //uart_write(UART_CHN_DEBUG, pframe, len);  //print recieve message
//		    put_array_with_len((uint8_t *)pframe, len);
        if (len > 1)
        {
            len = set_ret_frame(pframe, len);
            uart_write(0, pframe, len);
			PRINTF("SEND REMOTE PROTOCOL \r\n");
//            printf("MCU_T: ");
//            //uart_write(UART_CHN_DEBUG, pframe, len);  //print send message
//				    put_array_with_len((uint8_t *)pframe, len);
        }
    }
    return (0);
}
struct PLC_STATE plc_state_slot[] =
{
   //init plc
   { RST_PLC,   R_EID,   reset_plc },
   { R_EID,     S_AID,   rd_plc_eid },
   { S_AID,     UNLINK2,  wr_plc_aid },
   { UNLINK2,   WAIT,     set_unlink },
   { WAIT,      S_PWDREG, wait_sec }, //20151105载波复位，载波网络断开
   { S_PWDREG,  S_PANID, set_register },
   { S_PANID,   _END,    wr_plc_panid },
#if KEY_REG
//   { UNLINK1,   S_REG,   set_unlink },
#endif
   { G_GWID,    S_REG,   rd_gw_aid },
   { S_REG,     G_SID,   set_register }, //设置注册属性，等待载波芯片上报panid
   { G_SID,     _END,    rd_plc_sid },
   { _END,      _END,    frame_handle },
};
#define   PLC_SLOT_SZ         (sizeof(plc_state_slot)/sizeof(plc_state_slot[0]))
static struct PLC_STATE *get_plc_state(uint8_t state)
{
    uint8_t i;

    for (i = 0; i < array_size(plc_state_slot); i++)
    {
        if (plc_state_slot[i].cur_state == state)
            return (&plc_state_slot[i]);
    }
    return (plc_state_slot);
}

void chg_state(uint8_t cur_state)
{
    static uint8_t last_state = INVALID;

    plc_state.wait_t = 0;
    plc_state.init   = 1;

    plc_state.pstate = get_plc_state(cur_state);
    if (last_state != plc_state.pstate->cur_state)
        plc_state.trycnt = 0;
    last_state = plc_state.pstate->cur_state;
    notify(EV_STATE);
}
void plc_machine_opt(void *args)
{
   struct PLC_STATE *pstate = plc_state.pstate;
   uint8_t init;

   init = plc_state.init;
   if (init) plc_state.wait_t = 0;
   plc_state.init = 0;
   if (NULL != pstate)
   {
      pstate->action(init, args);
   }
}

uint32_t USART5_GetFreq(void)
{
    return CLOCK_GetFreq(kCLOCK_Flexcomm5);
}
//#define DEMO_USART Driver_USART5
#define ECHO_BUFFER_LENGTH 8
uint8_t g_tipString[] = "USART DMA example\r\nSend back received data\r\nEcho every 8 characters\r\n";
uint8_t g_txBuffer[ECHO_BUFFER_LENGTH] = {0};
uint8_t g_rxBuffer[ECHO_BUFFER_LENGTH] = {0};
volatile bool rxBufferEmpty = true;
volatile bool txBufferFull = false;
volatile bool txOnGoing = false;
volatile bool rxOnGoing = false;

/* USART user callback */

uint8_t receive_String[] = "USART DMA received data\r\n";
uint8_t send_String[] = "USART DMA send data\r\n";
//void USART_Callback(uint32_t event)
//{
//    if (event == ARM_USART_EVENT_SEND_COMPLETE)
//    {
//        txBufferFull = false;
//        txOnGoing = false;
//    }

//    if (event == ARM_USART_EVENT_RECEIVE_COMPLETE)
//    {
////        rxBufferEmpty = false;
////        rxOnGoing = false;
//		notify(EV_PLC);
//    }
//}
uint8_t bufferString[50] = {0};
uint8_t receive_flag = 0;			//收到M0+数据标志位
uint8_t pc_receive_index = 0;  //记录接收的字符数
uint8_t pc_receive_flag = 0;
void FLEXCOMM5_IRQHandler(void)
{
    uint8_t data;
    /* If new data arrived. */
    if ((kUSART_RxFifoNotEmptyFlag | kUSART_RxError) & USART_GetStatusFlags(USART5))
    {
        data = USART_ReadByte(USART5);
		{
//////			bufferString[pc_receive_index] = data;
//////			if (bufferString[pc_receive_index] == '#')
//////			{
//////				pc_receive_flag = 1;
//////				USART_ClearStatusFlags(USART5,USART_FIFOSTAT_RXERR_MASK);
//////			}
//////			pc_receive_index++;
			if (uart_push_rx(0, &data, 1))
            uart_rx_hook(0);
        }                      
    }
}
#define DEMO_USART USART5
#define DEMO_USART_CLK_FREQ CLOCK_GetFreq(kCLOCK_Flexcomm5)
void uart5_config(void)
{
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM5);

	RESET_PeripheralReset(kFC5_RST_SHIFT_RSTn);
	usart_config_t config;
	USART_GetDefaultConfig(&config);
    config.baudRate_Bps = 9600;
    config.enableTx = true;
    config.enableRx = true;

	USART_Init(USART5, &config, CLOCK_GetFreq(kCLOCK_Flexcomm5));
	USART_EnableInterrupts(DEMO_USART, kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable);
	EnableIRQ(FLEXCOMM5_IRQn);
}
void plc_reset_init(void)
{
	GPIO->DIR[0] |= 1U << 30;
		GPIO->B[0][30] = 0;
		IOCON_PinMuxSet(IOCON,0, 30, IOCON_MODE_PULLUP | IOCON_FUNC0 | IOCON_GPIO_MODE | IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);
}
void plc_init(void)
{
	USART5_InitPins();
	uart5_config();
	plc_reset_init();
}
struct UART_Infor uart_infor[0x01];
void init_uart_infor(void)
{
    struct UART_Infor *pchn;

    pchn                   = &uart_infor[0];
    pchn->rx_slot.data_cnt = 0;
    pchn->rx_slot.data_max = 300;
    pchn->rx_slot.tx       = INVALID_PTR;
    pchn->rx_slot.rx       = INVALID_PTR;
    memcpy(&pchn->tx_slot, &pchn->rx_slot, sizeof(pchn->rx_slot));
    
}
int uart_pop_tx(int chn, uint8_t *out, int len)
{
    return (get_chn_bytes(&uart_infor[0].tx_slot, out, len));
}

int uart_push_rx(int chn, uint8_t *in, int len)
{
    return (put_chn_bytes(&uart_infor[0].rx_slot, in, len));
}
int uart_peek(int chn, uint8_t buf[], int len)
{
    return (peek_chn_bytes(&uart_infor[0].rx_slot, buf, len));
}
static void empty_a_chn_slot(struct _CHN_SLOT *pCHN_SLOT, uint8_t len)
{
    unsigned char c;

    while (len > 0)
    {
        len--;
        get_chn_bytes(pCHN_SLOT, &c, 1);
    }
}
void clear_uart(int chn, uint8_t len)
{
    empty_a_chn_slot((struct _CHN_SLOT *)&(uart_infor[0].rx_slot), len);
}
void uart_rx_hook(int chn)
{
    uart_infor[chn].busy_rxing = 10;
    notify(EV_PLC);
}
void uart_tick_hook(void)
{
    for (int i = 0; i < 1; ++i)
    {
        struct UART_Infor *pchn = &uart_infor[i];

        if (pchn->busy_rxing)
        {
            if (--pchn->busy_rxing == 0)
            {
                uint8_t data_cnt = uart_infor[i].rx_slot.data_cnt;
                empty_a_chn_slot((struct _CHN_SLOT *)&(uart_infor[i].rx_slot), data_cnt);
            }
        }
    }
}
void plc_reset_on(void)
{
	GPIO->B[0][30] = 1;
}
void plc_reset_off(void)
{
	GPIO->B[0][30] = 0;
}
void plc_reset_toggle(void)
{
	GPIO->NOT[0] |= (1 << 30);
}
