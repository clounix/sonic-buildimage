/*
 * temp_sensor_device_driver.c
 *
 * This module realize /sys/s3ip/temp_sensor attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "temp_sensor_sysfs.h"
#include "temp_interface.h"

/***************************************main board temp*****************************************/
static ssize_t clx_get_main_board_temp_loglevel(char *buf, size_t count)
{
    PRINT_LOGLEVEL(g_dev_loglevel[CLX_DRIVER_TYPES_TEMP], buf, count);
}

static ssize_t clx_set_main_board_temp_loglevel(const char *buf, size_t count)
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
    g_dev_loglevel[CLX_DRIVER_TYPES_TEMP] = loglevel;
    return count;
}

static ssize_t clx_get_main_board_temp_debug(char *buf, size_t count)
{
    return sprintf(buf, "read sensor info: \n"
                        "cat /sys/switch/sensor/temp*/*\n"
                        "cat /sys/switch/sensor/in*/*\n"
                        "cat /sys/switch/sensor/curr*/*\n");
}

static ssize_t clx_set_main_board_temp_debug(const char *buf, size_t count)
{
    return -ENOSYS;
}
/*
 * clx_get_main_board_temp_number - Used to get main board temperature sensors number,
 *
 * This function returns main board temperature sensors by your switch,
 * If there is no main board temperature sensors, returns 0,
 * otherwise it returns a negative value on failed.
 */
static int clx_get_main_board_temp_number(void)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->get_main_board_temp_number);
    return temp_dev->get_main_board_temp_number(temp_dev);
}

/*
 * clx_get_main_board_temp_alias - Used to identify the location of the temperature sensor,
 * such as air_inlet, air_outlet and so on.
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_temp_alias(unsigned int temp_index, char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->get_main_board_temp_alias);
    return temp_dev->get_main_board_temp_alias(temp_dev, temp_index, buf, count);
}

/*
 * clx_get_main_board_temp_type - Used to get the model of temperature sensor,
 * such as lm75, tmp411 and so on
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_temp_type(unsigned int temp_index, char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->get_main_board_temp_type);
    return temp_dev->get_main_board_temp_type(temp_dev, temp_index, buf, count);
}

/*
 * clx_get_main_board_temp_max - Used to get the maximum threshold of temperature sensor
 * filled the value to buf, and the value keep three decimal places
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_temp_max(unsigned int temp_index, char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->get_main_board_temp_max);
    return temp_dev->get_main_board_temp_max(temp_dev, temp_index, buf, count);
}

/*
 * clx_set_main_board_temp_max - Used to set the maximum threshold of temperature sensor
 * get value from buf and set it to maximum threshold of temperature sensor
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '80.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_temp_max(unsigned int temp_index, const char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->set_main_board_temp_max);
    return temp_dev->set_main_board_temp_max(temp_dev, temp_index, buf, count);
}

/*
 * clx_get_main_board_temp_max_hyst - Used to get the maximum hyst of temperature sensor
 * filled the value to buf, and the value keep three decimal places
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_temp_max_hyst(unsigned int temp_index, char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->get_main_board_temp_max_hyst);
    return temp_dev->get_main_board_temp_max_hyst(temp_dev, temp_index, buf, count);
}

/*
 * clx_set_main_board_temp_max_hyst - Used to set the maximum hyst of temperature sensor
 * get value from buf and set it to maximum threshold of temperature sensor
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '80.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_temp_max_hyst(unsigned int temp_index, const char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->set_main_board_temp_max_hyst);
    return temp_dev->set_main_board_temp_max_hyst(temp_dev, temp_index, buf, count);
}

/*
 * clx_get_main_board_temp_min - Used to get the minimum threshold of temperature sensor
 * filled the value to buf, and the value keep three decimal places
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_temp_min(unsigned int temp_index, char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->get_main_board_temp_min);
    return temp_dev->get_main_board_temp_min(temp_dev, temp_index, buf, count);
}

/*
 * clx_set_main_board_temp_min - Used to set the minimum threshold of temperature sensor
 * get value from buf and set it to minimum threshold of temperature sensor
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_main_board_temp_min(unsigned int temp_index, const char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->set_main_board_temp_min);
    return temp_dev->set_main_board_temp_min(temp_dev, temp_index, buf, count);
}

/*
 * clx_get_main_board_temp_value - Used to get the input value of temperature sensor
 * filled the value to buf, and the value keep three decimal places
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_main_board_temp_value(unsigned int temp_index, char *buf, size_t count)
{
    struct temp_fn_if *temp_dev = get_temp();

    TEMP_DEV_VALID(temp_dev);
    TEMP_DEV_VALID(temp_dev->get_main_board_temp_value);
    return temp_dev->get_main_board_temp_value(temp_dev, temp_index, buf, count);
}
/***********************************end of main board temp*************************************/

static struct s3ip_sysfs_temp_sensor_drivers_s drivers = {
    /*
     * set ODM temperature sensor drivers to /sys/s3ip/temp_sensor,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_loglevel = clx_get_main_board_temp_loglevel,
    .set_loglevel = clx_set_main_board_temp_loglevel,
    .get_debug = clx_get_main_board_temp_debug,
    .set_debug = clx_set_main_board_temp_debug,    
    .get_main_board_temp_number = clx_get_main_board_temp_number,
    .get_main_board_temp_alias = clx_get_main_board_temp_alias,
    .get_main_board_temp_type = clx_get_main_board_temp_type,

    .get_main_board_temp_max = clx_get_main_board_temp_max,
    .set_main_board_temp_max = clx_set_main_board_temp_max,
    
    .get_main_board_temp_max_hyst = clx_get_main_board_temp_max_hyst,
    .set_main_board_temp_max_hyst = clx_set_main_board_temp_max_hyst,
    
    .get_main_board_temp_min = clx_get_main_board_temp_min,
    .set_main_board_temp_min = clx_set_main_board_temp_min,
    
    .get_main_board_temp_value = clx_get_main_board_temp_value,
};

static int __init temp_sensor_dev_drv_init(void)
{
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_TEMP, "temp_sensor_init...\n");
    ret = temp_if_create_driver();
    if (ret != 0) {
        LOG_ERR(CLX_DRIVER_TYPES_TEMP, "temp sensor if create err, ret %d.\n", ret);
        return ret;
    }

    ret = s3ip_sysfs_temp_sensor_drivers_register(&drivers);
    if (ret < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_TEMP, "temp sensor drivers register err, ret %d.\n", ret);
        return ret;
    }
    LOG_INFO(CLX_DRIVER_TYPES_TEMP, "temp_sensor_init success.\n");
    return 0;
}

static void __exit temp_sensor_dev_drv_exit(void)
{
    temp_if_delete_driver();
    s3ip_sysfs_temp_sensor_drivers_unregister();
    LOG_INFO(CLX_DRIVER_TYPES_TEMP, "temp_sensor_exit success.\n");
    return;
}

module_init(temp_sensor_dev_drv_init);
module_exit(temp_sensor_dev_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("temperature sensors device driver");
