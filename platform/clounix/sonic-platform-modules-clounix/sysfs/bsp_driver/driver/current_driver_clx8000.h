#ifndef _CURRENT_DRIVER_CLX8000_H_
#define _CURRENT_DRIVER_CLX8000_H_

#include "current_interface.h"

struct current_driver_clx8000 {
    struct current_fn_if current_if;
    //private
    void __iomem *current_base;
    unsigned int index;
};

#define CURRENT_CHIP_NUM 2

#define CURRENT_BASE_ADDRESS           (0x0300)

//register define
#define CURRENT_VERSION_ADDR           (0x4)

#endif //_CURRENT_DRIVER_CLX8000_H_
