#ifndef _TEMP_DRIVER_CLX8000_H_
#define _TEMP_DRIVER_CLX8000_H_

#include "temp_interface.h"

struct temp_driver_clx8000 {
    struct temp_fn_if temp_if;
    //private
    void __iomem *temp_base;
    unsigned int index;
};

#define TEMP_CHIP_NUM 2

#define TEMP_BASE_ADDRESS           (0x0300)

//register define
#define TEMP_VERSION_ADDR           (0x4)

#endif //_TEMP_DRIVER_CLX8000_H_
