#include <linux/slab.h>

#include "device_driver_common.h"
#include "i2c_master_interface.h"

int g_loglevel = ERR;

static int __init i2c_master_dev_drv_init(void)
{
    return i2c_master_if_create_driver();
}

static void __exit i2c_master_dev_drv_exit(void)
{
    i2c_master_if_delete_driver();

    return;
}

module_init(i2c_master_dev_drv_init);
module_exit(i2c_master_dev_drv_exit);

module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("baohx@clounix.com");
MODULE_DESCRIPTION("i2c master device driver");
