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
    struct sensor_descript sensor_map_index[SENSOR_MAP_INDEX_MAX];
};

#define TEMP_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//TEMP ERROR CODE
#define ERR_TEMP_INIT_FAIL ((ERR_MODULLE_TEMP << 16) | 0x1)
#define ERR_TEMP_REG_FAIL ((ERR_MODULLE_TEMP << 16) | 0x2)

struct temp_fn_if *get_temp(void);
int temp_if_create_driver(void);
void temp_if_delete_driver(void);
#endif //_TEMP_INTERFACE_H_
