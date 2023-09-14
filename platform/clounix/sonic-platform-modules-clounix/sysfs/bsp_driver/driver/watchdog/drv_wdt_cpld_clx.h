#ifndef _DRV_WDT_CPLD_CLX_H_
#define _DRV_WDT_CLX_CPLD_H_

#include "watchdog_interface.h"
#include "drv_wdt_clx.h"


#define WATCHDOG_CHIP_NUM 2

#define WATCHDOG_BASE_ADDRESS           (0x0100)

#define WATCHDOG_ENABLE_CONFIG          0x0110
#define WATCHDOG_SET_CNT                0x0114
#define WATCHDOG_CNT_VAL                0x011c
#define WATCHDOG_WDI_ADDR               0x0118
/*bit mask*/
#define WATCHDOG_CNT_MASK               0xFFFF
#define WATCHDOG_ENABLE_BIT             0x01
#define WATCHDOG_WDI_BIT                0x01

//register define
#define WATCHDOG_VERSION_ADDR           (0x4)

#endif //_DRV_WDT_CPLD_CLX_H_
