#ifndef _CPLD_INTERFACE_H_
#define _CPLD_INTERFACE_H_
#include "device_driver_common.h"
#include "clx_driver.h"

void cpld_dev_log(int level, const char *format, ...);

//interface for CPLD
struct cpld_fn_if {
    int (*get_main_board_cpld_number)(void * driver);
    ssize_t (*get_main_board_cpld_alias)(void * driver, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_type)(void * driver, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_firmware_version)(void *driver, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_board_version)(void * driver, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_test_reg)(void * driver, unsigned int cpld_index, char *buf, size_t count);
    int (*set_main_board_cpld_test_reg)(void * driver, unsigned int cpld_index, unsigned int value);
};

#define CPLD_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//CPLD ERROR CODE
#define ERR_CPLD_INIT_FAIL ((ERR_MODULLE_CPLD << 16) | 0x1)
#define ERR_CPLD_REG_FAIL ((ERR_MODULLE_CPLD << 16) | 0x2)

struct cpld_fn_if *get_cpld(void);
int cpld_if_create_driver(void);
void cpld_if_delete_driver(void);
#endif //_CPLD_INTERFACE_H_


