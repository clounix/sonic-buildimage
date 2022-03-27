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

extern int g_dev_loglevel[];
#define SYSLED_LOG_INFO(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_SYSLED], INFO, fmt, ##args); \
} while (0)

#define SYSLED_LOG_ERR(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_SYSLED], ERR, fmt, ##args); \
} while (0)

#define SYSLED_LOG_DBG(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_SYSLED], DBG, fmt, ##args); \
} while (0)

#define SYSLED_INFO(fmt, args...) SYSLED_LOG_INFO("sysled@INFO ", fmt, ##args)
#define SYSLED_ERR(fmt, args...)  SYSLED_LOG_ERR("sysled@ERR ", fmt, ##args)
#define SYSLED_DBG(fmt, args...)  SYSLED_LOG_DBG("sysled@DBG ", fmt, ##args)

struct sysled_fn_if *get_sysled(void);
void sysled_if_create_driver(void);
void sysled_if_delete_driver(void);
#endif //_SYSLED_INTERFACE_H_


