#ifndef _SYSEEPROM_INTERFACE_H_
#define _SYSEEPROM_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for SYSEEPROM
struct syseeprom_fn_if {
    int (*get_syseeprom_size)(void *driver);
    ssize_t (*read_syseeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
    ssize_t (*write_syseeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
    unsigned char bus;
    unsigned char addr;
    unsigned int size;
    unsigned char mux_addr;
    unsigned char mux_channel;
};

#define SYSEEPROM_DEV_VALID(dev) \
    if (dev == NULL) \
        return (-1);

//SYSEEPROM ERROR CODE
#define ERR_SYSEEPROM_INIT_FAIL ((ERR_MODULLE_SYSEEPROM << 16) | 0x1)
#define ERR_SYSEEPROM_REG_FAIL ((ERR_MODULLE_SYSEEPROM << 16) | 0x2)

struct syseeprom_fn_if *get_syseeprom(void);
int syseeprom_if_create_driver(void);
void syseeprom_if_delete_driver(void);
#endif //_SYSEEPROM_INTERFACE_H_


