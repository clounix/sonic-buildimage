/*
 * curr_sensor_device_driver.c
 *
 * This module realize /sys/s3ip/curr_sensor attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "curr_sensor_sysfs.h"
#include "current_interface.h"

/*************************************main board current***************************************/
static int clx_get_main_board_curr_number(void)
{
    struct current_fn_if *current_dev = get_curr();
    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->get_main_board_curr_number);
    return current_dev->get_main_board_curr_number(current_dev);
}

/*
 * clx_get_main_board_curr_alias - Used to identify the location of the current sensor,
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_curr_alias(unsigned int curr_index, char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();
    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->get_main_board_curr_alias);
    return current_dev->get_main_board_curr_alias(current_dev, curr_index, buf, count);
}

/*
 * clx_get_main_board_curr_type - Used to get the model of current sensor,
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_curr_type(unsigned int curr_index, char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->get_main_board_curr_type);
    return current_dev->get_main_board_curr_type(current_dev, curr_index, buf, count);
}

/*
 * clx_get_main_board_curr_max - Used to get the maximum threshold of current sensor
 * filled the value to buf, and the value keep three decimal places
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_curr_max(unsigned int curr_index, char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->get_main_board_curr_max);
    return current_dev->get_main_board_curr_max(current_dev, curr_index, buf, count);
}

/*
 * clx_set_main_board_curr_max - Used to set the maximum threshold of current sensor
 * get value from buf and set it to maximum threshold of current sensor
 * @curr_index: start with 1
 * @buf: the buf store the data to be set
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_curr_max(unsigned int curr_index, const char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->set_main_board_curr_max);
    return current_dev->set_main_board_curr_max(current_dev, curr_index, buf, count);
}

/*
 * clx_get_main_board_curr_min - Used to get the minimum threshold of current sensor
 * filled the value to buf, and the value keep three decimal places
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_curr_min(unsigned int curr_index, char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->get_main_board_curr_min);
    return current_dev->get_main_board_curr_min(current_dev, curr_index, buf, count);
}

/*
 * clx_set_main_board_curr_min - Used to set the minimum threshold of current sensor
 * get value from buf and set it to minimum threshold of current sensor
 * @curr_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_curr_min(unsigned int curr_index, const char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->set_main_board_curr_min);
    return current_dev->set_main_board_curr_min(current_dev, curr_index, buf, count);
}

/*
 * clx_get_main_board_curr_crit - Used to get the crit of current sensor
 * filled the value to buf, and the value keep three decimal places
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_curr_crit(unsigned int curr_index, char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->get_main_board_curr_crit);
    return current_dev->get_main_board_curr_crit(current_dev, curr_index, buf, count);
}

/*
 * clx_set_main_board_curr_crit - Used to set the crit of current sensor
 * get value from buf and set it to crit of current sensor
 * @curr_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_curr_crit(unsigned int curr_index, const char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->set_main_board_curr_crit);
    return current_dev->set_main_board_curr_crit(current_dev, curr_index, buf, count);
}

/*
 * clx_get_main_board_curr_average - Used to get the average of current sensor
 * filled the value to buf, and the value keep three decimal places
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_curr_average(unsigned int curr_index, char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->get_main_board_curr_average);
    return current_dev->get_main_board_curr_average(current_dev, curr_index, buf, count);
}

/*
 * clx_get_main_board_curr_value - Used to get the input value of current sensor
 * filled the value to buf, and the value keep three decimal places
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_curr_value(unsigned int curr_index, char *buf, size_t count)
{
    struct current_fn_if *current_dev = get_curr();

    CURRENT_DEV_VALID(current_dev);
    CURRENT_DEV_VALID(current_dev->get_main_board_curr_value);
    return current_dev->get_main_board_curr_value(current_dev, curr_index, buf, count);
}
/*********************************end of main board current************************************/

static struct s3ip_sysfs_curr_sensor_drivers_s drivers = {
    /*
     * set ODM current sensor drivers to /sys/s3ip/curr_sensor,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_curr_number = clx_get_main_board_curr_number,
    .get_main_board_curr_alias = clx_get_main_board_curr_alias,
    .get_main_board_curr_type = clx_get_main_board_curr_type,
    .get_main_board_curr_max = clx_get_main_board_curr_max,
    .set_main_board_curr_max = clx_set_main_board_curr_max,
    .get_main_board_curr_min = clx_get_main_board_curr_min,
    .set_main_board_curr_min = clx_set_main_board_curr_min,
    .get_main_board_curr_crit = clx_get_main_board_curr_crit,
    .set_main_board_curr_crit = clx_set_main_board_curr_crit,
    .get_main_board_curr_average = clx_get_main_board_curr_average,
    .get_main_board_curr_value = clx_get_main_board_curr_value,
};

static int __init curr_sensor_dev_drv_init(void)
{
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_CURR, "curr_sensor_init...\n");
    ret = current_if_create_driver();
    if (ret != 0) {
        LOG_ERR(CLX_DRIVER_TYPES_CURR, "curr sensor if create err, ret %d.\n", ret);
        return ret;
    }

    ret = s3ip_sysfs_curr_sensor_drivers_register(&drivers);
    if (ret < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_CURR, "curr sensor drivers register err, ret %d.\n", ret);
        return ret;
    }

    LOG_INFO(CLX_DRIVER_TYPES_CURR, "curr_sensor_init success.\n");
    return 0;
}

static void __exit curr_sensor_dev_drv_exit(void)
{
    current_if_delete_driver();
    s3ip_sysfs_curr_sensor_drivers_unregister();
    LOG_INFO(CLX_DRIVER_TYPES_CURR, "curr_sensor_exit success.\n");
    return;
}

module_init(curr_sensor_dev_drv_init);
module_exit(curr_sensor_dev_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("current sensors device driver");
