#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <utils.h>
#include <stddef.h>
#include "protocol_smart.h"
#include "fsl_debug_console.h"
#include "app_dev_ctrl.h"
#include "app_si70xx.h"
#include "auto_report.h"
#include "auto_report_app.h"
//�ж��Ƿ���յ�������һ֡����
extern struct dev dev;
extern uint8_t plc_state_flag;
extern uint8_t plc_communication;
extern uint8_t get_1byte_bit1_number(uint8_t data, uint8_t pos);
extern uint8_t save_dev_flag;//�洢Flash��־
extern uint8_t g_frame_buffer[0x400];
extern struct SI7020_DATA si7020_data;//��ȡ�Ĵ�����ֵ
struct SmartFrame *get_smart_frame(const uint8_t *in, int len)
{
    int i = 0;

 start_lbl:
    while (i < len)//�ҳ�7E��λ��
    {
        if (STC == in[i])
            break;
        i++;
    }
    if (len - i < SMART_FRAME_HEAD)//�������ݰ�����
        return NULL;
    struct SmartFrame *pframe = (struct SmartFrame *)&in[i];
    int dlen = pframe->len;

    if (i + SMART_FRAME_HEAD + dlen + 1 > len)//���ȴ���
    {
        i++;
        goto start_lbl;
    }

    if (pframe->data[dlen] != checksum(pframe, dlen + SMART_FRAME_HEAD))//У��ʹ���
    {
        i++;
        goto start_lbl;
    }
    pframe = (struct SmartFrame *)&in[i];
    return pframe;
}
//---------------------------------------------------------------------------------------
int code_frame(const uint8_t *src, const uint8_t *dest, int seq, int cmd, //��ظ�����
    const uint8_t *data, int len, void *out, int maxlen)
{
    const uint8_t addr[AID_LEN] = {0x00, 0x00, 0x00, 0x00};

    static uint8_t _seq = 0;
    struct SmartFrame *pframe = (struct SmartFrame *)out;

    memmove(&pframe->data[1], data, len);
    pframe->stc = STC;
    if (!src) src = addr;
    if (!dest) dest = addr;
    memcpy(pframe->said, src, AID_LEN);
    memcpy(pframe->taid, dest, AID_LEN);
    pframe->seq = seq < 0 ? (_seq++ & 0x7F) : seq;

    PRINTF( "The seq is %d!\n",_seq&0x7f);

    pframe->data[0] = cmd;
//    memcpy(&pframe->data[1], data, len);
//    pframe->len = len+1;
//    pframe->data[pframe->len] = checksum(pframe, SMART_FRAME_HEAD + pframe->len);
//    pframe->data[0] = cmd;
    
    pframe->len                = len + 1;
    pframe->data[pframe->len] = checksum(pframe, SMART_FRAME_HEAD + pframe->len);
    return SMART_FRAME_HEAD + pframe->len + 1;
}

int code_local_frame(const uint8_t *in, int len, void *out, int maxlen)//���ر���
{
    return code_frame(NULL, NULL, 0, in[0], &in[1], len-1, out, maxlen);
}

int code_ret_frame(struct SmartFrame *pframe, int len)//��Զ�̱��ġ�����AID��SID
{
    uint8_t tmp[AID_LEN];

    memcpy(tmp,          pframe->taid, sizeof(pframe->taid)); 
    memcpy(pframe->taid, pframe->said, sizeof(pframe->taid)); 
    memcpy(pframe->said, tmp,          sizeof(pframe->said)); 
    pframe->seq |= 0x80;
    pframe->len  = len;
    pframe->data[len] = checksum(pframe, pframe->len + SMART_FRAME_HEAD);
    return pframe->len + SMART_FRAME_HEAD + 1;
}

int code_body(uint16_t did, int err, const void *data, int len, void *out, int maxlen)//FBD
{
    struct Body *body = (struct Body *)out;

    put_le_val(did, body->did, sizeof(body->did));//��DIDת����body->did��С��
    body->ctrl = get_bits(len, 0, 6);//ȡ�����ȡ�
    if (err) body->ctrl |= 0x80;
    memcpy(body->data, data, len);

    return sizeof(struct Body) + len;
}
//void send_local_frame(uint8_t buffer[], uint8_t len)
//{
//    struct SmartFrame *pframe = (struct SmartFrame *)buffer;

