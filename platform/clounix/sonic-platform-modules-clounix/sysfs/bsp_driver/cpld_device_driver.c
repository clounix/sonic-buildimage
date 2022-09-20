/*
 * cpld_device_driver.c
 *
 * This module realize /sys/s3ip/cpld attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "cpld_sysfs.h"
#include "cpld_interface.h"

static int g_loglevel = 0;

/******************************************CPLD***********************************************/

static ssize_t clx_get_main_board_cpld_loglevel(char *buf, size_t count)
{
    return sprintf(buf, "0x%x\n", g_dev_loglevel[CLX_DRIVER_TYPES_CPLD]);
}

static ssize_t clx_set_main_board_cpld_loglevel(char *buf, size_t count)
{
    int loglevel = 0;

    if (kstrtouint(buf, 16, &loglevel))
    {
        return -EINVAL;
    }
    g_dev_loglevel[CLX_DRIVER_TYPES_CPLD] = loglevel;
    return count;
}

static ssize_t clx_get_main_board_cpld_debug(char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t clx_set_main_board_cpld_debug(char *buf, size_t count)
{
    return -ENOSYS;
}

static int clx_get_main_board_cpld_number(void)
{
    struct cpld_fn_if *cpld_dev = get_cpld();

    CPLD_DEV_VALID(cpld_dev);
    CPLD_DEV_VALID(cpld_dev->get_main_board_cpld_number);
    return cpld_dev->get_main_board_cpld_number(cpld_dev);
}

/*
 * clx_get_main_board_cpld_alias - Used to identify the location of cpld,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_cpld_alias(unsigned int cpld_index, char *buf, size_t count)
{
    int ret;
    struct cpld_fn_if *cpld_dev = get_cpld();

    CPLD_DEV_VALID(cpld_dev);
    CPLD_DEV_VALID(cpld_dev->get_main_board_cpld_alias);
    ret = cpld_dev->get_main_board_cpld_alias(cpld_dev, cpld_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_cpld_type - Used to get cpld model name
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_cpld_type(unsigned int cpld_index, char *buf, size_t count)
{
    int ret;
    struct cpld_fn_if *cpld_dev = get_cpld();

    CPLD_DEV_VALID(cpld_dev);
    CPLD_DEV_VALID(cpld_dev->get_main_board_cpld_type);
    ret = cpld_dev->get_main_board_cpld_type(cpld_dev, cpld_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_cpld_firmware_version - Used to get cpld firmware version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_cpld_firmware_version(unsigned int cpld_index, char *buf, size_t count)
{
    int ret;
    struct cpld_fn_if *cpld_dev = get_cpld();

    CPLD_DEV_VALID(cpld_dev);
    CPLD_DEV_VALID(cpld_dev->get_main_board_cpld_firmware_version);
    ret = cpld_dev->get_main_board_cpld_firmware_version(cpld_dev, cpld_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_cpld_board_version - Used to get cpld board version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_cpld_board_version(unsigned int cpld_index, char *buf, size_t count)
{
    int ret;
    struct cpld_fn_if *cpld_dev = get_cpld();

    CPLD_DEV_VALID(cpld_dev);
    CPLD_DEV_VALID(cpld_dev->get_main_board_cpld_board_version);
    ret = cpld_dev->get_main_board_cpld_board_version(cpld_dev, cpld_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_cpld_test_reg - Used to test cpld register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_cpld_test_reg(unsigned int cpld_index, char *buf, size_t count)
{
    int ret;
    struct cpld_fn_if *cpld_dev = get_cpld();

    CPLD_DEV_VALID(cpld_dev);
    CPLD_DEV_VALID(cpld_dev->get_main_board_cpld_test_reg);
    ret = cpld_dev->get_main_board_cpld_test_reg(cpld_dev, cpld_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_set_main_board_cpld_test_reg - Used to test cpld register write
 * @cpld_index: start with 1
 * @value: value write to cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_cpld_test_reg(unsigned int cpld_index, unsigned int value)
{
    int ret;
    struct cpld_fn_if *cpld_dev = get_cpld();

    CPLD_DEV_VALID(cpld_dev);
    CPLD_DEV_VALID(cpld_dev->set_main_board_cpld_test_reg);
    ret = cpld_dev->set_main_board_cpld_test_reg(cpld_dev, cpld_index, value);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}
/***************************************end of CPLD*******************************************/

static struct s3ip_sysfs_cpld_drivers_s drivers = {
    /*
     * set ODM CPLD drivers to /sys/s3ip/cpld,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_cpld_number = clx_get_main_board_cpld_number,
    .get_loglevel = clx_get_main_board_cpld_loglevel,
    .set_loglevel = clx_set_main_board_cpld_loglevel,
    .get_debug = clx_get_main_board_cpld_debug,
    .set_debug = clx_set_main_board_cpld_debug,
    .get_main_board_cpld_alias = clx_get_main_board_cpld_alias,
    .get_main_board_cpld_type = clx_get_main_board_cpld_type,
    .get_main_board_cpld_firmware_version = clx_get_main_board_cpld_firmware_version,
    .get_main_board_cpld_board_version = clx_get_main_board_cpld_board_version,
    .get_main_board_cpld_test_reg = clx_get_main_board_cpld_test_reg,
    .set_main_board_cpld_test_reg = clx_set_main_board_cpld_test_reg,
};

static int __init cpld_device_driver_init(void)
{
    int ret;

    CPLD_INFO("cpld_init...\n");
    cpld_if_create_driver();

    ret = s3ip_sysfs_cpld_drivers_register(&drivers);
    if (ret < 0) {
        CPLD_ERR("cpld drivers register err, ret %d.\n", ret);
        return ret;
    }

    CPLD_INFO("cpld_init success.\n");
    return 0;
}

static void __exit cpld_device_driver_exit(void)
{
    s3ip_sysfs_cpld_drivers_unregister();
    cpld_if_delete_driver();
    CPLD_INFO("cpld_exit success.\n");
    return;
}

module_init(cpld_device_driver_init);
module_exit(cpld_device_driver_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("cpld device driver");
