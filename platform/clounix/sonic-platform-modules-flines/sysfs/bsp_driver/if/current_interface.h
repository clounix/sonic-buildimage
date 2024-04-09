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
    unsigned char real_max_sensor_num;
    short (*sensor_map)[SENSOR_COL_MAX];
    unsigned char (*index_range_map)[SENSOR_RANGE_MAX];
};

#define CURRENT_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//CURRENT ERROR CODE
#define ERR_CURRENT_INIT_FAIL ((ERR_MODULLE_CURRENT << 16) | 0x1)
#define ERR_CURRENT_REG_FAIL ((ERR_MODULLE_CURRENT << 16) | 0x2)

struct current_fn_if *get_curr(void);
int current_if_create_driver(void);
void current_if_delete_driver(void);
#endif //_CURRENT_INTERFACE_H_