//    memmove(&buffer[offsetof(struct SmartFrame, data)], buffer, len);
//    pframe->stc = STC;
//    memset(pframe->said, 0x00, ID_LEN);
//    memset(pframe->taid, 0x00, ID_LEN);
//    pframe->seq        = 0;
//    pframe->len        = len;
//    pframe->data[len] = checksum((uint8_t *)pframe, SMART_FRAME_HEAD + len);
//    uart_write(0, buffer, SMART_FRAME_HEAD + len + 1);
//}
//uint8_t local_ack_opt(struct SmartFrame *pframe)
//{
//    if (pframe)
//    {
//        if (!is_all_xx(pframe->said, 0x00, ID_LEN) || !is_all_xx(pframe->taid, 0x00, ID_LEN))
//            return (0);

//        uint8_t cmd = pframe->data[0];
//        if (cmd == CMD_ACK || cmd == CMD_NAK)
//        {
//            next_state();
//            return (1);
//        }
//    }
//    if (plc_state.wait_t > MAX_OVERTIME)
//        try_state_again();
//    return (0);
//}
//��ȡ�豸����
int get_device_type(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	return dev_type_get(out, maxlen);
}
//static int do_get_dev_type(const uint8_t *in, size_t len, uint8_t *out, size_t maxlen)
//{
//    return dev_type_get(out, maxlen);
//}
//������汾��
char *dev_soft_ver = "P-TK-03-2A-TH-63(v1.0)-20171128";

int get_dev_soft_ver(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
    uint8_t info_len;

    info_len = (char)strlen(dev_soft_ver);
    if (maxlen < info_len)
        return (-BUFFER_ERR);
    memcpy(out, dev_soft_ver, info_len);
    return (info_len);
}
//static int do_get_dev_ver(const uint8_t *in, size_t len, uint8_t *out, size_t maxlen)
//{
//    return dev_ver_get(out, maxlen);
//}
//Ӧ�ò�ͨ��Э��汾��
//static int get_app_comm_ver(const uint8_t *in, uint8_t len, uint8_t *out, size_t maxlen)
//{
//	uint8_t ver_len;
//	const char ver[] = "EASTSOFT(v1.0)";
//	ver_len = (char)strlen(ver);
//	if (maxlen < ver_len)
//		return (-BUFFER_ERR);
//	memcpy(out, ver, ver_len);
//	return (ver_len);
//}
static int get_app_comm_ver(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
    uint8_t ver_len;
    const char ver[] = "EASTSOFT(v1.0)";

    ver_len = (char)strlen(ver);
    if (maxlen < ver_len)
        return (-BUFFER_ERR);

    memcpy(out, ver, ver_len);
    return (ver_len);
}
//��D-KEY
static int get_dev_key(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
    memcpy(out, dev.encode.dkey, DKEY_LEN);
    return (DKEY_LEN);
}

static int get_dev_sn(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
    memcpy(out, dev.encode.sn, SN_LEN);
    return (SN_LEN);
}

static int chk_pwd(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	uint8_t ret = 0;
	if (len != PW_LEN)
		return (-DATA_ERR);
	if (!memcmp(dev.encode.pwd, in, PW_LEN))//int memcmp(const void *buf1, const void *buf2, unsigned int count);
												//��ͬʱ����0��bru1<buf2ʱ������<0��bru1>buf2ʱ������>0.
		ret = 1;//PWD��ͬʱ�� ret=1;
	out[0] = ret;
	return (0x01);
}
//��ȡ������ֵ

static int  get_sensor_value(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	uint8_t sensor_type = in[0];
	if (len < 0x01)
		return (-DATA_ERR);
	if (TEMP_SENSOR == sensor_type)
	{
		out[0] = TEMP_SENSOR;
		hex2bcd(abs((int)si7020_data.temperature), &out[1], 2);
//		sensor.temp < 0 ?  set_bit(out[2], 7) : clr_bit(out[2], 7);
	}
	else if (HUMI_SENSOR == sensor_type)
	{
		out[0] = HUMI_SENSOR;
        hex2bcd((int)si7020_data.humidity, &out[1], 2);
	}
	else
	{
		return (-DATA_ERR);
	}
	return (0x03);
	
}

