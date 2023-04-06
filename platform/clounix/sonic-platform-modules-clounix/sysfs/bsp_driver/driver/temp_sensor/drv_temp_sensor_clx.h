#ifndef _DRV_TEMP_SENSOR_CLX_H_
#define _DRV_TEMP_SENSOR_CLX_H_

#include "temp_interface.h"

struct drv_temp_sensor_clx {
    struct temp_fn_if temp_if;
    //private
    void __iomem *temp_base;
    unsigned int index;
};

#define TEMP_CHIP_NUM 2

#define TEMP_BASE_ADDRESS           (0x0300)

//register define
#define TEMP_VERSION_ADDR           (0x4)

#endif //_DRV_TEMP_SENSOR_CLX_H_
