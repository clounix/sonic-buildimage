/*
 * syseeprom_device_driver.c
 *
 * This module realize /sys/s3ip/syseeprom attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "syseeprom_sysfs.h"
#include "syseeprom_interface.h"

/*****************************************syseeprom*******************************************/\
static ssize_t clx_get_syseeprom_loglevel(char *buf, size_t count)
{
    PRINT_LOGLEVEL(g_dev_loglevel[CLX_DRIVER_TYPES_SYSEEPROM], buf, count);
}

static ssize_t clx_set_syseeprom_loglevel(const char *buf, size_t count)
{
    int loglevel = 0;
    unsigned int base = 16;

    if (buf[1] == 'x') {
        base = 16;
    }
    else {
        base = 10;
    }
    if (kstrtouint(buf, base, &loglevel)) {
        return -EINVAL;
    }
    g_dev_loglevel[CLX_DRIVER_TYPES_SYSEEPROM] = loglevel;
    return count;
}

static ssize_t clx_get_syseeprom_debug(char *buf, size_t count)
{
    return sprintf(buf, "read eeprom: \n"
                        "hexdump -C /sys/switch/syseeprom/eeprom\n");
}

static ssize_t clx_set_syseeprom_debug(const char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t clx_get_bsp_version_loglevel(char *buf, size_t count)
{
    char *bsp_version = "1.0";

    return sprintf(buf, "%s\n", bsp_version);
}

/*
 * clx_get_syseeprom_size - Used to get syseeprom size
 *
 * This function returns the size of syseeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int clx_get_syseeprom_size(void)
{
    struct syseeprom_fn_if *syseeprom_dev = get_syseeprom();

    SYSEEPROM_DEV_VALID(syseeprom_dev);
    SYSEEPROM_DEV_VALID(syseeprom_dev->get_syseeprom_size);
    return syseeprom_dev->get_syseeprom_size(syseeprom_dev);
}

/*
 * clx_read_syseeprom_data - Used to read syseeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read syseeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_read_syseeprom_data(char *buf, loff_t offset, size_t count)
{
    struct syseeprom_fn_if *syseeprom_dev = get_syseeprom();

    SYSEEPROM_DEV_VALID(syseeprom_dev);
    SYSEEPROM_DEV_VALID(syseeprom_dev->read_syseeprom_data);
    return syseeprom_dev->read_syseeprom_data(syseeprom_dev, buf, offset, count);
}

/*
 * clx_write_syseeprom_data - Used to write syseeprom data
 * @buf: Data write buffer
 * @offset: offset address to write syseeprom data
 * @count: length of buf
 *
 * This function returns the written length of syseeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_write_syseeprom_data(char *buf, loff_t offset, size_t count)
{
    struct syseeprom_fn_if *syseeprom_dev = get_syseeprom();

    SYSEEPROM_DEV_VALID(syseeprom_dev);
    SYSEEPROM_DEV_VALID(syseeprom_dev->write_syseeprom_data);
    return syseeprom_dev->write_syseeprom_data(syseeprom_dev, buf, offset, count);
}
/*************************************end of syseeprom****************************************/

static struct s3ip_sysfs_syseeprom_drivers_s drivers = {
    /*
     * set ODM syseeprom drivers to /sys/s3ip/syseeprom,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_bsp_version = clx_get_bsp_version_loglevel,
    .get_loglevel = clx_get_syseeprom_loglevel,
    .set_loglevel = clx_set_syseeprom_loglevel,
    .get_debug = clx_get_syseeprom_debug,
    .set_debug = clx_set_syseeprom_debug,
    .get_syseeprom_size = clx_get_syseeprom_size,
    .read_syseeprom_data = clx_read_syseeprom_data,
    .write_syseeprom_data = clx_write_syseeprom_data,
};

static int __init syseeprom_dev_drv_init(void)
{
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_SYSEEPROM, "syseeprom_dev_drv_init...\n");
    ret = syseeprom_if_create_driver();
    if (ret != 0) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "syseeprom if create err, ret %d.\n", ret);
        return ret;
    }

    ret = s3ip_sysfs_syseeprom_drivers_register(&drivers);
    if (ret < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "syseeprom drivers register err, ret %d.\n", ret);
        return ret;
    }
    LOG_INFO(CLX_DRIVER_TYPES_SYSEEPROM, "syseeprom_dev_drv_init success.\n");
    return 0;
}

static void __exit syseeprom_dev_drv_exit(void)
{
    syseeprom_if_delete_driver();
    s3ip_sysfs_syseeprom_drivers_unregister();
    LOG_INFO(CLX_DRIVER_TYPES_SYSEEPROM, "syseeprom_exit success.\n");
    return;
}

module_init(syseeprom_dev_drv_init);
module_exit(syseeprom_dev_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("syseeprom device driver");
