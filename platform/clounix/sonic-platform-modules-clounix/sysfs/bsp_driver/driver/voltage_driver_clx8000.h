#ifndef _VOLTAGE_DRIVER_CLX8000_H_
#define _VOLTAGE_DRIVER_CLX8000_H_

#include "voltage_interface.h"

struct voltage_driver_clx8000 {
    struct voltage_fn_if voltage_if;
    //private
    void __iomem *voltage_base;
    unsigned int index;
};

#define VOLTAGE_CHIP_NUM 2

#define VOLTAGE_BASE_ADDRESS           (0x0300)

//register define
#define VOLTAGE_VERSION_ADDR           (0x4)

#endif //_VOLTAGE_DRIVER_CLX8000_H_
