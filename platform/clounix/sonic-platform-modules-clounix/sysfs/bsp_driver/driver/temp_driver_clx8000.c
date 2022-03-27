#include <linux/io.h>
#include <linux/rwlock.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/string.h>

#include "temp_driver_clx8000.h"
#include "clx_driver.h"

#include "clounix/hwmon_dev_common.h"

//internal function declaration

#define MAX_SENSOR_NUM (5)

#define MAX_CPU_DIE_NUM (1)
#define SENSOR_NUM_PER_DIE (9)
#define CPU_SENSOR_BASE (1)

#define TEMP_NODE "temp1"
#define TEMP_OUT "_input"
#define TEMP_TYPE "_name"
#define TEMP_MAX "_max"
#define TEMP_MAX_HYST "_max_hyst"
#define CPU_TEMP_MAX_HYST "_crit"

DEFINE_RWLOCK(list_lock);

struct sensor_descript sensor_map_index[] = {
    {"cpu package", 0, "cpu package"},
    {"cpu 0", 0, "cpu 0"},
    {"cpu 1", 0, "cpu 1"},
    {"cpu 2", 0, "cpu 2"},
    {"cpu 3", 0, "cpu 3"},
    {"cpu 4", 0, "cpu 4"},
    {"cpu 5", 0, "cpu 5"},
    {"cpu 6", 0, "cpu 6"},
    {"cpu 7", 0, "cpu 7"},
    {"fpga-tmp", 0x48, "FAN 0x48"},
    {"fpga-tmp", 0x49, "FAN 0x49"},
    {"fpga-tmp", 0x4a, "BOARD 0x4a"},
    {"fpga-fan", 0x48, "BOARD 0x48"},
    {"fpga-fan", 0x49, "BOARD 0x49"},
    {NULL, 0, 0},
};

struct temp_driver_clx8000 driver_temp_clx8000;

struct device *sensor_arry[MAX_SENSOR_NUM + SENSOR_NUM_PER_DIE] = {0};

int hwmon_cpu_sensor_add(struct device *dev)
{
    write_lock(&list_lock);
    sensor_arry[0] = dev;
    write_unlock(&list_lock);

    return 0;
}
EXPORT_SYMBOL(hwmon_cpu_sensor_add);

void hwmon_cpu_sensor_del(struct device *dev)
{
    write_lock(&list_lock);
    sensor_arry[0] = NULL;
    write_unlock(&list_lock);

    return;
}
EXPORT_SYMBOL(hwmon_cpu_sensor_del);

int hwmon_sensor_add(struct device *dev)
{
    int i;
    struct i2c_client *client;

    client = to_i2c_client(dev->parent);
    if (client == NULL)
        return -ENODATA;

    i = get_sensor_index(client, sensor_map_index);
    if (i < 0)
        return i;

    write_lock(&list_lock);
    if (sensor_arry[i] == NULL)
        sensor_arry[i] = dev;

    write_unlock(&list_lock);
    return 0;
}
EXPORT_SYMBOL(hwmon_sensor_add);

void hwmon_sensor_del(struct device *dev)
{
    int i;

    write_lock(&list_lock);
    for (i=0; i<MAX_SENSOR_NUM+SENSOR_NUM_PER_DIE; i++) {
        if (sensor_arry[i] == dev)
            sensor_arry[i] = NULL;
    }
    write_unlock(&list_lock);

    return;
}
EXPORT_SYMBOL(hwmon_sensor_del);

