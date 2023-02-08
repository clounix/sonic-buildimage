#include <linux/io.h>
#include <linux/rwlock.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/string.h>

#include "drv_temp_sensor_clx.h"
#include "clx_driver.h"

#include "clounix/hwmon_dev_common.h"

//internal function declaration

#define MAX_SENSOR_NUM (4)
#define SENSOR_NUM_PER_DIE (5)

#define MAX_CPU_DIE_NUM (1)
#define CPU_SENSOR_BASE (1)

#define DEFAULT_CPU_TEMP_MAX (86*1000)

#define TEMP_NODE "temp1"
#define CPU_TEMP_NODE "temp"
#define TEMP_OUT "_input"
#define TEMP_LABEL "_label"
#define TEMP_MAX "_max"
#define TEMP_MAX_HYST "_max_hyst"

#define CPU_TEMP_MAX "_crit"
#define CPU_TEMP_MAX_HYST "_max"

static DEFINE_RWLOCK(list_lock);

struct sensor_descript *sensor_map_index;

struct drv_temp_sensor_clx driver_temp_clx;

struct device *sensor_arry[MAX_SENSOR_NUM + SENSOR_NUM_PER_DIE] = {0};
static struct s3ip_cpu_temp_data cpu_temp_data_list[MAX_CORE_DATA] = {0};

int s3ip_cpu_temp_sensor_add(struct device *dev, struct attribute **attrs, int auto_inc)
{
    int i;
    int ret = -ENOMEM;

    write_lock(&list_lock);
    for (i=0; i< MAX_CORE_DATA; i++) {
        if (cpu_temp_data_list[i].dev == 0) {
            LOG_DBG(CLX_DRIVER_TYPES_TEMP, "cpu temp add %s\n", dev->init_name);
            cpu_temp_data_list[i].dev = dev;
            cpu_temp_data_list[i].attrs = attrs;
            cpu_temp_data_list[i].auto_inc = auto_inc;
            ret = 0;
            break;
        }
    }
    write_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(s3ip_cpu_temp_sensor_add);

void s3ip_cpu_temp_sensor_del(struct device *dev, struct attribute **attrs)
{
    int i;
    write_lock(&list_lock);
    for (i=0; i< MAX_CORE_DATA; i++) {
        if (cpu_temp_data_list[i].dev != dev)
            continue;

        if (cpu_temp_data_list[i].attrs != attrs)
            continue;

        LOG_DBG(CLX_DRIVER_TYPES_TEMP, "cpu temp del %s\n", dev->init_name);
        cpu_temp_data_list[i].dev = NULL;
        cpu_temp_data_list[i].attrs = NULL;
        break;
    }
    write_unlock(&list_lock);

    return;
}
EXPORT_SYMBOL(s3ip_cpu_temp_sensor_del);

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
    if (sensor_arry[i] == NULL) {
        LOG_DBG(CLX_DRIVER_TYPES_TEMP, "temp add %s\n", dev->init_name);
        sensor_arry[i] = dev;
    }
    write_unlock(&list_lock);
    return 0;
}
EXPORT_SYMBOL(hwmon_sensor_add);

void hwmon_sensor_del(struct device *dev)
{
    int i;

    write_lock(&list_lock);
    for (i=0; i<MAX_SENSOR_NUM+SENSOR_NUM_PER_DIE; i++) {
        if (sensor_arry[i] == dev) {
            LOG_DBG(CLX_DRIVER_TYPES_TEMP, "temp del %s\n", dev->init_name);
            sensor_arry[i] = NULL;
        }
    }
    write_unlock(&list_lock);

    return;
}
EXPORT_SYMBOL(hwmon_sensor_del);