//��ȡ�ϱ�����
static int get_report_step(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if (len < 0x01)
		return (-DATA_ERR);
	uint8_t sensor_type = in[0];
	if (TEMP_SENSOR == sensor_type)
	{
		out[0] = TEMP_SENSOR;
		hex2bcd(dev.temp_report_step, &out[1], STEP_LEN);
	}
	else if (HUMI_SENSOR == sensor_type)
	{
		out[0] = HUMI_SENSOR;
		hex2bcd(dev.humi_report_step, &out[1], STEP_LEN);
	}
	else
	{
		return (-DATA_ERR);
	}
	return (0x03);
}
//�����ϱ�����
static int set_report_step(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if (len != 0x03)
		return (-DATA_ERR);
    if (!is_all_bcd(&in[1], STEP_LEN))
		return (-DATA_ERR);
	uint8_t sensor_type = in[0];
	if (TEMP_SENSOR == sensor_type)
	{   /*1 <= temp step <= 10*/
        if ((in[2] == 0x00 && in[1] < 0x10 && in[1] != 0x00)//С��1��
            || (in[2] > 0x01) || (in[2] == 0x01 && in[1] != 0x00))//����10��
            return (-DATA_ERR);
		dev.temp_report_step = xbcd2hex(&in[1], STEP_LEN);
	}
	else if (HUMI_SENSOR == sensor_type)
	{   /*1 <= humidity step <= 30*/
        if ((in[2] == 0x00 && in[1] < 0x10 && in[1] != 0x00)//С��1%
            || (in[2] > 0x03) || (in[2] == 0x03 && in[1] != 0x00))//����30%��
            return (-DATA_ERR);
		dev.humi_report_step = xbcd2hex(&in[1], STEP_LEN);
	}
	else
	{
		return (-DATA_ERR);
	}
	write_to_flash();
//    write_param_to_flash();
//    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
//    auto_report_parameter_refresh(dev.gid, dev.encode.id, dev.sid, REPORT_GW, STEP_CHANGE);
	return (NO_ERR);
}
//��ȡ�ϱ�Ƶ��
static int get_report_freq(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if (len < 0x01)
		return (-DATA_ERR);
	uint8_t sensor_type = in[0];
	if (TEMP_SENSOR == sensor_type)
	{
		out[0] = TEMP_SENSOR;
		put_le_val(dev.temp_report_freq, &out[1], FREQ_LEN);
//    hex2bcd(dev.temp_report_freq, &out[1], FREQ_LEN);
	}
	else if (HUMI_SENSOR == sensor_type)
	{
		out[0] = HUMI_SENSOR;
		put_le_val(dev.humi_report_freq, &out[1], FREQ_LEN);
//    hex2bcd(dev.humi_report_freq, &out[1], FREQ_LEN);
	}
	else
	{
		return (-DATA_ERR);
	}
	return (0x03);
}
//�����ϱ�Ƶ��
static int set_report_freq(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if (len != 0x03)
		return (-DATA_ERR);
	uint8_t sensor_type = in[0];
	if (TEMP_SENSOR == sensor_type)
	{
		dev.temp_report_freq = get_le_val(&in[1], FREQ_LEN);
	}
	else if (HUMI_SENSOR == sensor_type)
	{
		dev.humi_report_freq = get_le_val(&in[1], FREQ_LEN); 
	}
	else
	{
		return (-DATA_ERR);
	}
    reload_freq_infor();
//    write_param_to_flash();
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
//    auto_report_parameter_refresh(dev.gid, dev.encode.id, dev.sid, REPORT_GW, REPORT_CHANGE);
	return (NO_ERR);
}
//��ȡ�ϵ���ʱʱ��
static int get_power_on_delay_time(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	put_le_val(dev.power_on_delay, out, 0x02);
	return (0x02);
}
//�����ϵ���ʱʱ��
static int set_power_on_delay_time(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if (len != 0x02)
		return (-DATA_ERR);
	uint16_t delay = get_le_val(in, 0x02);
	dev.power_on_delay = delay < 60 ? 60 : delay;
//	write_param_to_flash();
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	return (NO_ERR);
}
//��ȡ��ʪ�Ȳ���ֵ
extern uint16_t photosensor_value;
static int get_dbg_info(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
    if (len < 0x01)
		return (-DATA_ERR);
    switch (in[0])
    {
    case TEMP_SENSOR:                     //�¶Ȳ���
    {
        if(dev.temp_reduction<0)        //����Ϊ����
        {
            hex2bcd(0-dev.temp_reduction,&out[1],2);
            out[2]|=0x80;
        }
        else      hex2bcd(dev.temp_reduction,&out[1],2);//����Ϊ��
        return (0x03);
    }
        
    case HUMI_SENSOR:                     //ʪ�Ȳ���
    {
        if(dev.humi_reduction<0)        //����Ϊ����
        {
            hex2bcd(0-dev.humi_reduction,&out[1],2);
            out[2]|=0x80;
        }
        else  hex2bcd(dev.humi_reduction,&out[1],2);//����Ϊ��
        
        return (0x03);
    }
    default:
        break;
    }
    return (-DATA_ERR);
}
//��ʪ�Ȳ���
static int set_dbg_info(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
    uint8_t in_temporary[3];//��ʱ����洢��Ҫ���õĲ���
	if (len < 0x03)
		return (-DATA_ERR);
    memcpy(in_temporary,in,2+1);
    in_temporary[2]&=0x7f;
//    if(xbcd2hex(&in_temporary[1],TEMP_LEN)>100)
//        return (-DATA_ERR);
    switch (in_temporary[0])
    {
        case TEMP_SENSOR:
        {
            if((in[2]&0x80)==0x80)//��ʾ��ʱΪ����
            {
                in_temporary[2]&=0x7f;
                dev.temp_reduction=0-xbcd2hex(&in_temporary[1],2);
            }
            else                    //����
            {
                dev.temp_reduction=xbcd2hex(&in[1],2);
            }
        }break;
        case HUMI_SENSOR:
        {
            if((in[2]&0x80)==0x80)//��ʾ��ʱΪ����
            {
                in_temporary[2]&=0x7f;
                dev.humi_reduction=0-xbcd2hex(&in_temporary[1],2);
            }
            else                    //����
            {
                dev.humi_reduction=xbcd2hex(&in[1],2);
            }
        }break;
        default:
            break;
    }
//    write_param_to_flash();
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	return (NO_ERR);
}

