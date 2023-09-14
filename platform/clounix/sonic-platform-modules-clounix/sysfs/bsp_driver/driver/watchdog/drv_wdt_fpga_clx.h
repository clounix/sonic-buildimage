#ifndef _DRV_WDT_FPGA_CLX_H_
#define _DRV_WDT_FPGA_CLX_H_

#include "watchdog_interface.h"
#include "drv_wdt_clx.h"

#define WATCHDOG_CHIP_NUM 2

#define WATCHDOG_BASE_ADDRESS           (0x0100)

#define WATCHDOG_CONFIG          0x0110
#define WATCHDOG_STATUS          0x0114
#define WATCHDOG_FEED            0x0118

/*bit field in WATCHDOG_CONFIG*/
#define WATCHDOG_CONFIG_ENABLE_OFFSET 31
#define WATCHDOG_CONFIG_ENABLE_SIZE 1
#define WATCHDOG_CONFIG_RST_OFFSET 30
#define WATCHDOG_CONFIG_RST_SIZE 1
#define WATCHDOG_CONFIG_CLEAR_OFFSET 24
#define WATCHDOG_CONFIG_CLEAR_SIZE 1
#define WATCHDOG_CONFIG_TIMEOUT_OFFSET 0
#define WATCHDOG_CONFIG_TIMEOUT_SIZE 8

/*bit field in WATCHDOG_STATUS*/
#define WATCHDOG_STATUS_REBOOT_OFFSET 31
#define WATCHDOG_STATUS_REBOOT_SIZE 1
#define WATCHDOG_STATUS_CNT_OFFSET 0
#define WATCHDOG_STATUS_CNT_SIZE 8
/*bit field in WATCHDOG_FEED*/
#define WATCHDOG_FEED_SET_OFFSET 0
#define WATCHDOG_FEED_SET_SIZE 1

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