static int drv_sensor_get_main_board_temp_number(void *driver)
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
static ssize_t drv_sensor_get_main_board_temp_alias(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    struct s3ip_cpu_temp_data *data;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    int ret;

    /* add vendor codes here */
    if (temp_index >= MAX_SENSOR_NUM) {
        read_lock(&list_lock);
        data = &cpu_temp_data_list[temp_index -MAX_SENSOR_NUM];
        if (data->dev != NULL) {
            if (data->auto_inc != 0)
                sprintf(node_name, "%s%d%s", CPU_TEMP_NODE, temp_index-MAX_SENSOR_NUM+1, TEMP_LABEL);
            else
                sprintf(node_name, "%s1%s", CPU_TEMP_NODE, TEMP_LABEL);

            ret = get_cpu_hwmon_attr_by_name(data, node_name, buf);
        } else {
            ret = -ENXIO;
        }
        read_unlock(&list_lock);

        return ret;
    } else {
        return sprintf(buf, "%s\n", sensor_map_index[temp_index].alias);
    }
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
static ssize_t drv_sensor_get_main_board_temp_type(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    int ret;

    /* add vendor codes here */
    if (temp_index >= MAX_SENSOR_NUM)
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
static ssize_t drv_sensor_get_main_board_temp_max(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    struct s3ip_cpu_temp_data *data;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;
    int ret;
    /* add vendor codes here */

    read_lock(&list_lock);
    if (temp_index >= MAX_SENSOR_NUM) {
        data = &cpu_temp_data_list[temp_index - MAX_SENSOR_NUM];
        if (data->dev != NULL) {
            if (data->auto_inc != 0)
                sprintf(node_name, "%s%d%s", CPU_TEMP_NODE, temp_index-MAX_SENSOR_NUM+1, CPU_TEMP_MAX);
            else
                sprintf(node_name, "%s1%s", CPU_TEMP_NODE, CPU_TEMP_MAX);

            ret = get_cpu_hwmon_attr_by_name(data, node_name, buf);
            if (ret < 0) {
               ret = sprintf(buf, "%d\n", DEFAULT_CPU_TEMP_MAX);
            }
        } else {
            ret = -ENXIO;
        }
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
static int drv_sensor_set_main_board_temp_max(void *driver, unsigned int temp_index, const char *buf, size_t count)
{
    int ret;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;

    read_lock(&list_lock);
    if (temp_index >= MAX_SENSOR_NUM) {
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
static ssize_t  drv_sensor_get_main_board_temp_min(void *driver, unsigned int temp_index, char *buf, size_t count)
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
static int  drv_sensor_set_main_board_temp_min(void *driver, unsigned int temp_index, const char *buf, size_t count)
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
static ssize_t  drv_sensor_get_main_board_temp_value(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    struct s3ip_cpu_temp_data *data;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;
    int ret;
    /* add vendor codes here */

    read_lock(&list_lock);
    if (temp_index >= MAX_SENSOR_NUM) {
        data = &cpu_temp_data_list[temp_index - MAX_SENSOR_NUM];
        if (data->dev != NULL) {
            if (data->auto_inc != 0)
                sprintf(node_name, "%s%d%s", CPU_TEMP_NODE, temp_index-MAX_SENSOR_NUM+1, TEMP_OUT);
            else
                sprintf(node_name, "%s1%s", CPU_TEMP_NODE, TEMP_OUT);

            ret = get_cpu_hwmon_attr_by_name(data, node_name, buf);
        } else {
            ret = -ENXIO;
        }
    } else {
        sprintf(node_name, "%s%s", TEMP_NODE, TEMP_OUT);
        dev = sensor_arry[temp_index];
        ret = get_hwmon_attr_by_name(sensor_arry[temp_index], node_name, buf);
    }
    read_unlock(&list_lock);
    return ret;
}

static int drv_sensor_set_main_board_temp_max_hyst(void *driver, unsigned int temp_index, const char *buf, size_t count)
{
    int ret;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;

    read_lock(&list_lock);
    if (temp_index >= MAX_SENSOR_NUM) {
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

static ssize_t drv_sensor_get_main_board_temp_max_hyst(void *driver, unsigned int temp_index, char *buf, size_t count)
{
    struct s3ip_cpu_temp_data *data;
    unsigned char node_name[MAX_SYSFS_ATTR_NAME_LENGTH];
    struct device *dev;
    int ret;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (temp_index >= MAX_SENSOR_NUM) {
        data = &cpu_temp_data_list[temp_index - MAX_SENSOR_NUM];
        if (data->dev != NULL) {
            if (data->auto_inc != 0)
                sprintf(node_name, "%s%d%s", CPU_TEMP_NODE, temp_index-MAX_SENSOR_NUM+1, CPU_TEMP_MAX_HYST);
            else
                sprintf(node_name, "%s1%s", CPU_TEMP_NODE, CPU_TEMP_MAX_HYST);

            ret = get_cpu_hwmon_attr_by_name(data, node_name, buf);
            if (ret < 0) {
               ret = sprintf(buf, "%d\n", DEFAULT_CPU_TEMP_MAX);
            }
        } else {
            ret = -ENXIO;
        }
    } else {
        sprintf(node_name, "%s%s", TEMP_NODE, TEMP_MAX_HYST);
        dev = sensor_arry[temp_index];
        ret = get_hwmon_attr_by_name(dev, node_name, buf);
    }

    read_unlock(&list_lock);

    return ret;
}

static int drv_sensor_temp_dev_init(struct drv_temp_sensor_clx *temp)
{
    struct drv_temp_sensor_clx *dev = (struct drv_temp_sensor_clx *)temp;
    sensor_map_index = dev->temp_if.sensor_map_index;
    return DRIVER_OK;
}


int drv_sensor_temp_init(void **temp_driver)
{
    struct drv_temp_sensor_clx *temp = &driver_temp_clx;
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_TEMP, "clx_driver_clx_temp_init\n");
    ret = drv_sensor_temp_dev_init(temp);
    if (ret != DRIVER_OK)
        return ret;

    temp->temp_if.get_main_board_temp_number = drv_sensor_get_main_board_temp_number;
    temp->temp_if.get_main_board_temp_alias = drv_sensor_get_main_board_temp_alias;
    temp->temp_if.get_main_board_temp_type = drv_sensor_get_main_board_temp_type;

    temp->temp_if.get_main_board_temp_max = drv_sensor_get_main_board_temp_max;
    temp->temp_if.set_main_board_temp_max = drv_sensor_set_main_board_temp_max;

    temp->temp_if.get_main_board_temp_max_hyst = drv_sensor_get_main_board_temp_max_hyst;
    temp->temp_if.set_main_board_temp_max_hyst = drv_sensor_set_main_board_temp_max_hyst;

    temp->temp_if.get_main_board_temp_min = drv_sensor_get_main_board_temp_min;
    temp->temp_if.set_main_board_temp_min = drv_sensor_set_main_board_temp_min;

    temp->temp_if.get_main_board_temp_value = drv_sensor_get_main_board_temp_value;
    *temp_driver = temp;
    LOG_INFO(CLX_DRIVER_TYPES_TEMP, "TEMP driver clx initialization done.\r\n");
    return DRIVER_OK;
}
//clx_driver_define_initcall(drv_sensor_temp_init);
