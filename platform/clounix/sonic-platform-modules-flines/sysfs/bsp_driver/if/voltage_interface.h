#ifndef _VOLTAGE_INTERFACE_H_
#define _VOLTAGE_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for VOLTAGE
struct voltage_fn_if {
    int (*get_main_board_vol_number)(void *driver);
    ssize_t (*get_main_board_vol_alias)(void *driver, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_type)(void *driver, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_max)(void *driver, unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_max)(void *driver, unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_min)(void *driver, unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_min)(void *driver, unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_crit)(void *driver, unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_crit)(void *driver, unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_range)(void *driver, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_nominal_value)(void *driver, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_value)(void *driver, unsigned int vol_index, char *buf, size_t count);
    unsigned char total_sensor_num;
    short (*psensor_map)[3];
    unsigned char (*pvol_index_range_map)[2];    
};

#define VOLTAGE_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//VOLTAGE ERROR CODE
#define ERR_VOLTAGE_INIT_FAIL ((ERR_MODULLE_VOLTAGE << 16) | 0x1)
#define ERR_VOLTAGE_REG_FAIL ((ERR_MODULLE_VOLTAGE << 16) | 0x2)

struct voltage_fn_if *get_voltage(void);
int voltage_if_create_driver(void);
void voltage_if_delete_driver(void);
#endif //_VOLTAGE_INTERFACE_H_


