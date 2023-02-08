#ifndef _REBOOT_EEPROM_INTERFACE_H_
#define _REBOOT_EEPROM_INTERFACE_H_

#include "device_driver_common.h"
#include "clx_driver.h"

// interface for REBOOT EEPROM
struct reboot_eeprom_fn_if
{
    int (*get_reboot_eeprom_size)(void *driver);
    ssize_t (*read_reboot_eeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
    ssize_t (*write_reboot_eeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
};

#define REBOOT_EEPROM_DEV_VALID(dev) \
    if (dev == NULL)                 \
        return (-1);

struct reboot_eeprom_fn_if *get_reboot_eeprom(void);
void reboot_eeprom_if_create_driver(void);
void reboot_eeprom_if_delete_driver(void);
#endif //_REBOOT_EEPROM_INTERFACE_H_
