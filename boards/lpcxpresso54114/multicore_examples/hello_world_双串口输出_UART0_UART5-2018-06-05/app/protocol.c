#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "alloter.h"
#include <utils.h>
#include <board.h>
#include "app_dev_ctrl.h"
#include "smart_plc.h"
#include "protocol.h"


struct RAM  ram;                  //内存最多分配15个字节

uint8 act_delay_flag = 0;
static uint8 compare_soft = 0xff;
/**************************************
    判断是否接受到完整的一帧数据
    output:		_TRUE, _FALSE
 
 举例：7e 7e 00 00 00 01 00 00 00 02 72 04 02 02 02 02 cs
**************************************/
struct SHS_frame* get_smart_frame(uint8 rxframe_raw[], uint8 rxlen)
{
   struct SHS_frame *pframe;
   uint8 i = 0;
   uint8 len;

start_lbl:
   while (i < rxlen)
   {
      if (STC == rxframe_raw[i]) break;
      i++;
   }
//   if(i >= rxlen) return(NULL);
   if (rxlen - i < SHS_FRAME_HEAD) return (NULL); //接收等待length长度
   pframe = (struct SHS_frame *)&rxframe_raw[i];
   len = pframe->length;

   if (i + SHS_FRAME_HEAD + len + 1 > rxlen)
   {
      i++;
      goto start_lbl;
   }

   if (pframe->infor[len] != checksum((uint8 *)pframe, len + SHS_FRAME_HEAD))
   {
      i++;
      goto start_lbl;
   }
//    uart_read(rxframe_raw, i+SHS_FRAME_HEAD+len+1);
//    mymemcpy(&rxframe_raw[0], &rxframe_raw[i], SHS_FRAME_HEAD+len+1);
//    pframe = (struct SHS_frame*)&rxframe_raw[i];
   return (pframe);
}

/*************************************************
                读取设备类型
*************************************************/
void _get_dev_type(uint8 *buff)
{
   uint8 _device_types[8];
   _device_types[0] = 0xff;
   _device_types[1] = 0xff;
   if (device_type == ESACT_1S1A) //0xFF,0xFF,0x18,0x00,0x01,0x00,0x01,0x00
   {
      _device_types[2] = 0x18;
      _device_types[3] = 0x00;
      _device_types[4] = 0x01;
      _device_types[5] = 0x00;
      _device_types[6] = 0x01;
      _device_types[7] = 0x00;
   }
   else if (device_type == ESACT_1S1A_MS) //0xFF,0xFF,0x18,0x00,0x01,0x00,0x02,0x00
   {
      _device_types[2] = 0x18;
      _device_types[3] = 0x00;
      _device_types[4] = 0x01;
      _device_types[5] = 0x00;
      _device_types[6] = 0x02;
      _device_types[7] = 0x00;
   }
   else //0xFF,0xFF,0x13,0x00,0x00,0x00,0x02,0x00
   {
      _device_types[2] = 0x13;
      _device_types[3] = 0x00;
      _device_types[4] = 0x00;
      _device_types[5] = 0x00;
      _device_types[6] = 0x02;
      _device_types[7] = 0x00;
   }
   mymemcpy(buff, (void *)_device_types, sizeof(_device_types));
}

static uint8 get_device_type(uint8 *buff, uint8 max_len)
{
   if (max_len < 0x08) return (0);
   _get_dev_type(&buff[1]);
   return (0x08);
}

/***********************************
 (bit7:1/0 表示通/断；bit6:保留；bit5~bit0:1/0 表示通道操作/不操作；bit0: 表示第 1 个通道
 读取继电器状态：bit5~bit0: 1 表示通；0 表示断）
***********************************/
static uint8 _get_relay_status(uint8 *buff, uint8 max_len)
{
   if (max_len < 1) return (0);
   buff[0] = _OFF;
   if ((device_type == ESACT_1S1A) || (device_type == ESACT_1S1A_MS))
   {
      if (GPIO_ReadOutputData(RELAY_CTRL1_PORT, RELAY_CTRL1_PIN))
      {
         buff[0] |= _ON;
      }
   }
   else if (device_type == ESACT_2S1A)
   {
      if (GPIO_ReadOutputData(RELAY2_CTRL1_PORT, RELAY2_CTRL1_PIN))
      {
         buff[0] |= (_ON << 0x00);
      }
      if (GPIO_ReadOutputData(RELAY2_CTRL2_PORT, RELAY2_CTRL2_PIN))
      {
         buff[0] |= (_ON << 0x01);
      }
   }
   return (1);
}

