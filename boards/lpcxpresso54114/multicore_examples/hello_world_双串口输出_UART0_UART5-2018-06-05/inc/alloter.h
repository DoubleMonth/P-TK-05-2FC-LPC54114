#ifndef _ALLOTER_H_
#define _ALLOTER_H_

#include <types.h>

#define MAX_POOL_SZ     (0x400)
#define MAX_BUFFER_SZ   (MAX_POOL_SZ-BUFFER_NO_SZ)
#define BUFFER_NO_SZ    (MAX_POOL_SZ>>BLK_NO_SHIFT)

#define BLK_NO_SHIFT    6
#define BLK_SZ          (0x01 << BLK_NO_SHIFT)

#define INVALID_BLK_NO  0xFF
#define INVALID_PTR     (INVALID_BLK_NO << BLK_NO_SHIFT)

struct _CHN_SLOT
{
    int tx, rx, data_cnt, data_max;
};

struct _CHN_POOL_MGR
{
    uint8_t buffer[MAX_POOL_SZ];
    uint32_t free_bitmap;
};


void init_chn_pool_mgr(void);

void pool_init(struct _CHN_SLOT *slot, size_t maxsize);

int put_chn_bytes(struct _CHN_SLOT *pCHN_SLOT, const uint8_t buffer[], int len);
int get_chn_bytes(struct _CHN_SLOT *pCHN_SLOT, uint8_t buffer[], int len);
int peek_chn_bytes(struct _CHN_SLOT *pCHN_SLOT, uint8_t data[], int len);
#endif
