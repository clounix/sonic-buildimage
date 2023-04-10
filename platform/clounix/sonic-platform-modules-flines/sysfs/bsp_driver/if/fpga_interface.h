#ifndef _FPGA_INTERFACE_H_
#define _FPGA_INTERFACE_H_

#include "device_driver_common.h"
#include "clx_driver.h"

// interface for fpga
struct fpga_fn_if
{
    int (*get_main_board_fpga_number)(void *driver);
    ssize_t (*get_main_board_fpga_alias)(void *driver, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_type)(void *driver, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_firmware_version)(void *driver, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_board_version)(void *driver, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_test_reg)(void *driver, unsigned int fpga_index, char *buf, size_t count);
    int (*set_main_board_fpga_test_reg)(void *driver, unsigned int fpga_index, unsigned int value);
    int (*get_reboot_eeprom_size)(void *driver);
    ssize_t (*read_reboot_eeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
    ssize_t (*write_reboot_eeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
    unsigned char reboot_eeprom_bus;
    unsigned char reboot_eeprom_addr;
};

#define FPGA_DEV_VALID(dev) \
    if (dev == NULL)        \
        return (-1);

// fpga ERROR CODE
#define ERR_FPGA_INIT_FAIL ((ERR_MODULLE_fpga << 16) | 0x1)
#define ERR_FPGA_REG_FAIL ((ERR_MODULLE_fpga << 16) | 0x2)

struct fpga_fn_if *get_fpga(void);
int fpga_if_create_driver(void);
void fpga_if_delete_driver(void);
#endif //_FPGA_INTERFACE_H_