//static int get_dev_inf(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
//{
//	//uint8_t xlen;
//	return (-DATA_ERR);
//}
static int get_report_switch(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if(len<0x01)
    return (-DATA_ERR);
    if(TEMP_SENSOR==in[0])
	{
		put_le_val(TEMP_SENSOR,&out[0],0x01);
		put_le_val(dev.active_report,&out[1],0x01);
	}
	else if(HUMI_SENSOR==in[0])
	{
		put_le_val(HUMI_SENSOR,&out[0],0x01);
		put_le_val(dev.active_report,&out[1],0x01);
	}
	else 
		return (-DATA_ERR);
	return 0x02;
}
//�����ϱ�  0x00:���ϱ���0x01���ϱ����أ�0x02���ϱ��豸��0x03���ϱ����غ��豸
 static int set_report_switch(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)//���д��������͡�
{
	if(len != 0x02)//////////////////////////
	return (-DATA_ERR);
	if(in[1]>0x03)//�޶�ֻ��д0x00,0X01,0x02,0x03;
		return (-DATA_ERR);
	if(TEMP_SENSOR==in[0])
	{
		put_le_val(TEMP_SENSOR,&out[0],0x01);
		dev.active_report=in[1];
	}
	else if(HUMI_SENSOR==in[0])
	{
		put_le_val(HUMI_SENSOR,&out[0],0x01);
		dev.active_report=in[1];
	}
	else 
		return (-DATA_ERR);
//	write_param_to_flash();
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x02;
}
//�������¶�  E004
static int get_temp_set(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	hex2bcd(dev.temp_param.set_temp, out,0x02);
	return 0x02;
	
}
//д�������¶� E004
static int set_temp_set(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	uint16_t temp;
//	uint8_t xlen;
	if (len != 0x02)
		return (-DATA_ERR);
	temp=xbcd2hex(in, 0x02);
	if(temp%5!=0)
		return (-DATA_ERR);//д������ݲ���5�����������������ݴ����־
	if(temp<dev.temp_param.mix_set_temp||temp>dev.temp_param.max_set_temp)
		return (-DATA_ERR);//���������¶�����Χ��
	dev.temp_param.set_temp=temp;
//	write_param_to_flash();//д��FLASH 
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x02;
}
//�������¶�  E002
static int get_temp_set_E002(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	//hex2bcd(dev.temp_param.set_temp, out,0x02);
    hex2bcd(dev.temp_param.set_temp/10, out,0x01);
	return 0x01;
	
}
//д�������¶� E002
static int set_temp_set_E002(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	uint16_t temp;
	if (len !=0x01)//////////////////////////////////////
		return (-DATA_ERR);
    temp=xbcd2hex(in,0x01);
    if((temp*10)<dev.temp_param.mix_set_temp||(temp*10)>dev.temp_param.max_set_temp)
        return (-DATA_ERR);//д������ݲ���5�����������������ݴ����־
    dev.temp_param.set_temp=temp*10;
//    write_param_to_flash();//д��FLASH 
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x01;
}
static int get_temp_range(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	hex2bcd(dev.temp_param.mix_cool_temp/10,&out[0],0X01);//ֻȡ����λ
	hex2bcd(dev.temp_param.max_cool_temp/10,&out[1],0X01);
	hex2bcd(dev.temp_param.mix_heat_temp/10,&out[2],0X01);
	hex2bcd(dev.temp_param.max_heat_temp/10,&out[3],0X01);
	return 0x04;
}
static int set_temp_range(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	uint16_t temporary[4];
	uint8_t i;
	if (len != 0x04)//���Ȳ�Ϊ4ʱ���������ݳ��ȴ���
		return (-DATA_ERR);
	for(i=0;i<4;i++)
	{
		temporary[i]=xbcd2hex(&in[i],0x01);
	}
	
	if((temporary[1]-temporary[0]<5)||(temporary[3]-temporary[2]<5))//����¶�ֵ������¶�ֵ����С��5�ȡ�
		return (-DATA_ERR);
	if(temporary[1]>(dev.temp_param.max_set_temp/10)||temporary[3]>(dev.temp_param.max_set_temp/10))//����¶Ȳ��ܳ���30��
		return (-DATA_ERR);
	if(temporary[0]<dev.temp_param.mix_set_temp/10||temporary[2]<dev.temp_param.mix_set_temp/10)//����¶Ȳ��ܵ͹�5��
		return (-DATA_ERR);
	dev.temp_param.mix_cool_temp=10*temporary[0];
	dev.temp_param.max_cool_temp=10*temporary[1];
	dev.temp_param.mix_heat_temp=10*temporary[2];
	dev.temp_param.max_heat_temp=10*temporary[3];
//	write_param_to_flash();//д��FLASH 
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x04;
}
static int get_low_temp_protect(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	put_le_val(dev.temp_param.low_temp_protect,out,0x01);
	return 0x01;//�������ݳ���
	
}
static int set_low_temp_protect(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if(len!=0x01)//���ݳ��Ȳ�Ϊ1ʱ���������ݳ��ȴ���
		return (-DATA_ERR);//�������ݳ��ȴ����־
//	if((in[0]!=0x01)&&(in[0]!=0x00))//�޶�ֻ��д0x00,��0X01;�˴�Ӧ��Ϊ&&�����Ϊ||����Ϊ�棬ִ��return��
	if(in[0]>0x01)//�޶�ֻ��д0x00,��0X01;
		return (-DATA_ERR);
	dev.temp_param.low_temp_protect=in[0];
//	write_param_to_flash();//
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x01;//
}
//������
static int get_win_speed_ctrl(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if(dev.temp_param.mode==COOL||dev.temp_param.mode==HEAT)//����ģʽ�£�
        put_le_val(dev.temp_param.win_speed,out,0x01);
    else if(dev.temp_param.mode==VENTILATE)//ͨ��ģʽ
        put_le_val(dev.temp_param.ventilate_speed,out,0x01);
	return 0x01;//���س���Ϊ1��
}
static int set_win_speed_ctrl(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if(len!=0x01)
		return (-DATA_ERR);//���ݴ����־
	if(in[0]>3)//�޶�ֻ��д0x00,0X01,0x02,0x03;
		return (-DATA_ERR);//���ݴ����־
    
    if(dev.temp_param.mode==COOL||dev.temp_param.mode==HEAT)
        dev.temp_param.win_speed=in[0];
    else
    {
       if(in[0]==AUTO)//�Զ���ģʽ�£�û���Զ��磬������������Զ��磬���ش���
           return (-DATA_ERR);//���ݴ����־
       else
           dev.temp_param.ventilate_speed=in[0]; 
    }
//	write_param_to_flash();//д��Flash
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x01;//����Ϊ1
}
static int get_mode_ctrl(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	put_le_val(dev.temp_param.mode,out,0x01);//��32λ����ת��Ϊ����
	return 0x01;//���س���Ϊ1
}
static int set_mode_ctrl(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if(len!=0x01)//�����Ȳ�Ϊ1�ǣ��ط������� ����
		return (-DATA_ERR);//���ش���
	if(in[0]>2)//�޶�ֻ��д0x00,0X01,0x02;
		return (-DATA_ERR);//����
	dev.temp_param.mode=in[0];
//	write_param_to_flash();//д��Flash
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x01;
}
static int get_air_coner_switch(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	put_le_val(dev.temp_param.air_coner_switch,out,0x01);//�����ݷ������� ��
	return 0x01;
}
static int set_air_coner_switch(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if(len!=0x01)
		return (-DATA_ERR);//�������ݳ��ȴ��� 
	if(in[0]>1)//�޶�ֻ��д0x00,0X01;
		return (-DATA_ERR);//�������ݳ��ȴ���
	dev.temp_param.air_coner_switch=in[0];
//	write_param_to_flash();//д��Flash
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x01;
}
static int get_panel_lock(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	put_le_val(dev.temp_param.panel_lock,out,0x01);
	return 0x01;
}
static int set_panel_lock(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if(len!=0x01)
		return (-DATA_ERR);
	if(in[0]>1)//�޶�ֻ��д0x00,0X01;
		return (-DATA_ERR);
	dev.temp_param.panel_lock=in[0];
//	write_param_to_flash(); 
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x01;
}
static int get_speed_ctrl_mode(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
  put_le_val(dev.auto_speed_ctrl_mode,out,0x01);
	return 0x01;
}
static int set_speed_ctrl_mode(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
  if(len!=0x01)
		return (-DATA_ERR);
	if(in[0]>1)//�޶�ֻ��д0x00,0X01;
		return (-DATA_ERR);
	dev.auto_speed_ctrl_mode=in[0];
//	write_param_to_flash();
    save_dev_flag=SAVE_DEV;//�ṹ�����ݴ洢��FLASH��־��Ϊ��Ч
	write_to_flash();
	return 0x01;
} 
#if DEV_SHOW


