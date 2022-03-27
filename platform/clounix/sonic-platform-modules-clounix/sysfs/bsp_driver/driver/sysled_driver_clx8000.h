#ifndef _SYSLED_DRIVER_CLX8000_H_
#define _SYSLED_DRIVER_CLX8000_H_

#include "sysled_interface.h"

struct sysled_driver_clx8000 {
    struct sysled_fn_if sysled_if;
    //private
    void __iomem *sysled_base;
    unsigned int index;
};

#define LED_MANAGER_OFFSET (0x200)
#define LED_MASK (0x3)

#define FRONT_PANEL_CFG (0x0)
#define BACK_PANEL_CFG (0x4)
#define P80_STATUS_CFG (0x10)

#define OFF(x) (x[0])
#define BLUE(x) (x[1])
#define GREEN(x) (x[2])
#define YELLOW(x) (x[3])
#define RED(x) (x[4])
#define BLINK(x) (x[5])

#endif //_SYSLED_DRIVER_CLX8000_H_
