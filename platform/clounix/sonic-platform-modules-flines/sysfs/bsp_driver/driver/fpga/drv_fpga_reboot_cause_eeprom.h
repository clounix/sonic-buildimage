#ifndef _RBOOT_EEPROM_DRIVER_H_
#define _RBOOT_EEPROM_DRIVER_H_

#include "reboot_eeprom_interface.h"

#define CLX_REBOOT_EEPROM_BUS 10
#define CLX_REBOOT_EEPROM_ADDR 0x50

struct reboot_eeprom_driver
{
    struct reboot_eeprom_fn_if reboot_eeprom_if;
    // private
    void __iomem *reboot_eeprom_base;
    unsigned int index;
    unsigned char bus;
    unsigned char addr;
};

#endif //_RBOOT_EEPROM_DRIVER_H_