static int set_silent_time(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
	if(len != 0x02) return(-DATA_ERR);//len is err
	    dev_search_param.silent_time = get_le_val(in,TIME_LEN);
	return(NO_ERR);
}

static int get_pwd(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
    if(maxlen < 0x01) return(0);
        
    memcpy(out, dev.encode.pwd,PW_LEN); 
	return(PW_LEN);
}

static int set_dev_show(const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen)
{
    if(len) return(-DATA_ERR);
    
    dev_search_param.dev_show_flag = 0x01;
    return(NO_ERR);
}
#endif
#define scat_flag(f) { (uint8_t)(f), (uint8_t)(f >> 8) }
struct func_ops func_items[] = {
    { scat_flag(0x0001), get_device_type,          NULL                    },
    { scat_flag(0x0002), get_app_comm_ver,         NULL                    },
    { scat_flag(0x0003), get_dev_soft_ver,         NULL                    },
    { scat_flag(0x0005), get_dev_key,              NULL                    },
    { scat_flag(0x0007), get_dev_sn,               NULL                    },
	
	{ scat_flag(0xC030), NULL,                     chk_pwd 			  },
	{ scat_flag(0xD005), get_report_switch,                  set_report_switch    				  },
	{ scat_flag(0XE004), get_temp_set,                     set_temp_set 		  },
	