static int clx_driver_clx8000_get_main_board_temp_number(void *driver)
{
    /* add vendor codes here */
    return MAX_SENSOR_NUM + SENSOR_NUM_PER_DIE;
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
static ssize_t clx_driver_clx8000_get_main_board_temp_alias(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return sprintf(buf, "%s\n", sensor_map_index[temp_index].alias);
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
static ssize_t clx_driver_clx8000_get_main_board_temp_type(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    int ret;

    /* add vendor codes here */
    if (temp_index < SENSOR_NUM_PER_DIE)
        ret = sprintf(buf, "%s\n", "cpu_sensor");
    else
        ret = sprintf(buf, "%s\n", "tmp75c");
    return ret;
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
static ssize_t clx_driver_clx8000_get_main_board_temp_max(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    int ret;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;
    /* add vendor codes here */

    read_lock(&list_lock);
    if (temp_index < SENSOR_NUM_PER_DIE) {
        sprintf(node_name, "%s%d%s", "temp", temp_index+CPU_SENSOR_BASE, TEMP_MAX);
        dev = sensor_arry[0];
        ret = get_cpu_hwmon_attr_by_name(dev, node_name, buf);
    } else {
        sprintf(node_name, "%s%s", TEMP_NODE, TEMP_MAX);
        dev = sensor_arry[temp_index];
        ret = get_hwmon_attr_by_name(dev, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
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
static int clx_driver_clx8000_set_main_board_temp_max(void *driver, unsigned int temp_index, const char *buf, size_t count)
{
    int ret;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;

    read_lock(&list_lock);
    if (temp_index < SENSOR_NUM_PER_DIE) {
        return count;
    } else {
        sprintf(node_name, "%s%s", TEMP_NODE, TEMP_MAX);
        dev = sensor_arry[temp_index];
        ret = set_hwmon_attr_by_name(dev, node_name, buf, count);
    }

    read_unlock(&list_lock);
    /* add vendor codes here */

    return ret;
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
static ssize_t  clx_driver_clx8000_get_main_board_temp_min(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
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
static int  clx_driver_clx8000_set_main_board_temp_min(void *driver, unsigned int temp_index, const char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
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
static ssize_t  clx_driver_clx8000_get_main_board_temp_value(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    int ret;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;
    /* add vendor codes here */

    read_lock(&list_lock);
    if (temp_index < SENSOR_NUM_PER_DIE) {
        sprintf(node_name, "%s%d%s", "temp", temp_index+CPU_SENSOR_BASE, TEMP_OUT);
        dev = sensor_arry[0];
        ret = get_cpu_hwmon_attr_by_name(dev, node_name, buf);
    } else {
        sprintf(node_name, "%s%s", TEMP_NODE, TEMP_OUT);
        dev = sensor_arry[temp_index];
        ret = get_hwmon_attr_by_name(sensor_arry[temp_index], node_name, buf);
    }
    read_unlock(&list_lock);
    return ret;
}

static int clx_driver_clx8000_set_main_board_temp_max_hyst(void *driver, unsigned int temp_index, const char *buf, size_t count)
{
    int ret;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;

    read_lock(&list_lock);
    if (temp_index < SENSOR_NUM_PER_DIE) {
        return count;
    } else {
        sprintf(node_name, "%s%s", TEMP_NODE, TEMP_MAX_HYST);
        dev = sensor_arry[temp_index];
        ret = set_hwmon_attr_by_name(dev, node_name, buf, count);
    }

    read_unlock(&list_lock);
    /* add vendor codes here */
    return ret;
}

static ssize_t clx_driver_clx8000_get_main_board_temp_max_hyst(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    int ret;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (temp_index < SENSOR_NUM_PER_DIE) {
        sprintf(node_name, "%s%d%s", "temp", temp_index+CPU_SENSOR_BASE, CPU_TEMP_MAX_HYST);
        dev = sensor_arry[0];
        ret = get_cpu_hwmon_attr_by_name(dev, node_name, buf);
    } else {
        sprintf(node_name, "%s%s", TEMP_NODE, TEMP_MAX_HYST);
        dev = sensor_arry[temp_index];
        ret = get_hwmon_attr_by_name(dev, node_name, buf);
    }

    read_unlock(&list_lock);

    return ret;
}

static int clx_driver_clx8000_temp_dev_init(struct temp_driver_clx8000 *temp)
{
    return DRIVER_OK;
}

//void clx_driver_clx8000_temp_init(struct temp_driver_clx8000 **temp_driver)
void clx_driver_clx8000_temp_init(void **temp_driver)
{
    struct temp_driver_clx8000 *temp = &driver_temp_clx8000;

    printk(KERN_ALERT "clx_driver_clx8000_temp_init\n");
    clx_driver_clx8000_temp_dev_init(temp);
    temp->temp_if.get_main_board_temp_number = clx_driver_clx8000_get_main_board_temp_number;
    temp->temp_if.get_main_board_temp_alias = clx_driver_clx8000_get_main_board_temp_alias;
    temp->temp_if.get_main_board_temp_type = clx_driver_clx8000_get_main_board_temp_type;

    temp->temp_if.get_main_board_temp_max = clx_driver_clx8000_get_main_board_temp_max;
    temp->temp_if.set_main_board_temp_max = clx_driver_clx8000_set_main_board_temp_max;

    temp->temp_if.get_main_board_temp_max_hyst = clx_driver_clx8000_get_main_board_temp_max_hyst;
    temp->temp_if.set_main_board_temp_max_hyst = clx_driver_clx8000_set_main_board_temp_max_hyst;

    temp->temp_if.get_main_board_temp_min = clx_driver_clx8000_get_main_board_temp_min;
    temp->temp_if.set_main_board_temp_min = clx_driver_clx8000_set_main_board_temp_min;

    temp->temp_if.get_main_board_temp_value = clx_driver_clx8000_get_main_board_temp_value;
    *temp_driver = temp;
    printk(KERN_INFO "TEMP driver clx8000 initialization done.\r\n");
}
//clx_driver_define_initcall(clx_driver_clx8000_temp_init);
