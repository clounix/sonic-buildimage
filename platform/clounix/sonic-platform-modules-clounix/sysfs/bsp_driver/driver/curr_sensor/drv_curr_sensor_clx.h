#ifndef _DRV_CURR_SENSOR_CLX_H_
#define _DRV_CURR_SENSOR_CLX_H_

#include "current_interface.h"

struct drv_curr_sensor_clx {
    struct current_fn_if current_if;
    //private
    void __iomem *current_base;
    unsigned int index;
};

#define CURRENT_CHIP_NUM 2

#define CURRENT_BASE_ADDRESS           (0x0300)

//register define
#define CURRENT_VERSION_ADDR           (0x4)

#endif //_DRV_CURR_SENSOR_CLX_H_