static uint8 get_relay_status(uint8 *buff, uint8 max_len)
{
   return (_get_relay_status(&buff[1], max_len));
}
void save_relay_status(void)
{
   uint8 relay_s;
   static uint8 relay_last_s = 0xFF; //保存上次继电器状态，防止继电器不动作也在记录状态
   if (LAST_STATUS != eep_function.relay_mode) return;
   EEP_Read(OF_RELAY_STAUS, (uint8 *)&relay_last_s, sizeof(relay_s));
   _get_relay_status(&relay_s, 1);
   if (relay_s != relay_last_s)
   {
      relay_last_s = relay_s;
      EEP_Write(OF_RELAY_STAUS, (uint8 *)&relay_s, sizeof(relay_s));
   }
}
/*********************************************************
   继电器翻转，使用的数据标识为OxC018，数据返回0xC012
********************************************************/
static uint8 set_relay_reverse(uint8 *buff, uint8 w_len)
{
   uint8 i = 0;
   if (w_len != 0x01) return (DATA_ERR);
     if ((device_type == ESACT_1S1A) || (device_type == ESACT_1S1A_MS))
     {
        if (buff[0] & CHN_1)
        {
           if (ram.relay_stat == _ON)
           {
              ram.relay_stat = _OFF;
           }
           else
           {
              ram.relay_stat = _ON;
           }
           ram.relay_act_flag |= CHN_1;
        }
     }
     else if (device_type == ESACT_2S1A)
     {
        if (buff[0] & CHN_1)
        {
           if (ram.relay_stat & CHN_1)
           {
              ram.relay_stat &= (~CHN_1);
           }
           else
           {
              ram.relay_stat |= CHN_1;
           }
           ram.relay_act_flag |= CHN_1;
        }
        if (buff[0] & CHN_2)
        {
           if (ram.relay_stat & CHN_2)
           {
              ram.relay_stat &= (~CHN_2);
           }
           else
           {
              ram.relay_stat |= CHN_2;
           }
           ram.relay_act_flag |= CHN_2;
        }
     }
   //返回报文数据
   buff[i++] = 0x12;  //DID 0xC012
   buff[i++] = 0xC0;
   buff[i++] = 0x01;  //
   buff[i++] = ram.relay_stat; //继电器状态01 闭合，00断开
   if ((ram.relay_act_flag & 0x7F)) //中断打开
   {
      act_delay_flag = 1; //延时标志位
      if (ram.relay_state_bak != ram.relay_stat)
      {
         stage_restart_data_init(REPORT_START);
         stage_data.wait_cnt = 5 + 2 * stage_data.equipment_gid; //间隔2S
         ram.relay_state_bak = ram.relay_stat;
      }
   }
   return (CHG_DID | i);
}


static uint8 set_relay_ctrl(uint8 *buff, uint8 w_len)
{
   if (w_len != 0x01) return (DATA_ERR);
   if (buff[0] & 0x80)
     {
        if ((device_type == ESACT_1S1A) || (device_type == ESACT_1S1A_MS))
        {
           if (buff[0] & CHN_1)
           {
              ram.relay_stat = _ON; //备份继电器状态
           }
        }
        else if (device_type == ESACT_2S1A)
        {
           if (buff[0] & CHN_1)
           {
              ram.relay_stat |= CHN_1;
           }
           if (buff[0] & CHN_2)
           {
              ram.relay_stat |= CHN_2;
           }
        }
     }
     else
     {
        if ((device_type == ESACT_1S1A) || (device_type == ESACT_1S1A_MS))
        {
           if (buff[0] & CHN_1)
           {
              ram.relay_stat = _OFF; //备份继电器状态
           }
        }
        else if (device_type == ESACT_2S1A)
        {
           if (buff[0] & CHN_1)
           {
              ram.relay_stat &= (~CHN_1);
           }
           if (buff[0] & CHN_2)
           {
              ram.relay_stat &= (~CHN_2);
           }
        }
     }
   buff[0] = ram.relay_stat;
   if (ram.relay_state_bak != ram.relay_stat)
   {
      act_delay_flag = 1;
      ram.relay_act_flag = ram.relay_act_flag | (ram.relay_state_bak ^ ram.relay_stat);
      stage_restart_data_init(REPORT_START);
      stage_data.wait_cnt = 5 + 2 * stage_data.equipment_gid; //间隔2S
      ram.relay_state_bak = ram.relay_stat;
   }
   return (NO_ERR);
}
/*******************************************************
                应用层通信协议及版本
********************************************************/

