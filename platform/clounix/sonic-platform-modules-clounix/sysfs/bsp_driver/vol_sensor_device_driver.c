/*
 * vol_sensor_device_driver.c
 *
 * This module realize /sys/s3ip/vol_sensor attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "vol_sensor_sysfs.h"
#include "voltage_interface.h"

static int g_loglevel = 0;

/*************************************main board voltage***************************************/
static int clx_get_main_board_vol_number(void)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_number);
    return voltage_dev->get_main_board_vol_number(voltage_dev);
}

/*
 * clx_get_main_board_vol_alias - Used to identify the location of the voltage sensor,
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_vol_alias(unsigned int vol_index, char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_alias);
    return voltage_dev->get_main_board_vol_alias(voltage_dev, vol_index, buf, count);
}

/*
 * clx_get_main_board_vol_type - Used to get the model of voltage sensor,
 * such as udc90160, tps53622 and so on
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_vol_type(unsigned int vol_index, char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_type);
    return voltage_dev->get_main_board_vol_type(voltage_dev, vol_index, buf, count);
}

/*
 * clx_get_main_board_vol_max - Used to get the maximum threshold of voltage sensor
 * filled the value to buf, and the value keep three decimal places
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_vol_max(unsigned int vol_index, char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_max);
    return voltage_dev->get_main_board_vol_max(voltage_dev, vol_index, buf, count);
}

/*
 * clx_set_main_board_vol_max - Used to set the maximum threshold of volatge sensor
 * get value from buf and set it to maximum threshold of volatge sensor
 * @vol_index: start with 1
 * @buf: the buf store the data to be set, eg '3.567'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_vol_max(unsigned int vol_index, const char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->set_main_board_vol_max);
    return voltage_dev->set_main_board_vol_max(voltage_dev, vol_index, buf, count);
}

/*
 * clx_get_main_board_vol_min - Used to get the minimum threshold of voltage sensor
 * filled the value to buf, and the value keep three decimal places
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_vol_min(unsigned int vol_index, char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_min);
    return voltage_dev->get_main_board_vol_min(voltage_dev, vol_index, buf, count);
}

/*
 * clx_set_main_board_vol_min - Used to set the minimum threshold of voltage sensor
 * get value from buf and set it to minimum threshold of voltage sensor
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '3.123'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_vol_min(unsigned int vol_index, const char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->set_main_board_vol_min);
    return voltage_dev->set_main_board_vol_min(voltage_dev, vol_index, buf, count);
}

/*
 * clx_get_main_board_vol_crit - Used to get the crit of voltage sensor
 * filled the value to buf, and the value keep three decimal places
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_vol_crit(unsigned int vol_index, char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_crit);
    return voltage_dev->get_main_board_vol_crit(voltage_dev, vol_index, buf, count);
}

/*
 * clx_set_main_board_vol_crit - Used to set the crit of voltage sensor
 * get value from buf and set it to crit of voltage sensor
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '3.123'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_vol_crit(unsigned int vol_index, const char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->set_main_board_vol_crit);
    return voltage_dev->set_main_board_vol_crit(voltage_dev, vol_index, buf, count);
}

/*
 * clx_get_main_board_vol_range - Used to get the output error value of voltage sensor
 * filled the value to buf
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_vol_range(unsigned int vol_index, char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_range);
    return voltage_dev->get_main_board_vol_range(voltage_dev, vol_index, buf, count);
}

/*
 * clx_get_main_board_vol_nominal_value - Used to get the nominal value of voltage sensor
 * filled the value to buf
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_vol_nominal_value(unsigned int vol_index, char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_nominal_value);
    return voltage_dev->get_main_board_vol_nominal_value(voltage_dev, vol_index, buf, count);
}

/*
 * clx_get_main_board_vol_value - Used to get the input value of voltage sensor
 * filled the value to buf, and the value keep three decimal places
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_vol_value(unsigned int vol_index, char *buf, size_t count)
{
    struct voltage_fn_if *voltage_dev = get_voltage();

    VOLTAGE_DEV_VALID(voltage_dev);
    VOLTAGE_DEV_VALID(voltage_dev->get_main_board_vol_value);
    return voltage_dev->get_main_board_vol_value(voltage_dev, vol_index, buf, count);
}
/*********************************end of main board voltage************************************/

static struct s3ip_sysfs_vol_sensor_drivers_s drivers = {
    /*
     * set ODM voltage sensor drivers to /sys/s3ip/vol_sensor,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_main_board_vol_number = clx_get_main_board_vol_number,
    .get_main_board_vol_alias = clx_get_main_board_vol_alias,
    .get_main_board_vol_type = clx_get_main_board_vol_type,
    .get_main_board_vol_max = clx_get_main_board_vol_max,
    .set_main_board_vol_max = clx_set_main_board_vol_max,
    .get_main_board_vol_min = clx_get_main_board_vol_min,
    .set_main_board_vol_min = clx_set_main_board_vol_min,
    .get_main_board_vol_crit = clx_get_main_board_vol_crit,
    .set_main_board_vol_crit = clx_set_main_board_vol_crit,
    .get_main_board_vol_range = clx_get_main_board_vol_range,
    .get_main_board_vol_nominal_value = clx_get_main_board_vol_nominal_value,
    .get_main_board_vol_value = clx_get_main_board_vol_value,
};

static int __init vol_sensor_dev_drv_init(void)
{
    int ret;

    VOL_SENSOR_INFO("vol_sensor_init...\n");
    voltage_if_create_driver();

    ret = s3ip_sysfs_vol_sensor_drivers_register(&drivers);
    if (ret < 0) {
        VOL_SENSOR_ERR("vol sensor drivers register err, ret %d.\n", ret);
        return ret;
    }
    VOL_SENSOR_INFO("vol_sensor_init success.\n");
    return 0;
}

static void __exit vol_sensor_dev_drv_exit(void)
{
    voltage_if_delete_driver();
    s3ip_sysfs_vol_sensor_drivers_unregister();
    VOL_SENSOR_INFO("vol_sensor_exit success.\n");
    return;
}

module_init(vol_sensor_dev_drv_init);
module_exit(vol_sensor_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("voltage sensors device driver");
