#ifndef _WATCHDOG_INTERFACE_H_
#define _WATCHDOG_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for WATCHDOG
struct watchdog_fn_if {
    ssize_t (*get_watchdog_identify)(void * driver, char *buf, size_t count);
    ssize_t (*get_watchdog_state)(void * driver, char *buf, size_t count);
    ssize_t (*get_watchdog_timeleft)(void * driver, char *buf, size_t count);
    ssize_t (*get_watchdog_timeout)(void * driver, char *buf, size_t count);
    int (*set_watchdog_timeout)(void * driver, int value);
    ssize_t (*get_watchdog_enable_status)(void * driver, char *buf, size_t count);
    int (*set_watchdog_enable_status)(void * driver, int value);
    int (*set_watchdog_reset)(void * driver, int value);
};

#define WATCHDOG_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//WATCHDOG ERROR CODE
#define ERR_WATCHDOG_INIT_FAIL ((ERR_MODULLE_WATCHDOG << 16) | 0x1)
#define ERR_WATCHDOG_REG_FAIL ((ERR_MODULLE_WATCHDOG << 16) | 0x2)

struct watchdog_fn_if *get_watchdog(void);
int watchdog_if_create_driver(void);
void watchdog_if_delete_driver(void);
#endif //_WATCHDOG_INTERFACE_H_