const static uint8 soft_ver[] = "EASTSOFT(v1.0)";
const static uint8 _dev_soft_ver[] = "ESACT-1A(v1.4)-20171020";

static uint8 get_string(uint8 *buff, uint8 max_len, uint8 *str)
{
   if (max_len < strlen(str)) return (0);
   mymemcpy(&buff[0], (uint8 *)str, strlen(str));
   return (strlen(str));
}
static uint8 get_dev_soft_ver(uint8 *buff, uint8 max_len)
{
   return (get_string(&buff[1], max_len, _dev_soft_ver));
}
uint8 compare_soft_ver(uint8 *buff, uint8 len)
{
   return (memcmp_my((uint8 *)_dev_soft_ver, buff, strlen((char const *)_dev_soft_ver)));
}
uint8 _get_dev_soft_ver(uint8 *buff)
{
   mymemcpy(buff, _dev_soft_ver, strlen(_dev_soft_ver));
   return (strlen(_dev_soft_ver));
}

static uint8 get_soft_ver(uint8 *buff, uint8 max_len)
{
   return (get_string(&buff[1], max_len, soft_ver));
}

static uint8 set_password(uint8 *buff, uint8 w_len)
{
   uint8 i = 0, ret = 0;

   if (w_len != 0x02)
   {
      return (DATA_ERR);
   }
   if ((VALID_DATA == eep_param.pwd_magic)
       && (0x00 == memcmp_my(eep_param.password, buff, sizeof(eep_param.password))))
   {
      ret = 1;
   }
   else if (INVALID_DATA == eep_param.pwd_magic)
   {
      ret = 1;
      mymemcpy(eep_param.password, buff, sizeof(eep_param.password));
      eep_param.pwd_magic = VALID_DATA;
      EEP_Write(OF_PLC_PARAM, (uint8 *)&eep_param, sizeof(eep_param));
   }
   buff[i++] = 0x30; //返回报文
   buff[i++] = 0xC0;
   buff[i++] = 0x01;
   buff[i++] = ret; //01成功
   return (CHG_DID | i);
}

static uint8 set_ctrl_mode(uint8 *buff, uint8 w_len)
{
   uint8 i = 0, ret = 0;
   if ((w_len != 0x01) || (buff[0] > 0x02))
   {
      return (DATA_ERR);
   }
   eep_function.relay_mode = buff[0];
   EEP_Write(OF_FUNCTION_ADDR, (uint8 *)&eep_function, sizeof(eep_function));
   return (NO_ERR);
}
static uint8 get_dkey(uint8 *buff, uint8 max_len)
{
   struct ENCODE_PARAM encode_param;
   if (max_len < DKEY_LEN) return (0);

   EEP_Read(ENCODE_MAGIC_ADDR, (uint8 *)&encode_param, sizeof(struct ENCODE_PARAM));
   if (ENCODE_MAGIC == encode_param.sole_magic)
   {
      mymemcpy(&buff[1], encode_param.dev.dkey, DKEY_LEN);
      return (DKEY_LEN);
   }
   buff[1] = DATA_ERR;
   buff[2] = 0x00;
   return (0x82);
}

static uint8 get_device_attribute(uint8 *buff, uint8 max_len)
{
   uint8 len;
   EEP_Read(ENCODE_PARAM_ADDR + ENCODE_LEN, (uint8 *)&len, 1);
   if ((len < 1) || (len > MAX_ATTRIBUTE_LEN))
   {
      buff[1] = DATA_ERR;
      buff[2] = 0x00;
      return (0x82);
   }
   EEP_Read(ENCODE_PARAM_ADDR + offset_of(struct DEV_INFOR, infor), (uint8 *)&buff[1], len);
   return (len);
}

static uint8 get_sn(uint8 *buff, uint8 max_len)
{
   struct ENCODE_PARAM encode_param;
   if (max_len < SN_LEN) return (0);
   EEP_Read(ENCODE_MAGIC_ADDR, (uint8 *)&encode_param, sizeof(struct ENCODE_PARAM));
   if (ENCODE_MAGIC == encode_param.sole_magic)
   {
      mymemcpy(&buff[1], encode_param.dev.sn, SN_LEN);
      return (SN_LEN);
   }
   buff[1] = DATA_ERR;
   buff[2] = 0x00;
   return (0x82);
}

