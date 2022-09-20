#ifndef _CPLD_DRIVER_CLX8000_H_
#define _CPLD_DRIVER_CLX8000_H_

#include "cpld_interface.h"

struct cpld_driver_clx8000 {
    struct cpld_fn_if cpld_if;
    //private
    void __iomem *cpld_base;
    unsigned int index;
};

#define CPLD_CHIP_NUM 2

#define CPLD_BASE_ADDRESS           (0x0300)

//register define
#define CPLD_INTR_CFG_ADDR           (0x0)
#define CPLD_VERSION_ADDR           (0x4)

#define CPLD0_RST_BIT      31
#define CPLD0_EN_BIT       30
#define CPLD1_RST_BIT      29
#define CPLD1_EN_BIT       28


#endif //_CPLD_DRIVER_CLX8000_H_
