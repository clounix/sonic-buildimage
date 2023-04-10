#include "clx_driver.h"

extern int drv_i2c_xilinx_init(void **driver);
extern int drv_i2c_master_xilinx_init(void **driver);
extern int drv_i2c_anlogic_init(void **driver);
extern void drv_i2c_xilinx_exit(void **driver);
extern void drv_i2c_master_xilinx_exit(void **driver);
extern void drv_i2c_anlogic_exit(void **driver);

static struct driver_map i2c_master_drv_map[] = {
    {"drv_i2c_xilinx", drv_i2c_xilinx_init, drv_i2c_xilinx_exit},
    {"drv_i2c_anlogic", drv_i2c_anlogic_init, drv_i2c_anlogic_exit},
    {"drv_i2c_master_xilinx", drv_i2c_master_xilinx_init, drv_i2c_master_xilinx_exit},
};

struct i2c_master_fn_if *i2c_master_driver;

int i2c_master_if_create_driver(void)
{
    char *driver_type = NULL;
    struct driver_map *it;
    int i;

    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_I2C_MASTER);

    for (i = 0; i < sizeof(i2c_master_drv_map) / sizeof(struct driver_map); i++)
    {
        it = &i2c_master_drv_map[i];
        if (strcmp(driver_type, it->name) == 0)
        {
            return it->driver_init((void *)&i2c_master_driver);
        }
    }

    return -ENODATA;
}

void i2c_master_if_delete_driver(void)
{
    char *driver_type = NULL;
    struct driver_map *it;
    int i;

    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_I2C_MASTER);

    for (i = 0; i < sizeof(i2c_master_drv_map) / sizeof(struct driver_map); i++)
    {
        it = &i2c_master_drv_map[i];
        if (strcmp(driver_type, it->name) == 0)
        {
            it->driver_exit((void *)&i2c_master_driver);
            break;
        }
    }

    return;
}
