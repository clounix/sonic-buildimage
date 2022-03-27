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

#define CLX_PLATFORM_INFO(fmt, args...) LOG_INFO("clx_platform: ", fmt, ##args)
#define CLX_PLATFORM_ERR(fmt, args...)  LOG_ERR("clx_platform: ", fmt, ##args)
#define CLX_PLATFORM_DBG(fmt, args...)  LOG_DBG("clx_platform: ", fmt, ##args)

static int g_loglevel = 0;

static int __init clx_platform_dev_drv_init(void)
{
    int ret;

    CLX_PLATFORM_INFO("clx_platform_init...\n");
	clx_driver_init();
    CLX_PLATFORM_INFO("clx_platform_init success.\n");
    return 0;
}

static void __exit clx_platform_dev_drv_exit(void)
{
    CLX_PLATFORM_INFO("clx_platform_exit success.\n");
    return;
}

module_init(clx_platform_dev_drv_init);
module_exit(clx_platform_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("clx_platform device driver");