static uint8 get_report_enable_infor(uint8 *buff, uint8 max_len)
{
   if (max_len < 0x01) return (0);
   //EEP_Read(OF_PLC_PARAM, (uint8 *)&eep_param, sizeof(eep_param));
   buff[1] = 0x00;
   buff[2] = eep_function.report_enable;
   return (0x02);
}

static uint8 set_report_enable_infor(uint8 *buff, uint8 w_len)
{
   if (w_len < 0x01) return (DATA_ERR);
   //buff[0];保留不处理
   eep_function.report_enable = buff[1];
   //gd_data.eep_opt = EEP_OPT;
   //EEP_Write(OF_PLC_PARAM, (uint8 *)&eep_param, sizeof(eep_param));
   EEP_Write(OF_FUNCTION_ADDR, (uint8 *)&eep_function, sizeof(eep_function));
   return (NO_ERR);
}

static uint8 get_delay_time(uint8 *buff, uint8 max_len)
{
   if (max_len < 0x01) return (0);
   if ((device_type == ESACT_1S1A) || (device_type == ESACT_1S1A_MS))
   {
      buff[1] = 0x01;
      buff[2] = eep_function.close_delay_time1; // close duan
      buff[3] = eep_function.open_delay_time1; // open tong
      return (0x03);
   }
   else
   {
      buff[1] = 0x01;
      buff[2] = eep_function.close_delay_time2_1; // close
      buff[3] = eep_function.open_delay_time2_1; // open
      buff[4] = 0x02;
      buff[5] = eep_function.close_delay_time2_2; // close duan
      buff[6] = eep_function.open_delay_time2_2; // open tong
      return (0x06);
   }

}

static uint8 set_delay_time(uint8 *buff, uint8 w_len)
{
   if (w_len < 0x01) return (DATA_ERR);
   if ((device_type == ESACT_1S1A) || (device_type == ESACT_1S1A_MS))
   {
      if (0x01 != buff[0]) return (DATA_ERR);
      eep_function.close_delay_time1 = buff[1]; // close
      eep_function.open_delay_time1 = buff[2]; // open
   }
   else
   {
      if (0x01 == buff[0])
      {
         eep_function.close_delay_time2_1 = buff[1]; // close
         eep_function.open_delay_time2_1 = buff[2]; // open
      }
      if (0x02 == buff[0])
      {
         eep_function.close_delay_time2_2 = buff[1]; // close
         eep_function.open_delay_time2_2 = buff[2]; // open
      }
   }
   EEP_Write(OF_FUNCTION_ADDR, (uint8 *)&eep_function, sizeof(eep_function));
   return (NO_ERR);
}
//---------------------------------------------------------------------------------
#if DEV_SHOW
static uint8 set_silent_time(uint8 *buff, uint8 w_len)
{
   if (w_len != 0x02) return (DATA_ERR); //len is zero

   dev_search_param.silent_time = get_le_val(&buff[0], TIME_LEN);
   return (NO_ERR);
}


static uint8 get_psw(uint8 *buff, uint8 max_len)
{
   if (max_len < 0x01) return (0);

   memcpy(&buff[1], eep_param.password, PWD_LEN);
   return (PWD_LEN);
}

static uint8 set_dev_show(uint8 *buff, uint8 w_len)
{
   if (w_len) return (DATA_ERR); //len is zero

   dev_search_param.dev_show_flag = TRUE;
   return (NO_ERR);
}
#endif

