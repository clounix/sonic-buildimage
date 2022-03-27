/*
 * watchdog_device_driver.c
 *
 * This module realize /sys/s3ip/watchdog attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "watchdog_sysfs.h"
#include "watchdog_interface.h"

static int g_loglevel = 0;

/****************************************watchdog*********************************************/
static ssize_t clx_get_watchdog_loglevel(char *buf, size_t count)
{
    return sprintf(buf, "0x%x\n", g_dev_loglevel[CLX_DRIVER_TYPES_WATCHDOG]);
}

static ssize_t clx_set_watchdog_loglevel(char *buf, size_t count)
{
    int loglevel = 0;

    if (kstrtouint(buf, 16, &loglevel))
    {
        return -EINVAL;
    }
    g_dev_loglevel[CLX_DRIVER_TYPES_WATCHDOG] = loglevel;
    return count;
}

static ssize_t clx_get_watchdog_debug(char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t clx_set_watchdog_debug(char *buf, size_t count)
{
    return -ENOSYS;
}
/*
 * clx_get_watchdog_identify - Used to get watchdog identify, such as iTCO_wdt
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_watchdog_identify(char *buf, size_t count)
{
    struct watchdog_fn_if *watchdog_dev = get_watchdog();

    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev->get_watchdog_identify);
    return watchdog_dev->get_watchdog_identify(watchdog_dev, buf, count);
}

/*
 * clx_get_watchdog_state - Used to get watchdog state,
 * filled the value to buf, 0: inactive, 1: active
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_watchdog_state(char *buf, size_t count)
{
    struct watchdog_fn_if *watchdog_dev = get_watchdog();

    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev->get_watchdog_state);
    return watchdog_dev->get_watchdog_state(watchdog_dev, buf, count);
}

/*
 * clx_get_watchdog_timeleft - Used to get watchdog timeleft,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_watchdog_timeleft(char *buf, size_t count)
{
    struct watchdog_fn_if *watchdog_dev = get_watchdog();

    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev->get_watchdog_timeleft);
    return watchdog_dev->get_watchdog_timeleft(watchdog_dev, buf, count);
}

/*
 * clx_get_watchdog_timeout - Used to get watchdog timeout,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_watchdog_timeout(char *buf, size_t count)
{
    struct watchdog_fn_if *watchdog_dev = get_watchdog();

    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev->get_watchdog_timeout);
    return watchdog_dev->get_watchdog_timeout(watchdog_dev, buf, count);
}

/*
 * clx_set_watchdog_timeout - Used to set watchdog timeout,
 * @value: timeout value
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_watchdog_timeout(int value)
{
    struct watchdog_fn_if *watchdog_dev = get_watchdog();

    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev->set_watchdog_timeout);
    return watchdog_dev->set_watchdog_timeout(watchdog_dev, value);
}

/*
 * clx_get_watchdog_enable_status - Used to get watchdog enable status,
 * filled the value to buf, 0: disable, 1: enable
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_watchdog_enable_status(char *buf, size_t count)
{
    struct watchdog_fn_if *watchdog_dev = get_watchdog();

    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev->get_watchdog_enable_status);
    return watchdog_dev->get_watchdog_enable_status(watchdog_dev, buf, count);
}

/*
 * clx_set_watchdog_enable_status - Used to set watchdog enable status,
 * @value: enable status value, 0: disable, 1: enable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_watchdog_enable_status(int value)
{
    struct watchdog_fn_if *watchdog_dev = get_watchdog();

    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev->set_watchdog_enable_status);
    return watchdog_dev->set_watchdog_enable_status(watchdog_dev, value);
}

/*
 * clx_set_watchdog_reset - Used to set watchdog enable status,
 * @value: enable status value, 0: no reset, 1: reset
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_watchdog_reset(int value)
{
    struct watchdog_fn_if *watchdog_dev = get_watchdog();

    WATCHDOG_DEV_VALID(watchdog_dev);
    WATCHDOG_DEV_VALID(watchdog_dev->set_watchdog_reset);
    return watchdog_dev->set_watchdog_reset(watchdog_dev, value);
}
/*************************************end of watchdog*****************************************/

static struct s3ip_sysfs_watchdog_drivers_s drivers = {
    /*
     * set ODM watchdog sensor drivers to /sys/s3ip/watchdog,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_loglevel = clx_get_watchdog_loglevel,
    .set_loglevel = clx_set_watchdog_loglevel,
    .get_debug = clx_get_watchdog_debug,
    .set_debug = clx_set_watchdog_debug,
    .get_watchdog_identify = clx_get_watchdog_identify,
    .get_watchdog_state = clx_get_watchdog_state,
    .get_watchdog_timeleft = clx_get_watchdog_timeleft,
    .get_watchdog_timeout = clx_get_watchdog_timeout,
    .set_watchdog_timeout = clx_set_watchdog_timeout,
    .get_watchdog_enable_status = clx_get_watchdog_enable_status,
    .set_watchdog_enable_status = clx_set_watchdog_enable_status,
    .set_watchdog_reset = clx_set_watchdog_reset,    
};

static int __init watchdog_dev_drv_init(void)
{
    int ret;

    WDT_INFO("watchdog_init...\n");
    watchdog_if_create_driver();    

    ret = s3ip_sysfs_watchdog_drivers_register(&drivers);
    if (ret < 0) {
        WDT_ERR("watchdog drivers register err, ret %d.\n", ret);
        return ret;
    }
    WDT_INFO("watchdog create success.\n");
    return 0;
}

static void __exit watchdog_dev_drv_exit(void)
{
    watchdog_if_delete_driver(); 
    s3ip_sysfs_watchdog_drivers_unregister();
    WDT_INFO("watchdog_exit success.\n");
    return;
}

module_init(watchdog_dev_drv_init);
module_exit(watchdog_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("watchdog device driver");
