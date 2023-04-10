#ifndef _DRV_VOL_SENSOR_H_
#define _DRV_VOL_SENSOR_H_

#include "voltage_interface.h"

struct drv_vol_sensor {
    struct voltage_fn_if voltage_if;
    //private
    void __iomem *voltage_base;
    unsigned int index;
};

#define VOLTAGE_CHIP_NUM 2

#define VOLTAGE_BASE_ADDRESS           (0x0300)

//register define
#define VOLTAGE_VERSION_ADDR           (0x4)

#endif //_DRV_VOL_SENSOR_H_
