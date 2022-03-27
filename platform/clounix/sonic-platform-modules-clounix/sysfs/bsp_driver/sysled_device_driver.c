/*
 * sysled_device_driver.c
 *
 * This module realize /sys/s3ip/sysled attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "sysled_sysfs.h"
#include "sysled_interface.h"

static int g_loglevel = 0;

/*****************************************sysled**********************************************/
static ssize_t clx_get_sysled_loglevel(char *buf, size_t count)
{
    return sprintf(buf, "0x%x\n", g_dev_loglevel[CLX_DRIVER_TYPES_SYSLED]);
}

static ssize_t clx_set_sysled_loglevel(char *buf, size_t count)
{
    int loglevel = 0;

    if (kstrtouint(buf, 16, &loglevel))
    {
        return -EINVAL;
    }
    g_dev_loglevel[CLX_DRIVER_TYPES_SYSLED] = loglevel;
    return count;
}

static ssize_t clx_get_sysled_debug(char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t clx_set_sysled_debug(char *buf, size_t count)
{
    return -ENOSYS;
}
/*
 * clx_get_sys_led_status - Used to get sys led status
 * filled the value to buf, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_sys_led_status(char *buf, size_t count)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->get_sys_led_status);
    return sysled_dev->get_sys_led_status(sysled_dev, buf, count);
}

/*
 * clx_set_sys_led_status - Used to set sys led status
 * @status: led status, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_sys_led_status(int status)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->set_sys_led_status);
    return sysled_dev->set_sys_led_status(sysled_dev, status);
}

/* Similar to clx_get_sys_led_status */
static ssize_t clx_get_bmc_led_status(char *buf, size_t count)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->get_bmc_led_status);
    return sysled_dev->get_bmc_led_status(sysled_dev, buf, count);
}

/* Similar to clx_set_sys_led_status */
static int clx_set_bmc_led_status(int status)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->set_bmc_led_status);
    return sysled_dev->set_bmc_led_status(sysled_dev, status);
}

/* Similar to clx_get_sys_led_status */
static ssize_t clx_get_sys_fan_led_status(char *buf, size_t count)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->set_id_led_status);
    return sysled_dev->get_sys_fan_led_status(sysled_dev, buf, count);
}

/* Similar to clx_set_sys_led_status */
static int clx_set_sys_fan_led_status(int status)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->set_sys_fan_led_status);
    return sysled_dev->set_sys_fan_led_status(sysled_dev, status);
}

/* Similar to clx_get_sys_led_status */
static ssize_t clx_get_sys_psu_led_status(char *buf, size_t count)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->get_sys_psu_led_status);
    return sysled_dev->get_sys_psu_led_status(sysled_dev, buf, count);
}

/* Similar to clx_set_sys_led_status */
static int clx_set_sys_psu_led_status(int status)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->set_sys_psu_led_status);
    return sysled_dev->set_sys_psu_led_status(sysled_dev, status);
}

/* Similar to clx_get_sys_led_status */
static ssize_t clx_get_id_led_status(char *buf, size_t count)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->get_id_led_status);
    return sysled_dev->get_id_led_status(sysled_dev, buf, count);
}

/* Similar to clx_set_sys_led_status */
static int clx_set_id_led_status(int status)
{
    struct sysled_fn_if *sysled_dev = get_sysled();

    SYSLED_DEV_VALID(sysled_dev);
    SYSLED_DEV_VALID(sysled_dev->set_id_led_status);
    return sysled_dev->set_id_led_status(sysled_dev, status);
}

/**************************************end of sysled******************************************/

static struct s3ip_sysfs_sysled_drivers_s drivers = {
    /*
     * set ODM sysled drivers to /sys/s3ip/sysled,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_loglevel = clx_get_sysled_loglevel,
    .set_loglevel = clx_set_sysled_loglevel,
    .get_debug = clx_get_sysled_debug,
    .set_debug = clx_set_sysled_debug,
    .get_sys_led_status = clx_get_sys_led_status,
    .set_sys_led_status = clx_set_sys_led_status,
    .get_bmc_led_status = clx_get_bmc_led_status,
    .set_bmc_led_status = clx_set_bmc_led_status,
    .get_sys_fan_led_status = clx_get_sys_fan_led_status,
    .set_sys_fan_led_status = clx_set_sys_fan_led_status,
    .get_sys_psu_led_status = clx_get_sys_psu_led_status,
    .set_sys_psu_led_status = clx_set_sys_psu_led_status,
    .get_id_led_status = clx_get_id_led_status,
    .set_id_led_status = clx_set_id_led_status,
};

static int __init sysled_init(void)
{
    int ret;

    SYSLED_INFO("sysled_init...\n");
    sysled_if_create_driver();

    ret = s3ip_sysfs_sysled_drivers_register(&drivers);
    if (ret < 0) {
        SYSLED_ERR("sysled drivers register err, ret %d.\n", ret);
        return ret;
    }

    SYSLED_INFO("sysled create success.\n");
    return 0;
}

static void __exit sysled_exit(void)
{
    sysled_if_delete_driver();
    s3ip_sysfs_sysled_drivers_unregister();
    SYSLED_INFO("sysled_exit ok.\n");
    return;
}

module_init(sysled_init);
module_exit(sysled_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("sysled device driver");