//---------------------------------------------------------------------------------
#define scat_flag(f) {(uint8)(f), (uint8)(f>>8)}
const struct func_ops func_items[] =
{
   //di,r_len,w_len, read, write
   { scat_flag(0xc012), get_relay_status,          	set_relay_ctrl }, //控制通断
   { scat_flag(0x0001), get_device_type,           	NULL },
   { scat_flag(0x0003), get_dev_soft_ver,						NULL },
   { scat_flag(0x0002), get_soft_ver,    						NULL },
   { scat_flag(0x0005), get_dkey,        						NULL },
   { scat_flag(0x0006), get_device_attribute,   		NULL },
   { scat_flag(0x0007), get_sn,          						NULL },
   { scat_flag(0xd005), get_report_enable_infor,		set_report_enable_infor },
   { scat_flag(0xC020), get_delay_time,							set_delay_time },
   { scat_flag(0xC030), NULL,           						set_password },
   { scat_flag(0xC060), NULL,           						set_ctrl_mode },
   { scat_flag(0xC018), NULL,           						set_relay_reverse },

#if DEV_SHOW
   { scat_flag(0x0009), NULL,           			 		set_dev_show },
   { scat_flag(0x000A), get_psw,         					NULL },
   { scat_flag(0x000B), NULL,         						set_silent_time },
#endif 
};
#define	METER_ITEM_MAX		ARRAY_SIZE(func_items)
/**************************************
根据数据标志执行相应的功能
**************************************/
#define DATA_LEN(pframe)    (pframe->ctrl&0x7F)
uint8 set_parameter(uint8 data[], uint8 len)
{
   uint8 i, ret;
   struct FBD_Frame *pframe;
   uint8 * pw,*pr;

   pw = data;
   pr = &g_frame_buffer[MAX_BUFFER_SZ - 1 - 1] - len;
   memmove_my(pr, pw, len);

   while ((len >= FBD_FRAME_HEAD) && (pw < pr))
   {
      pframe = (struct FBD_Frame *)pr;

      if (len <  FBD_FRAME_HEAD + DATA_LEN(pframe))
      {
         mymemcpy(pw, pframe, 2);
         pw += 2;
         *(pw++) = 0x82;   //返回报文 ，长度出错
         *(pw++) = LEN_ERR; //01
         *(pw++) = 0x00;
         break;
      }
      mymemcpy(pw, pr, FBD_FRAME_HEAD + DATA_LEN(pframe));
      pframe = (struct FBD_Frame *)pw;
      pw += 2;
      len -= FBD_FRAME_HEAD + DATA_LEN(pframe);
      pr += FBD_FRAME_HEAD + DATA_LEN(pframe);

      for (i = 0; i < METER_ITEM_MAX; i++)
      {
         if (memcmp_my(pframe->did, func_items[i].di, 2) == 0) break;
      }
      if ((i >= METER_ITEM_MAX) || (NULL == func_items[i].write))
      {
         *(pw++) = 0x82;    //返回报文 ，数据标识不存在
         *(pw++) = DID_ERR; //04
         *(pw++) = 0x00;
      }
      else
      { //调用func_items函数，函数指针(各种控制命令等)
         ret = func_items[i].write(pframe->data, DATA_LEN(pframe));
         if (CHG_DID & ret)
         { //修改数据did
            ret &= 0x7f;
            mymemcpy(pframe, pframe->data, ret);
            ret -= 2;
            pw += ret;
         }
         else if (NO_ERR != ret)
         {
            *(pw++) = 0x82;
            *(pw++) = ret;
            *(pw++) = 0x00;
         }
         else
         {
            ret = DATA_LEN(pframe) + 1; //设置数据长度
            pw += ret;
         }
      }
   }
   return (pw - data);
}

#define GROUP_LEN       0x3F
uint8 is_gid_equal(uint8 data[])
{
   uint16 s_gid, t_gid;
   uint8 k, connt_size = 0;
   uint8 i, len_t, type_t;
   uint16 j = 0;
   s_gid = eep_param.sid[1]; //网关分配的SID,2B
   s_gid <<= 8;
   s_gid += eep_param.sid[0];
   len_t = data[0] & GROUP_LEN; //组地址字节数
   type_t = data[0] >> 6; //组地址类型
   if (0x00 == type_t) //组地址用一个位表示
   {
      s_gid--;
      k = s_gid + 1;
      if (k % 8)
      {
         k = k % 8;
      }
      else
      {
         k = 8;
      }
      if(len_t < (s_gid>>3)+1)
      {
         goto deal_find_none;
      }
      if (data[(s_gid >> 3) + 1] & (0x01 << (s_gid & 0x07)))
      {

         for (j = 0;j <(s_gid>>3); j++)
         {
            connt_size += get_1byte_bit1_number(data[j + 1], 8);
         }
         connt_size += get_1byte_bit1_number(data[(s_gid >>3) + 1], k);
         if (0 == stage_data.find_myself_flag)
         {
            stage_data.equipment_gid += connt_size;
         }
         stage_data.find_myself_flag = 1;
         return (1);
      }
   }
   else //组地址用字节表示
   {
      for (i = 1; i <= len_t; i++)
      {
         t_gid = data[i];
         if (0x02 == type_t)
         {
            if (len_t & 1) return (0);
            t_gid += data[i + 1] << 8;
            i++;
         }
         if (t_gid == s_gid)
         {
            if (0x02 == type_t)
            {
               connt_size += i / 2;
            }
            else
            {
               connt_size += i;
            }
            if (0 == stage_data.find_myself_flag)
            {
               stage_data.equipment_gid += connt_size;
            }
            stage_data.find_myself_flag = 1;
            return (1);
         }
         if (0x00 == t_gid) //全广播
         {
            stage_data.equipment_gid += s_gid;
            stage_data.find_myself_flag = 1;
            return (1);
         }
      }
   }
   deal_find_none:
   if (0 == stage_data.find_myself_flag)
   {
      if (0x00 == type_t)
      {
         for (j = 0; j < len_t; j++)
         {
            stage_data.equipment_gid += get_1byte_bit1_number(data[j + 1], 8);
         }
      }
      else if (0x01 == type_t)
      {
         stage_data.equipment_gid += len_t;
      }
      else if (0x02 == type_t)
      {
         stage_data.equipment_gid += len_t / 2;
      }
   }
   return (0);
}

