#ifndef _SYSEEPROM_INTERFACE_H_
#define _SYSEEPROM_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for SYSEEPROM
struct syseeprom_fn_if {
    int (*get_syseeprom_size)(void *driver);
    ssize_t (*read_syseeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
    ssize_t (*write_syseeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
};

#define SYSEEPROM_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//SYSEEPROM ERROR CODE
#define ERR_SYSEEPROM_INIT_FAIL ((ERR_MODULLE_SYSEEPROM << 16) | 0x1)
#define ERR_SYSEEPROM_REG_FAIL ((ERR_MODULLE_SYSEEPROM << 16) | 0x2)

extern int g_dev_loglevel[];
#define SYSEEPROM_LOG_INFO(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_SYSEEPROM], INFO, fmt, ##args); \
} while (0)

#define SYSEEPROM_LOG_ERR(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_SYSEEPROM], ERR, fmt, ##args); \
} while (0)

#define SYSEEPROM_LOG_DBG(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_SYSEEPROM], DBG, fmt, ##args); \
} while (0)

#define SYSE2_INFO(fmt, args...) SYSEEPROM_LOG_INFO("syseeprom@INFO ", fmt, ##args)
#define SYSE2_ERR(fmt, args...)  SYSEEPROM_LOG_ERR("syseeprom@ERR ", fmt, ##args)
#define SYSE2_DBG(fmt, args...)  SYSEEPROM_LOG_DBG("syseeprom@DBG ", fmt, ##args)

struct syseeprom_fn_if *get_syseeprom(void);
void syseeprom_if_create_driver(void);
void syseeprom_if_delete_driver(void);
#endif //_SYSEEPROM_INTERFACE_H_