    { scat_flag(0XE005), get_temp_range,                     set_temp_range                 },
    { scat_flag(0xE010), get_low_temp_protect,         set_low_temp_protect                    },  //supplement: sensor type 0x0E,0x0F
    { scat_flag(0xE011), get_win_speed_ctrl,          set_win_speed_ctrl                    },
    { scat_flag(0xE012), get_mode_ctrl,         set_mode_ctrl        },
    { scat_flag(0xE013), get_air_coner_switch,  set_air_coner_switch },
    { scat_flag(0xE014), get_panel_lock,          set_panel_lock         },
    { scat_flag(0xB701), get_sensor_value,          NULL         },
    { scat_flag(0xD103), get_report_step,        set_report_step                    },  //newly increased
    { scat_flag(0xD104), get_report_freq,  set_report_freq },
    { scat_flag(0xC01A), NULL,             NULL            },
    { scat_flag(0xD702), get_power_on_delay_time,        set_power_on_delay_time       },
    { scat_flag(0xFF08), get_dbg_info,                 set_dbg_info                },
    { scat_flag(0XB691), get_sensor_value,               NULL              },
    { scat_flag(0XE002), get_temp_set_E002,                 set_temp_set_E002                },
//    { scat_flag(0xE016), get_dew_point_prot_temp,  set_dew_point_prot_temp },  //newly increased
	{ scat_flag(0xE018), get_speed_ctrl_mode,		   set_speed_ctrl_mode 		   },  //newly increased
	

#if DEV_SHOW
    { scat_flag(0x0009), NULL,     set_dev_show    },  //only for debug
    { scat_flag(0x000A), get_pwd,            NULL           },  //only for debug
    { scat_flag(0x000B), NULL,             set_silent_time            },  //only for debug low temperayure protect control logic
#endif
		
};
struct func_ops *get_option(uint8_t *did)
{
    uint8_t i;

