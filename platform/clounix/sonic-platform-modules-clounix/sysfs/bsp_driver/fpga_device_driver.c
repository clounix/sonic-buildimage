/*
 * fpga_device_driver.c
 *
 * This module realize /sys/s3ip/fpga attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "fpga_sysfs.h"
#include "fpga_interface.h"

static int g_loglevel = 0;

/******************************************FPGA***********************************************/

static ssize_t clx_get_main_board_fpga_loglevel(char *buf, size_t count)
{
    return sprintf(buf, "0x%x\n", g_dev_loglevel[CLX_DRIVER_TYPES_CPLD]);
}

static ssize_t clx_set_main_board_fpga_loglevel(char *buf, size_t count)
{
    int loglevel = 0;

    if (kstrtouint(buf, 16, &loglevel))
    {
        return -EINVAL;
    }
    g_dev_loglevel[CLX_DRIVER_TYPES_FPGA] = loglevel;
    return count;
}

static ssize_t clx_get_main_board_fpga_debug(char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t clx_set_main_board_fpga_debug(char *buf, size_t count)
{
    return -ENOSYS;
}
static int clx_get_main_board_fpga_number(void)
{
    int ret;
    struct fpga_fn_if *fpga_dev = get_fpga();

    FPGA_DEV_VALID(fpga_dev);
    FPGA_DEV_VALID(fpga_dev->get_main_board_fpga_number);
    ret = fpga_dev->get_main_board_fpga_number(fpga_dev);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_fpga_alias - Used to identify the location of fpga,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_fpga_alias(unsigned int fpga_index, char *buf, size_t count)
{
    int ret;
    struct fpga_fn_if *fpga_dev = get_fpga();

    FPGA_DEV_VALID(fpga_dev);
    FPGA_DEV_VALID(fpga_dev->get_main_board_fpga_alias);
    ret = fpga_dev->get_main_board_fpga_alias(fpga_dev, fpga_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_fpga_type - Used to get fpga model name
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_fpga_type(unsigned int fpga_index, char *buf, size_t count)
{
    int ret;
    struct fpga_fn_if *fpga_dev = get_fpga();

    FPGA_DEV_VALID(fpga_dev);
    FPGA_DEV_VALID(fpga_dev->get_main_board_fpga_type);
    ret = fpga_dev->get_main_board_fpga_type(fpga_dev, fpga_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_fpga_firmware_version - Used to get fpga firmware version,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_fpga_firmware_version(unsigned int fpga_index, char *buf, size_t count)
{
    int ret;
    struct fpga_fn_if *fpga_dev = get_fpga();

    FPGA_DEV_VALID(fpga_dev);
    FPGA_DEV_VALID(fpga_dev->get_main_board_fpga_firmware_version);
    ret = fpga_dev->get_main_board_fpga_firmware_version(fpga_dev, fpga_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_fpga_board_version - Used to get fpga board version,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_fpga_board_version(unsigned int fpga_index, char *buf, size_t count)
{
    int ret;
    struct fpga_fn_if *fpga_dev = get_fpga();

    FPGA_DEV_VALID(fpga_dev);
    FPGA_DEV_VALID(fpga_dev->get_main_board_fpga_board_version);
    ret = fpga_dev->get_main_board_fpga_board_version(fpga_dev, fpga_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_main_board_fpga_test_reg - Used to test fpga register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_fpga_test_reg(unsigned int fpga_index, char *buf, size_t count)
{
    int ret;
    struct fpga_fn_if *fpga_dev = get_fpga();

    FPGA_DEV_VALID(fpga_dev);
    FPGA_DEV_VALID(fpga_dev->get_main_board_fpga_test_reg);
    ret = fpga_dev->get_main_board_fpga_test_reg(fpga_dev, fpga_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_set_main_board_fpga_test_reg - Used to test fpga register write
 * @fpga_index: start with 1
 * @value: value write to fpga
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_fpga_test_reg(unsigned int fpga_index, unsigned int value)
{
    int ret;
    struct fpga_fn_if *fpga_dev = get_fpga();

    FPGA_DEV_VALID(fpga_dev);
    FPGA_DEV_VALID(fpga_dev->set_main_board_fpga_test_reg);
    ret = fpga_dev->set_main_board_fpga_test_reg(fpga_dev, fpga_index, value);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}
/***************************************end of FPGA*******************************************/

static struct s3ip_sysfs_fpga_drivers_s drivers = {
    /*
     * set ODM FPGA drivers to /sys/s3ip/fpga,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_fpga_number = clx_get_main_board_fpga_number,
    .get_loglevel = clx_get_main_board_fpga_loglevel,
    .set_loglevel = clx_set_main_board_fpga_loglevel,
    .get_debug = clx_get_main_board_fpga_debug,
    .set_debug = clx_set_main_board_fpga_debug,
    .get_main_board_fpga_alias = clx_get_main_board_fpga_alias,
    .get_main_board_fpga_type = clx_get_main_board_fpga_type,
    .get_main_board_fpga_firmware_version = clx_get_main_board_fpga_firmware_version,
    .get_main_board_fpga_board_version = clx_get_main_board_fpga_board_version,
    .get_main_board_fpga_test_reg = clx_get_main_board_fpga_test_reg,
    .set_main_board_fpga_test_reg = clx_set_main_board_fpga_test_reg,
};

static int __init fpga_dev_drv_init(void)
{
    int ret;

    FPGA_INFO("fpga_init...\n");
    fpga_if_create_driver();
    ret = s3ip_sysfs_fpga_drivers_register(&drivers);
    if (ret < 0) {
        FPGA_ERR("fpga drivers register err, ret %d.\n", ret);
        return ret;
    }
    FPGA_INFO("fpga_init success.\n");
    return 0;
}

static void __exit fpga_dev_drv_exit(void)
{
    fpga_if_delete_driver();
    s3ip_sysfs_fpga_drivers_unregister();
    FPGA_INFO("fpga_exit success.\n");
    return;
}

module_init(fpga_dev_drv_init);
module_exit(fpga_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("fpga device driver");
