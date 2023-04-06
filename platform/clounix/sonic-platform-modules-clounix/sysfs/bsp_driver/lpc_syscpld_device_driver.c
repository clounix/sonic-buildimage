#include <linux/slab.h>

#include "device_driver_common.h"
#include "lpc_syscpld_interface.h"

static int __init lpc_syscpld_dev_drv_init(void)
{
    return lpc_syscpld_if_create_driver();
}

static void __exit lpc_syscpld_dev_drv_exit(void)
{
    lpc_syscpld_if_delete_driver();
    return;
}

module_init(lpc_syscpld_dev_drv_init);
module_exit(lpc_syscpld_dev_drv_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("songqh@clounix.com");
MODULE_DESCRIPTION("lpc syscpld device driver");
