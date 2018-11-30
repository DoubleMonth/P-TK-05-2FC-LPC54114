#ifndef	_SMART_PLC_H_
#define	_SMART_PLC_H_
#include <stdio.h>
#include <stdint.h>
#include "app_dev_ctrl.h"
#define VALID_DATA      0x05
#define INVALID_DATA    0x0A
enum
{
    INVALID=0, RST_PLC, R_EID, S_AID, S_PWDREG, S_PANID, S_BPS, 
    S_REG, UNLINK1, PWD_ERR, UNLINK2, G_GWID, G_SID,WAIT, _END
};
struct PLC_STATE 
{
    uint8_t cur_state;
    uint8_t next_state;

    uint8_t (*action)(uint8_t init, void *args);                            
};

struct PLC_MACHINE
{
    uint8_t init;
    uint8_t trycnt;
    uint8_t wait_t;
//    uint8 state;
    struct PLC_STATE *pstate;
};

struct REG
{
    uint8_t type;
    uint8_t last_status;
    uint8_t wait_t;
};
void plc_machine_opt(void *args);
void plc_init(void);
void plc_init(void);
void init_uart_infor(void);
int uart_pop_tx(int chn, uint8_t *out, int len);
int uart_push_rx(int chn, uint8_t *in, int len);
int uart_peek(int chn, uint8_t buf[], int len);
static void empty_a_chn_slot(struct _CHN_SLOT *pCHN_SLOT, uint8_t len);
void clear_uart(int chn, uint8_t len);
void uart_rx_hook(int chn);
void uart_tick_hook(void);
void plc_reset_on(void);
void plc_reset_off(void);
void plc_reset_toggle(void);
static struct PLC_STATE *get_plc_state(uint8_t state);
void chg_state(uint8_t cur_state);
#endif
