#ifndef _FPGA_INTERFACE_H_
#define _FPGA_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for fpga
struct fpga_fn_if {
    int (*get_main_board_fpga_number)(void * driver);
    ssize_t (*get_main_board_fpga_alias)(void * driver, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_type)(void * driver, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_firmware_version)(void *driver, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_board_version)(void * driver, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_test_reg)(void * driver, unsigned int fpga_index, char *buf, size_t count);
    int (*set_main_board_fpga_test_reg)(void * driver, unsigned int fpga_index, unsigned int value);
};

#define FPGA_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//fpga ERROR CODE
#define ERR_FPGA_INIT_FAIL ((ERR_MODULLE_fpga << 16) | 0x1)
#define ERR_FPGA_REG_FAIL ((ERR_MODULLE_fpga << 16) | 0x2)

extern int g_dev_loglevel[];
#define FPGA_LOG_INFO(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_FPGA], INFO, fmt, ##args); \
} while (0)

#define FPGA_LOG_ERR(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_FPGA], ERR, fmt, ##args); \
} while (0)

#define FPGA_LOG_DBG(_prefix, fmt, args...) do { \
    dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_FPGA], DBG, fmt, ##args); \
} while (0)

#define FPGA_INFO(fmt, args...) FPGA_LOG_INFO("fpga@INFO ", fmt, ##args)
#define FPGA_ERR(fmt, args...)  FPGA_LOG_ERR("fpga@ERR ", fmt, ##args)
#define FPGA_DBG(fmt, args...)  FPGA_LOG_DBG("fpga@DBG ", fmt, ##args)

struct fpga_fn_if *get_fpga(void);
void fpga_if_create_driver(void);
void fpga_if_delete_driver(void);
#endif //_FPGA_INTERFACE_H_


