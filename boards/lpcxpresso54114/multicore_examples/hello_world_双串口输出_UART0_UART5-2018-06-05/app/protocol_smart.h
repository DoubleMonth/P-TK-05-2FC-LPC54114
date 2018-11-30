#ifndef _PROTOCOL_SMART_H_
#define _PROTOCOL_SMART_H_

#include <types.h>
#include <utils.h>

//---------------------------------------------------------------------------------------
#define STC             0x7E
#define DID_LEN         0x02
#define SID_LEN         0x02
#define AID_LEN         0x04
#define ID_LEN          0X04//
#define PW_LEN          0x02
#define PANID_LEN		0x02
#define BUF_LEN			0xFF
#define EID_LEN			0x08
#define PW_LEN          0x02
#define PSK_LEN 		0x08
#define SN_LEN          0x0C
#define DKEY_LEN        0x08
#define MAGIC_NUM_LEN	0x04
#define WDATA_NUM		0x0A
#define WDATA_LEN		0x40

//---------------------------------------------------------------------------------------
#define CMD_SET_AID     0x01
#define CMD_GET_AID     0x03
#define CMD_ACK_AID     0x13
#define CMD_DEL_AID     0x04
#define CMD_REQ_AID     0x05
#define CMD_GET_SID     0x06
#define CMD_ACK_SID     0x16
#define CMD_GET_EID     0x07
#define CMD_ACK_EID     0x17
#define CMD_SET_BPS     0x08
#define CMD_SET_REG     0x09
#define CMD_UNLINK      0x0A
#define CMD_REGINFOR    0x0B
#define CMD_SET_PANID   0x0C
#define CMD_GET_GWAID   0x0D
#define CMD_GET_VER     0x0E
#define CMD_ACK_VER     0x1E
#define CMD_GET_PANID   0x0F
#define CMD_ACK_PANID   0x1F
#define CMD_TST_PLC     0x20
#define CMD_CHG_TONE    0x21

#define CMD_ACK         0x00
#define CMD_NAK         0xFF

#define CMD_SET         0x07
#define CMD_GET        	0x02
#define CMD_UPDATE      0x05
#define CMD_SHOW 		0X04
#define CMD_RELI_REPORT     0x01
#define CMD_NRELI_REPORT    0x00


#define PRESSKEY_REG     0x01
#define PASSWORD_REG     0x00
#define PASSWORD_ERR     0x02
//---------------------------------------------------------------------------------------
#define NO_ERR        0x00
#define OTHER_ERR     0x0F
#define LEN_ERR       0x01
#define BUFFER_ERR    0x02
#define DATA_ERR      0x03
#define DID_ERR       0x04
#define DEV_BUSY      0x05
#define NO_RETURN     0x10
#define DATA_TRANS    0x12

#define TEMP_SENSOR     0x03
#define HUMI_SENSOR     0x04
#define STEP_LEN        0x02
#define FREQ_LEN        0x02
//#define TEMP_LEN        0X02
//#define HUMI_LEN        0X02

#pragma pack(1)
struct SmartFrame
{
    uint8_t stc;
    uint8_t said[AID_LEN];
    uint8_t taid[AID_LEN];
    uint8_t seq;
    uint8_t len;
    uint8_t data[1];
};

//struct SHS_frame
//{
//    uint8_t stc;
//    uint8_t said[ID_LEN];
//    uint8_t taid[ID_LEN];
//    uint8_t seq;
//    uint8_t len;
//    uint8_t infor[1];
//};
#define SMART_FRAME_HEAD offsetof(struct SmartFrame, data)//11

struct AppFrame
{
    uint8_t cmd;
    uint8_t data[0];
};
struct GroupFrame
{
    uint8_t len : 6;
    uint8_t type: 2;
    uint8_t data[1];
};

struct Body
{
    uint8_t did[DID_LEN];
    uint8_t ctrl;
    uint8_t data[0];
};
#define FBD_FRAME_HEAD  offsetof(struct Body, data)//3

struct RegData
{
    uint8_t aid[AID_LEN];
    uint8_t panid[SID_LEN];
    uint8_t pw[PW_LEN];
    uint8_t gid[AID_LEN];
    uint8_t sid[SID_LEN];
};
#pragma pack()

static inline int smart_frame_len(const struct SmartFrame *frame)
{
    return SMART_FRAME_HEAD + frame->len + 1;//加1是校验位
}
static inline int smart_frame_body_len(const struct Body *body)
{
    return get_bits(body->ctrl, 0, 6);
}
static inline bool smart_frame_is_broadcast(const struct SmartFrame *frame)
{
    return is_all_xx(frame->taid, 0xff, AID_LEN);
}
static inline bool smart_frame_is_local(const struct SmartFrame *pframe)
{
    return is_all_xx(pframe->said, 0x00, AID_LEN) && is_all_xx(pframe->taid, 0x00, AID_LEN);
}
static inline bool smart_frame_is_ack(const struct SmartFrame *frame)
{
    return tst_bit(frame->seq, 7);
}
struct func_ops
{
    uint8_t did[2];
    int (*read) (const uint8_t * in, uint8_t len, uint8_t * out, uint8_t maxlen);
    int (*write) (const uint8_t * in, uint8_t len, uint8_t * out, uint8_t maxlen);
};
struct SmartFrame *get_smart_frame(const uint8_t *in, int len);
int code_body(uint16_t did, int err, const void *data, int len, void *out, int maxlen);
int code_frame(const uint8_t *src, const uint8_t *dest, int seq, int cmd, 
    const uint8_t *data, int len, void *out, int maxlen);
int code_local_frame(const uint8_t *in, int len, void *out, int maxlen);

int code_ret_frame(struct SmartFrame *pframe, int len);
int smart_frame_handle(struct SmartFrame *pframe);
// is_gid_equal(const uint8_t *data, uint16_t mysid);
uint8_t do_cmd(uint8_t cmd, uint8_t data[], uint8_t len);
struct func_ops *get_option(uint8_t *did);
//uint8_t is_gid_equal(const uint8_t *data);
#endif
