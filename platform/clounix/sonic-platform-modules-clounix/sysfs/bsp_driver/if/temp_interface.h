#ifndef _TEMP_INTERFACE_H_
#define _TEMP_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for TEMP
struct temp_fn_if {
    int (*get_main_board_temp_number)(void *driver);
    ssize_t (*get_main_board_temp_alias)(void *driver, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_main_board_temp_type)(void *driver, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_main_board_temp_max)(void *driver, unsigned int temp_index, char *buf, size_t count);
    int (*set_main_board_temp_max)(void *driver, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_main_board_temp_max_hyst)(void *driver,unsigned int temp_index, char *buf, size_t count);
    int (*set_main_board_temp_max_hyst)(void *driver,unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_main_board_temp_min)(void *driver, unsigned int temp_index, char *buf, size_t count);
    int (*set_main_board_temp_min)(void *driver, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_main_board_temp_value)(void *driver, unsigned int temp_index, char *buf, size_t count);
};

#define TEMP_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//TEMP ERROR CODE
#define ERR_TEMP_INIT_FAIL ((ERR_MODULLE_TEMP << 16) | 0x1)
#define ERR_TEMP_REG_FAIL ((ERR_MODULLE_TEMP << 16) | 0x2)

extern int g_dev_loglevel[];
#define TEMP_LOG_INFO(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_TEMP], INFO, fmt, ##args); \
} while (0)

#define TEMP_LOG_ERR(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_TEMP], ERR, fmt, ##args); \
} while (0)

#define TEMP_LOG_DBG(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_TEMP], DBG, fmt, ##args); \
} while (0)

#define TEMP_SENSOR_INFO(fmt, args...) TEMP_LOG_INFO("temp sensor@INFO ", fmt, ##args)
#define TEMP_SENSOR_ERR(fmt, args...)  TEMP_LOG_ERR("temp sensor@ERR ", fmt, ##args)
#define TEMP_SENSOR_DBG(fmt, args...)  TEMP_LOG_DBG("temp sensor@DBG ", fmt, ##args)

struct temp_fn_if *get_temp(void);
void temp_if_create_driver(void);
void temp_if_delete_driver(void);
#endif //_TEMP_INTERFACE_H_


