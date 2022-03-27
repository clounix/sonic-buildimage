#ifndef _PSU_INTERFACE_H_
#define _PSU_INTERFACE_H_

#include "device_driver_common.h"
#include "clx_driver.h"

//interface for PSU
struct psu_fn_if {
    int (*get_psu_number)(void *driver);
    int (*get_psu_temp_number)(void *driver, unsigned int psu_index);
    ssize_t (*get_psu_model_name)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_serial_number)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_part_number)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_hardware_version)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_type)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_curr)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_vol)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_power)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_curr)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_vol)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_power)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_max_power)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_present_status)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_status)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_status)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_speed)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_ratio)(void *driver, unsigned int psu_index, char *buf, size_t count);
    int (*set_psu_fan_ratio)(void *driver, unsigned int psu_index, int ratio);
    ssize_t (*get_psu_fan_direction)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_led_status)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_date)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_vendor)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_alarm)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_alarm_threshold_curr)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_alarm_threshold_vol)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_max_output_power)(void *driver, unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_alias)(void *driver, unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_type)(void *driver, unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_max)(void *driver, unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_max)(void *driver, unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_max_hyst)(void *driver, unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_max_hyst)(void *driver, unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_min)(void *driver, unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_min)(void *driver, unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_value)(void *driver, unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
};

#define PSU_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//PSU ERROR CODE
#define ERR_PSU_INIT_FAIL ((ERR_MODULLE_PSU << 16) | 0x1)
#define ERR_PSU_REG_FAIL ((ERR_MODULLE_PSU << 16) | 0x2)

extern int g_dev_loglevel[];
#define PSU_LOG_INFO(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_PSU], INFO, fmt, ##args); \
} while (0)

#define PSU_LOG_ERR(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_PSU], ERR, fmt, ##args); \
} while (0)

#define PSU_LOG_DBG(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_PSU], DBG, fmt, ##args); \
} while (0)

#define PSU_INFO(fmt, args...) PSU_LOG_INFO("psu@INFO ", fmt, ##args)
#define PSU_ERR(fmt, args...)  PSU_LOG_ERR("psu@ERR ", fmt, ##args)
#define PSU_DBG(fmt, args...)  PSU_LOG_DBG("psu@DBG ", fmt, ##args)

struct psu_fn_if *get_psu(void);
void psu_if_create_driver(void);
void psu_if_delete_driver(void);
#endif //_PSU_INTERFACE_H_


