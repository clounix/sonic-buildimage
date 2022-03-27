#ifndef _PSU_DRIVER_CLX8000_H_
#define _PSU_DRIVER_CLX8000_H_

#include "psu_interface.h"

struct psu_driver_clx8000 {
    struct psu_fn_if psu_if;
    //private
    void __iomem *psu_base;
    unsigned int index;
};

#define PSU_CHIP_NUM 2

#define PSU_BASE_ADDRESS           (0x0300)

//register define
#define PSU_VERSION_ADDR           (0x4)

#endif //_PSU_DRIVER_CLX8000_H_
