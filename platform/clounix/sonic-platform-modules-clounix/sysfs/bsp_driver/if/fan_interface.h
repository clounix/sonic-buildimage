#ifndef _FAN_INTERFACE_H_
#define _FAN_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for FAN
struct fan_fn_if {
    int (*get_fan_number)(void *driver);
    int (*get_fan_motor_number)(void *driver, unsigned int fan_index);
    ssize_t (*get_fan_vendor_name)(void *driver, unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_model_name)(void *driver, unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_serial_number)(void *driver, unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_part_number)(void *driver, unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_hardware_version)(void *driver, unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_status)(void *driver, unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_led_status)(void *driver, unsigned int fan_index, char *buf, size_t count);
    int (*set_fan_led_status)(void *driver, unsigned int fan_index, int status);
    ssize_t (*get_fan_direction)(void *driver, unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed)(void *driver, unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_tolerance)(void *driver, unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_target)(void *driver, unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_max)(void *driver, unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_min)(void *driver, unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_ratio)(void *driver, unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    int (*set_fan_motor_ratio)(void *driver, unsigned int fan_index, unsigned int motor_index, int ratio);
};

#define FAN_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//FAN ERROR CODE
#define ERR_FAN_INIT_FAIL ((ERR_MODULLE_FAN << 16) | 0x1)
#define ERR_FAN_REG_FAIL ((ERR_MODULLE_FAN << 16) | 0x2)

extern int g_dev_loglevel[];
#define FAN_LOG_INFO(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_FAN], INFO, fmt, ##args); \
} while (0)

#define FAN_LOG_ERR(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_FAN], ERR, fmt, ##args); \
} while (0)

#define FAN_LOG_DBG(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_FAN], DBG, fmt, ##args); \
} while (0)

#define FAN_INFO(fmt, args...) FAN_LOG_INFO("fan@INFO ", fmt, ##args)
#define FAN_ERR(fmt, args...)  FAN_LOG_ERR("fan@ERR ", fmt, ##args)
#define FAN_DBG(fmt, args...)  FAN_LOG_DBG("fan@DBG ", fmt, ##args)

//start with 0
#define FAN_INDEX_MAPPING(fan_index) \
    fan_index = fan_index - 1;

struct fan_fn_if *get_fan(void);
void fan_if_create_driver(void);
void fan_if_delete_driver(void);
#endif //_FAN_INTERFACE_H_


