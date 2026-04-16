#ifndef _I2C_MASTER_INTERFACE_H_
#define _I2C_MASTER_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for i2c master

struct i2c_master_fn_if {
    void *data;
};

int i2c_master_if_create_driver(void);
void i2c_master_if_delete_driver(void);
#endif //_FPGA_INTERFACE_H_
