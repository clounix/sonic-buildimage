#ifndef _DRV_WDT_CLX_H_
#define _DRV_WDT_CLX_H_

#include "watchdog_interface.h"

struct drv_wdt_clx {
    struct watchdog_fn_if watchdog_if;
    //private
    void __iomem *watchdog_base;
    unsigned int index;
};

/* Bit manipulation macros */
#define WATCHDOG_BIT(name)					\
	(1 << WATCHDOG_##name##_OFFSET)
#define WATCHDOG_BF(name,value)				\
	(((value) & ((1 << WATCHDOG_##name##_SIZE) - 1))	\
	 << WATCHDOG_##name##_OFFSET)
#define WATCHDOG_BFEXT(name,value) \
	(((value) >> WATCHDOG_##name##_OFFSET) \
     & ((1 << WATCHDOG_##name##_SIZE) - 1))
#define WATCHDOG_BFINS(name,value,old)			\
	(((old) & ~(((1 << WATCHDOG_##name##_SIZE) - 1)	\
		    << WATCHDOG_##name##_OFFSET))		\
	 | WATCHDOG_BF(name,value))

#endif //_DRV_WDT_FPGA_CLX_H_
