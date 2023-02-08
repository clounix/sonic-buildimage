/*
 * fan_device_driver.c
 *
 * This module realize /sys/s3ip/fan attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "fan_sysfs.h"
#include "fan_interface.h"

/********************************************fan**********************************************/
static ssize_t clx_get_fan_loglevel(char *buf, size_t count)
{
    PRINT_LOGLEVEL(g_dev_loglevel[CLX_DRIVER_TYPES_FAN], buf, count);
}

static ssize_t clx_set_fan_loglevel(const char *buf, size_t count)
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
    g_dev_loglevel[CLX_DRIVER_TYPES_FAN] = loglevel;
    return count;
}

static ssize_t clx_get_fan_debug(char *buf, size_t count)
{
    return sprintf(buf, "fan speed adjust: \n"
                        "echo <ratio> > /sys/switch/fan/fan1/motor0/ratio\n"
                        "check fan speed: \n"
                        "cat /sys/switch/fan/fan1/motor0/speed\n");
}

static ssize_t clx_set_fan_debug(const char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t clx_get_fan_eeprom_wp(char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_eeprom_wp);
    return fan_dev->get_fan_eeprom_wp(fan_dev, buf, count);
}

static int clx_set_fan_eeprom_wp(unsigned int enable)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->set_fan_eeprom_wp);
    return fan_dev->set_fan_eeprom_wp(fan_dev, enable);
}

static int clx_get_fan_number(void)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_number);
    return fan_dev->get_fan_number(fan_dev);
}


static int clx_get_fan_motor_number(unsigned int fan_index)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_motor_number);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_motor_number(fan_dev, fan_index);
}

/*
 * clx_get_fan_vendor_name - Used to get fan vendor name,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_vendor_name(unsigned int fan_index, char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_model_name);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_model_name(fan_dev, fan_index, buf, count);
}

/*
 * clx_get_fan_model_name - Used to get fan model name,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_model_name(unsigned int fan_index, char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_model_name);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_model_name(fan_dev, fan_index, buf, count);
}

/*
 * clx_get_fan_serial_number - Used to get fan serial number,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_serial_number(unsigned int fan_index, char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_serial_number);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_serial_number(fan_dev, fan_index, buf, count);
}

/*
 * clx_get_fan_part_number - Used to get fan part number,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_part_number(unsigned int fan_index, char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_part_number);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_part_number(fan_dev, fan_index, buf, count);
}

/*
 * clx_get_fan_hardware_version - Used to get fan hardware version,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_hardware_version(unsigned int fan_index, char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_hardware_version);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_hardware_version(fan_dev, fan_index, buf, count);
}

/*
 * clx_get_fan_status - Used to get fan status,
 * filled the value to buf, fan status define as below:
 * 0: ABSENT
 * 1: OK
 * 2: NOT OK
 *
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_status(unsigned int fan_index, char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_status);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_status(fan_dev, fan_index, buf, count);
}

/*
 * clx_get_fan_led_status - Used to get fan led status
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
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_led_status(unsigned int fan_index, char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_led_status);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_led_status(fan_dev, fan_index, buf, count);
}

/*
 * clx_set_fan_led_status - Used to set fan led status
 * @fan_index: start with 1
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
static int clx_set_fan_led_status(unsigned int fan_index, int status)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->set_fan_led_status);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->set_fan_led_status(fan_dev, fan_index, status);
}

/*
 * clx_get_fan_direction - Used to get fan air flow direction,
 * filled the value to buf, air flow direction define as below:
 * 0: F2B
 * 1: B2F
 *
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_direction(unsigned int fan_index, char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_direction);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_direction(fan_dev, fan_index, buf, count);
}

/*
 * clx_get_fan_motor_speed - Used to get fan motor speed
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_motor_speed(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_motor_speed);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_motor_speed(fan_dev,fan_index, motor_index, buf, count);
}

/*
 * clx_get_fan_motor_speed_tolerance - Used to get fan motor speed tolerance
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_motor_speed_tolerance(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_motor_speed_tolerance);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_motor_speed_tolerance(fan_dev,fan_index, motor_index, buf, count);
}

/*
 * clx_get_fan_motor_speed_target - Used to get fan motor speed target
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_motor_speed_target(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_motor_speed_target);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_motor_speed_target(fan_dev,fan_index, motor_index, buf, count);
}

/*
 * clx_get_fan_motor_speed_max - Used to get the maximum threshold of fan motor
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_motor_speed_max(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_motor_speed_max);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_motor_speed_max(fan_dev, fan_index, motor_index, buf, count);
}

/*
 * clx_get_fan_motor_speed_min - Used to get the minimum threshold of fan motor
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_motor_speed_min(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_motor_speed_min);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_motor_speed_min(fan_dev, fan_index, motor_index, buf, count);
}

/*
 * clx_get_fan_motor_ratio - Used to get the ratio of fan motor
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_fan_motor_ratio(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_motor_ratio);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->get_fan_motor_ratio(fan_dev, fan_index, motor_index, buf, count);
}

/*
 * clx_set_fan_motor_ratio - Used to set the ratio of fan motor
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @ratio: motor speed ratio, from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_fan_motor_ratio(unsigned int fan_index, unsigned int motor_index,
                   int ratio)
{
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->set_fan_motor_ratio);
    FAN_INDEX_MAPPING(fan_index);
    return fan_dev->set_fan_motor_ratio(fan_dev, fan_index, motor_index, ratio);
}

/*
 * clx_get_fan_eeprom_size - Used to get port eeprom size
 *
 * This function returns the size of fan eeprom,
 * otherwise it returns a negative value on failed.
 */
