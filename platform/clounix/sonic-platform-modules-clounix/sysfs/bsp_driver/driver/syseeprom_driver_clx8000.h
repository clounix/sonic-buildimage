#ifndef _SYSEEPROM_DRIVER_CLX8000_H_
#define _SYSEEPROM_DRIVER_CLX8000_H_

#include "syseeprom_interface.h"

struct syseeprom_driver_clx8000 {
    struct syseeprom_fn_if syseeprom_if;
    //private
    void __iomem *syseeprom_base;
    unsigned int index;
    unsigned char bus;
    unsigned char addr;
    unsigned char mux_addr;
    unsigned char mux_channel;        
};

#endif //_SYSEEPROM_DRIVER_CLX8000_H_
