#ifndef _DRV_WDT_CLX_H_
#define _DRV_WDT_CLX_H_

#include "watchdog_interface.h"

struct drv_wdt_clx {
    struct watchdog_fn_if watchdog_if;
    //private
    void __iomem *watchdog_base;
    unsigned int index;
};

#define WATCHDOG_CHIP_NUM 2

#define WATCHDOG_BASE_ADDRESS           (0x0300)

//register define
#define WATCHDOG_VERSION_ADDR           (0x4)

#endif //_DRV_WDT_CLX_H_
