/*
 * reboot eeprom_device_driver.c
 *
 * This module realize /sys/s3ip/reboot eeprom attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "reboot_eeprom_sysfs.h"
#include "reboot_eeprom_interface.h"

static int g_loglevel = 1;

/*****************************************reboot eeprom*******************************************/
static ssize_t clx_get_reboot_eeprom_loglevel(char *buf, size_t count)
{
    return sprintf(buf, "0x%x\n", g_dev_loglevel[CLX_DRIVER_TYPES_REBOOT_EEPROM]);
}

static ssize_t clx_set_reboot_eeprom_loglevel(char *buf, size_t count)
{
    int loglevel = 0;

    if (kstrtouint(buf, 16, &loglevel))
    {
        return -EINVAL;
    }
    g_dev_loglevel[CLX_DRIVER_TYPES_REBOOT_EEPROM] = loglevel;
    return count;
}

static ssize_t clx_get_reboot_eeprom_debug(char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t clx_set_reboot_eeprom_debug(char *buf, size_t count)
{
    return -ENOSYS;
}

/*
 * clx_get_reboot_eeprom_size - Used to get reboot eeprom size
 *
 * This function returns the size of reboot eeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int clx_get_reboot_eeprom_size(void)
{
    struct reboot_eeprom_fn_if *reboot_eeprom_dev = get_reboot_eeprom();

    REBOOT_EEPROM_DEV_VALID(reboot_eeprom_dev);
    REBOOT_EEPROM_DEV_VALID(reboot_eeprom_dev->get_reboot_eeprom_size);
    return reboot_eeprom_dev->get_reboot_eeprom_size(reboot_eeprom_dev);
}

/*
 * clx_read_reboot_eeprom_data - Used to read reboot eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read reboot eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_read_reboot_eeprom_data(char *buf, loff_t offset, size_t count)
{
    struct reboot_eeprom_fn_if *reboot_eeprom_dev = get_reboot_eeprom();

    REBOOT_EEPROM_DEV_VALID(reboot_eeprom_dev);
    REBOOT_EEPROM_DEV_VALID(reboot_eeprom_dev->read_reboot_eeprom_data);
    return reboot_eeprom_dev->read_reboot_eeprom_data(reboot_eeprom_dev, buf, offset, count);
}

/*
 * clx_write_reboot_eeprom_data - Used to write reboot eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write reboot eeprom data
 * @count: length of buf
 *
 * This function returns the written length of reboot eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_write_reboot_eeprom_data(char *buf, loff_t offset, size_t count)
{
    struct reboot_eeprom_fn_if *reboot_eeprom_dev = get_reboot_eeprom();

    REBOOT_EEPROM_DEV_VALID(reboot_eeprom_dev);
    REBOOT_EEPROM_DEV_VALID(reboot_eeprom_dev->write_reboot_eeprom_data);
    return reboot_eeprom_dev->write_reboot_eeprom_data(reboot_eeprom_dev, buf, offset, count);
}
/*************************************end of reboot eeprom****************************************/

static struct s3ip_sysfs_reboot_eeprom_drivers_s drivers = {

    .get_loglevel = clx_get_reboot_eeprom_loglevel,
    .set_loglevel = clx_set_reboot_eeprom_loglevel,
    .get_debug = clx_get_reboot_eeprom_debug,
    .set_debug = clx_set_reboot_eeprom_debug,
    .get_reboot_eeprom_size = clx_get_reboot_eeprom_size,
    .read_reboot_eeprom_data = clx_read_reboot_eeprom_data,
    .write_reboot_eeprom_data = clx_write_reboot_eeprom_data,
};

static int __init reboot_eeprom_dev_drv_init(void)
{
    int ret = 0;

    LOG_INFO(CLX_DRIVER_TYPES_REBOOT_EEPROM, "reboot_eeprom_dev_drv_init...\n");
    reboot_eeprom_if_create_driver();

    ret = s3ip_sysfs_reboot_eeprom_drivers_register(&drivers);
    if (ret < 0)
    {
        LOG_ERR(CLX_DRIVER_TYPES_REBOOT_EEPROM, "reboot eeprom drivers register err, ret %d.\n", ret);
        return ret;
    }
    LOG_INFO(CLX_DRIVER_TYPES_REBOOT_EEPROM, "reboot_eeprom_dev_drv_init success.\n");
    return 0;
}

static void __exit reboot_eeprom_dev_drv_exit(void)
{
    reboot_eeprom_if_delete_driver();
    s3ip_sysfs_reboot_eeprom_drivers_unregister();
    LOG_INFO(CLX_DRIVER_TYPES_REBOOT_EEPROM, "reboot_eeprom_exit success.\n");
    return;
}

module_init(reboot_eeprom_dev_drv_init);
module_exit(reboot_eeprom_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("reboot eeprom device driver");
