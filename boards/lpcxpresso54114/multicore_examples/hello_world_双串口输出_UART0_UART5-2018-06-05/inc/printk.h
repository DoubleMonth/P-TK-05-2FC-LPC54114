#ifndef _PRINTK_H_
#define _PRINTK_H_

#include <stdint.h>
#include "device.h"
/*
 * used to output log by modules
 * add your module before MODULE_NR
 */
 

enum
{
    MODULE_OS,						/* module for OS */
    MODULE_UART,                    /* module for uart */
    MODULE_APP,                     /* module for sdk layer */
    MODULE_NR,                      /* module for default */
};

#ifdef USING_EASYLOG
#include "elog.h"


/*
 * log_a, log_arr_a: emerg
 * log_e, log_arr_e: error
 * log_w, log_arr_w: warn
 * log_i, log_arr_i: info
 * log_d, log_arr_d: debug
 * log_v, log_arr_v: verb 
 *  
 * note: append newline character automatically
 */
#define log_a(m, ...) elog_a(m, ##__VA_ARGS__)
#define log_e(m, ...) elog_e(m, ##__VA_ARGS__)
#define log_w(m, ...) elog_w(m, ##__VA_ARGS__)
#define log_i(m, ...) elog_i(m, ##__VA_ARGS__)
#define log_d(m, ...) elog_d(m, ##__VA_ARGS__)
#define log_v(m, ...) elog_v(m, ##__VA_ARGS__)

#define log_arr_a(m, arr, len) elog_arr_a((m), (arr), (len))
#define log_arr_e(m, arr, len) elog_arr_e((m), (arr), (len))
#define log_arr_w(m, arr, len) elog_arr_w((m), (arr), (len))
#define log_arr_i(m, arr, len) elog_arr_i((m), (arr), (len))
#define log_arr_d(m, arr, len) elog_arr_d((m), (arr), (len))
#define log_arr_v(m, arr, len) elog_arr_v((m), (arr), (len))


#define print_debug_array(lvl, arr, len) \
    elog_array(MODULE_NR, (lvl), __FILE__, __func__, __LINE__, (arr), (len))

#else


# ifdef NDEBUG
#define log_a(m, ...) ((void)0)
#define log_e(m, ...) ((void)0)
#define log_w(m, ...) ((void)0)
#define log_i(m, ...) ((void)0)
#define log_d(m, ...) ((void)0)
#define log_v(m, ...) ((void)0)

#define print_debug_array(arr, len) ((void)0)

# else
/*
 * log_a, log_arr_a: emerg
 * log_e, log_arr_e: error
 * log_w, log_arr_w: warn
 * log_i, log_arr_i: info
 * log_d, log_arr_d: debug
 * log_v, log_arr_v: verb 
 *  
 * note: append newline character automatically
 */
#define log_a(m, ...) printk(__VA_ARGS__)
#define log_e(m, ...) printk(__VA_ARGS__)
#define log_w(m, ...) printk(__VA_ARGS__)
#define log_i(m, ...) printk(__VA_ARGS__)
#define log_d(m, ...) printk(__VA_ARGS__)
#define log_v(m, ...) printk(__VA_ARGS__)

#define print_debug_array(arr, len) \
    pr_arr((arr), (len))

# endif


void pr_arr(const void *arr, size_t len);
#endif

/*
 * used to upward compatibility
 */
#define pr_emerg(...)  log_a(MODULE_NR, __VA_ARGS__)
#define pr_alert(...)  log_a(MODULE_NR, __VA_ARGS__)
#define pr_crit(...)   log_a(MODULE_NR, __VA_ARGS__)
#define pr_err(...)    log_e(MODULE_NR, __VA_ARGS__)
#define pr_warn(...)   log_w(MODULE_NR, __VA_ARGS__)
#define pr_notice(...) log_e(MODULE_NR, __VA_ARGS__)
#define pr_info(...)   log_i(MODULE_NR, __VA_ARGS__)
#define pr_dbg(...)    log_d(MODULE_NR, __VA_ARGS__)


device_t console_get_device(void);
device_t console_set_device(const char *name);

void BUG(void);

#ifdef NDEBUG
# define assert(p) ((void)0)
#else
# define assert(p) do {	\
	if (!(p)) {	\
		pr_emerg("BUG at assert(%s)\n",	#p); \
        BUG(); \
	}		\
 } while (0)
#endif

void printk(const char *fmt, ...);
/*
 * initialize log 
 */
void setup_print(void);
#endif
