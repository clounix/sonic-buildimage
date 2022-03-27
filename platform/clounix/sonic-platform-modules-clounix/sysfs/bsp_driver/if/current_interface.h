#ifndef _CURRENT_INTERFACE_H_
#define _CURRENT_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for CURRENT
struct current_fn_if {
    int (*get_main_board_curr_number)(void *driver);
    ssize_t (*get_main_board_curr_alias)(void *driver, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_type)(void *driver, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_max)(void *driver, unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_max)(void *driver, unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_min)(void *driver, unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_min)(void *driver, unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_crit)(void *driver, unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_crit)(void *driver, unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_average)(void *driver, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_value)(void *driver, unsigned int curr_index, char *buf, size_t count);
};

#define CURRENT_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//CURRENT ERROR CODE
#define ERR_CURRENT_INIT_FAIL ((ERR_MODULLE_CURRENT << 16) | 0x1)
#define ERR_CURRENT_REG_FAIL ((ERR_MODULLE_CURRENT << 16) | 0x2)

extern int g_dev_loglevel[];
//share debug with temp sensors
#define CURR_SENSOR_LOG_INFO(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_TEMP], INFO, fmt, ##args); \
} while (0)

#define CURR_SENSOR_LOG_ERR(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_TEMP], ERR, fmt, ##args); \
} while (0)

#define CURR_SENSOR_LOG_DBG(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_TEMP], DBG, fmt, ##args); \
} while (0)

#define CURR_SENSOR_INFO(fmt, args...) CURR_SENSOR_LOG_INFO("current@INFO ", fmt, ##args)
#define CURR_SENSOR_ERR(fmt, args...)  CURR_SENSOR_LOG_ERR("current@ERR ", fmt, ##args)
#define CURR_SENSOR_DBG(fmt, args...)  CURR_SENSOR_LOG_DBG("current@DBG ", fmt, ##args)

struct current_fn_if *get_curr(void);
void current_if_create_driver(void);
void current_if_delete_driver(void);
#endif //_CURRENT_INTERFACE_H_