uint8 set_group_parameter(uint8 data[], uint8 len)
{
   uint8 i, j, gid_len, fbd_len;
   struct FBD_Frame *pframe;

   j = 0;
   gid_len = (data[j] & GROUP_LEN) + 1;
   while (len >= (FBD_FRAME_HEAD + gid_len))
   {
      pframe = (struct FBD_Frame *)&data[j + gid_len];
      fbd_len = DATA_LEN(pframe) + FBD_FRAME_HEAD + gid_len;
      if (len < fbd_len) break;

      if (is_gid_equal(&data[j]))
      {
         for (i = 0; i < METER_ITEM_MAX; i++)
         {
            if (memcmp_my(pframe->did, func_items[i].di, 2) == 0) break;
         }
         if ((i < METER_ITEM_MAX) && (NULL != func_items[i].write))
         { //调用func_items函数，函数指针(各种控制命令等)
            func_items[i].write(pframe->data, DATA_LEN(pframe));
         }
      }

      j += fbd_len;
      len -= fbd_len;

      gid_len = (data[j] & GROUP_LEN) + 1;
   }
   return (0);
}

/**************************************
根据数据标志执行相应的功能
**************************************/
uint8 read_parameter(uint8 data[], uint8 len)
{
   uint8 i, ret;
   struct FBD_Frame *pframe;
   uint8 * pw,*pr;

   pw = data;
   pr = &g_frame_buffer[MAX_BUFFER_SZ - 1 - 1] - len; //最大可读数据长度,当数据返回长度刚刚好，cs无法发送出去问题？！
   memmove_my(pr, &data[0], len);

   while (len >= FBD_FRAME_HEAD)
   {
      pframe = (struct FBD_Frame *)pr;

      if (len <  FBD_FRAME_HEAD + DATA_LEN(pframe))
      { //ctrl 长度出错
         mymemcpy(pw, pframe, 2);
         pw += 2;
         *(pw++) = 0x82;
         *(pw++) = LEN_ERR;
         *(pw++) = 0x00;
         break;
      }
      mymemcpy(pw, pr, FBD_FRAME_HEAD + DATA_LEN(pframe));
      pframe = (struct FBD_Frame *)pw;
      pw += 2;
      len -= FBD_FRAME_HEAD + DATA_LEN(pframe);
      pr += FBD_FRAME_HEAD + DATA_LEN(pframe);

      for (i = 0; i < METER_ITEM_MAX; i++)
      {
         if (memcmp_my(pframe->did, func_items[i].di, 2) == 0) break;
      }
      if ((i >= METER_ITEM_MAX) || (func_items[i].read == NULL))
      {
         *(pw++) = 0x82;
         *(pw++) = DID_ERR;
         *(pw++) = 0x00;
      }
      else
      {
         ret = func_items[i].read(pw, (uint8)(pr - (pw + 1)));
         if (0x00 == ret) //数据返回0，可能导致读取数据内容没有数据标识，数据长度为1的报文!?
         {
            pw -= 2;
            continue;
         }
         *(pw++) = ret;
         pw += (ret & 0x7F); //最高位为错误标志位，返回的数据如果是带有错误标志位？！
      }
   }
   return (pw - data);
}