#ifndef _TYPES_H
#define _TYPES_H

#include <stddef.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include "errno.h"
#include "compiler.h"

typedef long                    base_t;      /* Nbit CPU related date type */
typedef unsigned long           ubase_t;     /* Nbit unsigned CPU related data type */

typedef base_t                  err_t;       /* Type for error number */
typedef uint32_t                time_t;      /* Type for time stamp */
typedef uint32_t                tick_t;      /* Type for tick count */
typedef base_t                  flag_t;      /* Type for flags */
typedef ubase_t                 dev_t;       /* Type for device */
typedef base_t                  off_t;       /* Type for offset */

struct list_head
{
    struct list_head *next, *prev;
};

typedef ubase_t sigset_t;

enum SIGNAL_TYPE
{
    SIGALARM = 1 << 0,
    SIGUART  = 1 << 1,
    SIGKEY   = 1 << 2,
    SIGTO    = 1 << 3,
    SIGMSTO  = 1 << 4,
	SIGMSFOUR  = 1 << 5,////
    SIGUSR1  = 1 << 8,
    SIGUSR2  = 1 << 9,
};

enum KEY_EVENT_TYPE
{
    KEY_EVENT_TRIG,
    KEY_EVENT_LONG_TRIG,
};
struct key_event
{
    uint8_t keyno;
    enum KEY_EVENT_TYPE keyevt;
};

/**
 * Base structure
 */

typedef struct object* object_t;
struct object
{
    const char *name;
    struct list_head entry;                 /* list node of kernel object */
};

/**
 * block device geometry structure
 */
struct device_blk_geometry
{
    uint32_t sector_count;                           /**< count of sectors */
    uint32_t bytes_per_sector;                       /**< number of bytes per sector */
    uint32_t block_size;                             /**< number of bytes to erase one block */
};

#endif	/* end of _TYPES_H */
