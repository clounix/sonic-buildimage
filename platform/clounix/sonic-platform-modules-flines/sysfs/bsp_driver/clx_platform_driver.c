/*
 * clx_platform_device_driver.c
 *
 * This module realize /sys/s3ip/clx_platform attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "clx_driver.h"
#include "clx_platform_interface.h"

int g_dev_loglevel[CLX_DRIVER_TYPES_MAX] = {
	[CLX_DRIVER_TYPES_CPLD] = ERR,
	[CLX_DRIVER_TYPES_FPGA] = ERR,
	[CLX_DRIVER_TYPES_I2C_MASTER] = ERR,
	[CLX_DRIVER_TYPES_SLOT] = ERR,
	[CLX_DRIVER_TYPES_XCVR] = ERR,
	[CLX_DRIVER_TYPES_SYSEEPROM] = ERR,
	[CLX_DRIVER_TYPES_FAN] = ERR,
	[CLX_DRIVER_TYPES_PSU] = ERR,
	[CLX_DRIVER_TYPES_TEMP] = ERR,
	[CLX_DRIVER_TYPES_CURR] = ERR,
	[CLX_DRIVER_TYPES_VOL] = ERR,
	[CLX_DRIVER_TYPES_WATCHDOG] = ERR,
	[CLX_DRIVER_TYPES_SYSLED] = ERR,
	[CLX_DRIVER_TYPES_PLT] = ERR,
	[CLX_DRIVER_TYPES_LPC] = ERR,
	[CLX_DRIVER_TYPES_REBOOT_EEPROM] = DBG,
};
EXPORT_SYMBOL_GPL(g_dev_loglevel);

static int arr_argc = 0;
static char* clx_platform = "x86_64-clounix_clx8000_48c8d-r0";

static int __init clx_platform_dev_drv_init(void)
{
    int i;
    for (i = 0; i < CLX_DRIVER_TYPES_MAX; i++)
        LOG_INFO(CLX_DRIVER_TYPES_PLT, "g_dev_loglevel:%d %d\n", i, g_dev_loglevel[i]);
    LOG_INFO(CLX_DRIVER_TYPES_PLT, "clx_platform_init...\n");
    clx_driver_init(clx_platform);
    LOG_INFO(CLX_DRIVER_TYPES_PLT, "clx_platform_init success.\n");
    return 0;
}

static void __exit clx_platform_dev_drv_exit(void)
{
    LOG_INFO(CLX_DRIVER_TYPES_PLT, "clx_platform_exit success.\n");
    return;
}

module_init(clx_platform_dev_drv_init);
module_exit(clx_platform_dev_drv_exit);

module_param_array(g_dev_loglevel, int, &arr_argc, 0644);
MODULE_PARM_DESC(g_dev_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");

module_param(clx_platform, charp, S_IRUSR);
MODULE_PARM_DESC(clx_platform, "product information.\n");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("clx_platform device driver");
