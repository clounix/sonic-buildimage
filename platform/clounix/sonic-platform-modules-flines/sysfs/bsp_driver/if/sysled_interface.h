#ifndef _SYSLED_INTERFACE_H_
#define _SYSLED_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for SYSLED
struct sysled_fn_if {
    ssize_t (*get_sys_led_status)(void *driver, char *buf, size_t count);
    int (*set_sys_led_status)(void *driver, int status);
    ssize_t (*get_bmc_led_status)(void *driver, char *buf, size_t count);
    int (*set_bmc_led_status)(void *driver, int status);
    ssize_t (*get_sys_fan_led_status)(void *driver, char *buf, size_t count);
    int (*set_sys_fan_led_status)(void *driver, int status);
    ssize_t (*get_sys_psu_led_status)(void *driver, char *buf, size_t count);
    int (*set_sys_psu_led_status)(void *driver, int status);
    ssize_t (*get_id_led_status)(void *driver, char *buf, size_t count);
    int (*set_id_led_status)(void *driver, int status);
};

#define SYSLED_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//SYSLED ERROR CODE
#define ERR_SYSLED_INIT_FAIL ((ERR_MODULLE_SYSLED << 16) | 0x1)
#define ERR_SYSLED_REG_FAIL ((ERR_MODULLE_SYSLED << 16) | 0x2)

struct sysled_fn_if *get_sysled(void);
int sysled_if_create_driver(void);
void sysled_if_delete_driver(void);
#endif //_SYSLED_INTERFACE_H_