    for (i = 0; i < array_size(func_items); i++)
    {
        if (!memcmp(func_items[i].did, did, sizeof(func_items[i].did)))
            return (&func_items[i]);
    }

    return (NULL);
}
static uint8_t form_error_body(void *out, uint8_t *did, uint16_t err)
{
    struct Body *body = (struct Body *)out;

    memcpy(body->did, did, sizeof(body->did));
    body->ctrl = 0x82;
    put_le_val(err, body->data, 2);

    return (offsetof(struct Body, data) + get_bits(body->ctrl, 0, 6));
}
uint8_t do_cmd(uint8_t cmd, uint8_t data[], uint8_t len)
{
    uint8_t outidx = 0, inidx = 0;

    /* move the data region to the end of the cache buffer */
    inidx = MAX_BUFFER_SZ - 1 - len;
    memmove(&g_frame_buffer[inidx], &data[outidx], len);

    while (len >= FBD_FRAME_HEAD)
    {
        struct func_ops *op;
        struct Body *body  = (struct Body *)&g_frame_buffer[inidx];
        struct Body *outbd = (struct Body *)&data[outidx];
        int (*handler) (const uint8_t *in, uint8_t len, uint8_t *out, uint8_t maxlen) = NULL;

        uint8_t dlen = get_bits(body->ctrl, 0, 6);
        if (len < FBD_FRAME_HEAD + dlen)
        {
            /* length error */
            outidx += form_error_body(outbd, body->did, LEN_ERR);
            break;
        }

        inidx += FBD_FRAME_HEAD + dlen;
        len   -= FBD_FRAME_HEAD + dlen;

        op = get_option(body->did);

        if (cmd == CMD_GET)
            handler = op ? op->read : NULL;
        else
            handler = op ? op->write : NULL;
        if (handler)
        {
            uint8_t maxlen = min(body->data - outbd->data, 128);
            int ret        = handler(body->data, dlen, outbd->data, maxlen);
            if (ret < 0)
            {
                if (ret == -NO_RETURN)
                    continue;
                /* got an error */
                form_error_body(outbd, body->did, -ret);
            }
            else if (ret > 0)
            {
                /* got an extra data to ack */
                memcpy(outbd->did, body->did, sizeof(body->did));
//                if (outbd->did[1] == 0xC0 && outbd->did[0] == 0x18)
//                    outbd->did[0] = 0x12;
                outbd->ctrl = ret;
            }
            else
            {
                /* no error */
                memcpy(outbd->did, body->did, FBD_FRAME_HEAD + dlen);
            }
        }
        else
        {
            /* not found the data id */
            form_error_body(outbd, body->did, DID_ERR);
        }
        outidx += FBD_FRAME_HEAD + get_bits(outbd->ctrl, 0, 6);
    }
    return (outidx);
}

