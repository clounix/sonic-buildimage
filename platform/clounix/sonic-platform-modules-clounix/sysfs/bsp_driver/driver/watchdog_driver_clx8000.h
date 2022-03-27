#ifndef _WATCHDOG_DRIVER_CLX8000_H_
#define _WATCHDOG_DRIVER_CLX8000_H_

#include "watchdog_interface.h"

struct watchdog_driver_clx8000 {
    struct watchdog_fn_if watchdog_if;
    //private
    void __iomem *watchdog_base;
    unsigned int index;
};

#define WATCHDOG_CHIP_NUM 2

#define WATCHDOG_BASE_ADDRESS           (0x0300)

//register define
#define WATCHDOG_VERSION_ADDR           (0x4)

#endif //_WATCHDOG_DRIVER_CLX8000_H_