static int clx_get_fan_eeprom_size(unsigned int fan_index)
{
    int ret;
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->get_fan_eeprom_size);
    FAN_INDEX_MAPPING(fan_index);
    ret = fan_dev->get_fan_eeprom_size(fan_dev, fan_index);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_read_fan_eeprom_data - Used to read fan eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read fan eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_read_fan_eeprom_data(unsigned int fan_index, char *buf, loff_t offset,
                   size_t count)
{
    int ret;
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->read_fan_eeprom_data);
    FAN_INDEX_MAPPING(fan_index);
    ret = fan_dev->read_fan_eeprom_data(fan_dev, fan_index, buf, offset, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_write_fan_eeprom_data - Used to write fan eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write fan eeprom data
 * @count: length of buf
 *
 * This function returns the written length of port eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_write_fan_eeprom_data(unsigned int fan_index, char *buf, loff_t offset,
                   size_t count)
{
    int ret;
    struct fan_fn_if *fan_dev = get_fan();

    FAN_DEV_VALID(fan_dev);
    FAN_DEV_VALID(fan_dev->write_fan_eeprom_data);
    FAN_INDEX_MAPPING(fan_index);
    ret = fan_dev->write_fan_eeprom_data(fan_dev, fan_index, buf, offset, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}
/****************************************end of fan*******************************************/

static struct s3ip_sysfs_fan_drivers_s drivers = {
    /*
     * set ODM fan drivers to /sys/s3ip/fan,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_loglevel = clx_get_fan_loglevel,
    .set_loglevel = clx_set_fan_loglevel,
    .get_debug = clx_get_fan_debug,
    .set_debug = clx_set_fan_debug,
    .get_fan_eeprom_wp = clx_get_fan_eeprom_wp,
    .set_fan_eeprom_wp = clx_set_fan_eeprom_wp,    
    .get_fan_number = clx_get_fan_number,
    .get_fan_motor_number = clx_get_fan_motor_number,
    .get_fan_vendor_name = clx_get_fan_vendor_name,
    .get_fan_model_name = clx_get_fan_model_name,
    .get_fan_serial_number = clx_get_fan_serial_number,
    .get_fan_part_number = clx_get_fan_part_number,
    .get_fan_hardware_version = clx_get_fan_hardware_version,
    .get_fan_status = clx_get_fan_status,
    .get_fan_led_status = clx_get_fan_led_status,
    .set_fan_led_status = clx_set_fan_led_status,
    .get_fan_direction = clx_get_fan_direction,
    .get_fan_motor_speed = clx_get_fan_motor_speed,
    .get_fan_motor_speed_tolerance = clx_get_fan_motor_speed_tolerance,
    .get_fan_motor_speed_target = clx_get_fan_motor_speed_target,
    .get_fan_motor_speed_max = clx_get_fan_motor_speed_max,
    .get_fan_motor_speed_min = clx_get_fan_motor_speed_min,
    .get_fan_motor_ratio = clx_get_fan_motor_ratio,
    .set_fan_motor_ratio = clx_set_fan_motor_ratio,
    .get_fan_eeprom_size = clx_get_fan_eeprom_size,
    .read_fan_eeprom_data = clx_read_fan_eeprom_data,
    .write_fan_eeprom_data = clx_write_fan_eeprom_data,
};

static int __init fan_dev_drv_init(void)
{
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_FAN, "fan_init...\n");
    ret = fan_if_create_driver();
    if (ret != 0) {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "fan if create err, ret %d.\n", ret);
        return ret;
    }

    ret = s3ip_sysfs_fan_drivers_register(&drivers);
    if (ret < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "fan drivers register err, ret %d.\n", ret);
        return ret;
    }

    LOG_INFO(CLX_DRIVER_TYPES_FAN, "fan_init success.\n");
    return 0;
}

static void __exit fan_dev_drv_exit(void)
{
    fan_if_delete_driver();
    s3ip_sysfs_fan_drivers_unregister();
    LOG_INFO(CLX_DRIVER_TYPES_FAN, "fan_exit success.\n");
    return;
}

module_init(fan_dev_drv_init);
module_exit(fan_dev_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("fan device driver");
