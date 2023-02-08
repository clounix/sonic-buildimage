#ifndef _DRV_SYSEEPROM_H_
#define _DRV_SYSEEPROM_H_

#include "syseeprom_interface.h"

struct drv_syseeprom {
    struct syseeprom_fn_if syseeprom_if;
    //private
    void __iomem *syseeprom_base;
    unsigned int index;
};

#endif //_DRV_SYSEEPROM_H_
