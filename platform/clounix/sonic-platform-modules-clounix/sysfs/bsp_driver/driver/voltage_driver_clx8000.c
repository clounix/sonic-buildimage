#include <linux/io.h>

#include "voltage_driver_clx8000.h"
#include "clx_driver.h"

#include <linux/rwlock.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/string.h>
#include "clounix/pmbus_dev_common.h"

//internal function declaration
struct voltage_driver_clx8000 driver_voltage_clx8000;

#define TOTAL_SENSOR_NUM (12)
#define REAL_MAX_SENSOR_NUM (4)

#define VOL_NODE "in"

#define VOL_MIN "_min"
#define VOL_MAX "_max"
#define VOL_DIR "_label"
#define VOL_VALUE "_input"

static DEFINE_RWLOCK(list_lock);
/*
    [0]:addr
    [1]:location in sensor_arry
    [2]:sensor offse
*/
static short sensor_map[][3] = {
    {0x10, 0, -1},
    {0x20, 1, 1},
    {0x21, 2, 5},
    {0x29, 3, 9},
    {0x0, 0},
};

/*
    [0]: range
    [1]: location in sensor_arry
*/
static unsigned char vol_index_range_map[][2] = {
    {2, 0},
    {6, 1},
    {10, 2},
    {0, 3},
};

static struct i2c_client *sensor_arry[REAL_MAX_SENSOR_NUM + 1] = {0};

int vol_sensor_add(struct i2c_client *client)
{
    int ret = -ENOMEM;
    int i;

    write_lock(&list_lock);
    
    i = 0;
    while (sensor_map[i][ADDR_LABEL] != 0) {
        if (client->addr == sensor_map[i][ADDR_LABEL]) {
            if (sensor_arry[(sensor_map[i][LOCATION_LABEL])] == NULL) {
                sensor_arry[(sensor_map[i][LOCATION_LABEL])] = client;
                ret = 0;
            }
            break;
        }
        i++;
    }

    write_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(vol_sensor_add);

void vol_sensor_del(struct i2c_client *client)
{
    int i;

    write_lock(&list_lock);
    for (i=0; i<REAL_MAX_SENSOR_NUM; i++) {
        if (sensor_arry[i] == client)
            sensor_arry[i] = NULL;
    }
    write_unlock(&list_lock);

    return;
}
EXPORT_SYMBOL(vol_sensor_del);

static int clx_driver_clx8000_get_main_board_vol_number(void *driver)
{
    /* add vendor codes here */
    return TOTAL_SENSOR_NUM;
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
static ssize_t clx_driver_clx8000_get_main_board_vol_alias(void *driver, unsigned int vol_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(vol_index, vol_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char vol_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    unsigned char tmp_buf[PMBUS_NAME_SIZE*3];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        vol_sensor_index = get_sensor_internal_index(sensor_index, vol_index, sensor_map);
        sprintf(node_name, "%s%d%s", VOL_NODE, vol_sensor_index, VOL_DIR);
        if (get_attr_val_by_name(client, node_name, tmp_buf) > 0)
            ret = sprintf(buf, "%s:%x %s", client->name, client->addr, tmp_buf);
    }
    read_unlock(&list_lock);

    return ret;
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
static ssize_t clx_driver_clx8000_get_main_board_vol_type(void *driver, unsigned int vol_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(vol_index, vol_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        ret = sprintf(buf, "%s\n", client->name);
    }
    read_unlock(&list_lock);
    
    return ret;
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
static ssize_t clx_driver_clx8000_get_main_board_vol_max(void *driver, unsigned int vol_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(vol_index, vol_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char vol_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        vol_sensor_index = get_sensor_internal_index(sensor_index, vol_index, sensor_map);
        sprintf(node_name, "%s%d%s", VOL_NODE, vol_sensor_index, VOL_MAX);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);
    
    return ret;
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
static int clx_driver_clx8000_set_main_board_vol_max(void *driver, unsigned int vol_index, const char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(vol_index, vol_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char vol_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        vol_sensor_index = get_sensor_internal_index(sensor_index, vol_index, sensor_map);
        sprintf(node_name, "%s%d%s", VOL_NODE, vol_sensor_index, VOL_MAX);
        ret = set_attr_val_by_name(client, node_name, buf, count);
    }
    read_unlock(&list_lock);

    return ret;
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
static ssize_t clx_driver_clx8000_get_main_board_vol_min(void *driver, unsigned int vol_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(vol_index, vol_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char vol_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        vol_sensor_index = get_sensor_internal_index(sensor_index, vol_index, sensor_map);
        sprintf(node_name, "%s%d%s", VOL_NODE, vol_sensor_index, VOL_MIN);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
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
static int clx_driver_clx8000_set_main_board_vol_min(void *driver, unsigned int vol_index, const char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(vol_index, vol_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char vol_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        vol_sensor_index = get_sensor_internal_index(sensor_index, vol_index, sensor_map);
        sprintf(node_name, "%s%d%s", VOL_NODE, vol_sensor_index, VOL_MIN);
        ret = set_attr_val_by_name(client, node_name, buf, count);
    }
    read_unlock(&list_lock);

    return ret;
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
static ssize_t clx_driver_clx8000_get_main_board_vol_range(void *driver, unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
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
static ssize_t clx_driver_clx8000_get_main_board_vol_nominal_value(void *driver, unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
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
static ssize_t clx_driver_clx8000_get_main_board_vol_value(void *driver, unsigned int vol_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(vol_index, vol_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char vol_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        vol_sensor_index = get_sensor_internal_index(sensor_index, vol_index, sensor_map);
        sprintf(node_name, "%s%d%s", VOL_NODE, vol_sensor_index, VOL_VALUE);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

static int clx_driver_clx8000_voltage_dev_init(struct voltage_driver_clx8000 *voltage)
{
    return DRIVER_OK;
}

//void clx_driver_clx8000_voltage_init(struct voltage_driver_clx8000 **voltage_driver)
void clx_driver_clx8000_voltage_init(void **voltage_driver)
{
    struct voltage_driver_clx8000 *voltage = &driver_voltage_clx8000;

    VOL_SENSOR_INFO("clx_driver_clx8000_voltage_init\n");
    clx_driver_clx8000_voltage_dev_init(voltage);
    voltage->voltage_if.get_main_board_vol_number = clx_driver_clx8000_get_main_board_vol_number;
    voltage->voltage_if.get_main_board_vol_alias = clx_driver_clx8000_get_main_board_vol_alias;
    voltage->voltage_if.get_main_board_vol_type = clx_driver_clx8000_get_main_board_vol_type;
    voltage->voltage_if.get_main_board_vol_max = clx_driver_clx8000_get_main_board_vol_max;
    voltage->voltage_if.set_main_board_vol_max = clx_driver_clx8000_set_main_board_vol_max;
    voltage->voltage_if.get_main_board_vol_min = clx_driver_clx8000_get_main_board_vol_min;
    voltage->voltage_if.set_main_board_vol_min = clx_driver_clx8000_set_main_board_vol_min;
    voltage->voltage_if.get_main_board_vol_range = clx_driver_clx8000_get_main_board_vol_range;
    voltage->voltage_if.get_main_board_vol_nominal_value = clx_driver_clx8000_get_main_board_vol_nominal_value;
    voltage->voltage_if.get_main_board_vol_value = clx_driver_clx8000_get_main_board_vol_value;
    *voltage_driver = voltage;
    VOL_SENSOR_INFO("VOLTAGE driver clx8000 initialization done.\r\n");
}
//clx_driver_define_initcall(clx_driver_clx8000_voltage_init);

