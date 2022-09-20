#include <linux/io.h>

#include "current_driver_clx8000.h"
#include "clx_driver.h"

#include <linux/rwlock.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/string.h>
#include "clounix/pmbus_dev_common.h"

//internal function declaration
struct current_driver_clx8000 driver_current_clx8000;

#define TOTAL_SENSOR_NUM (6)
#define REAL_MAX_SENSOR_NUM (4)

#define CURR_NODE "curr"
#define CURR_MIN "_min"
#define CURR_MAX "_max"
#define CURR_DIR "_label"
#define CURR_VALUE "_input"

static DEFINE_RWLOCK(list_lock);
/*
    [0]:addr
    [1]:location in sensor_arry
    [2]:sensor offse
*/
static short sensor_map[][3] = {
    {0x10, 0, -1},
    {0x20, 1, 0},
    {0x21, 2, 2},
    {0x29, 3, 4},
    {0x0, 0},
};

/*
    [0]: range
    [1]: location in sensor_arry
*/
static unsigned char curr_index_range_map[][2] = {
    {1, 0},
    {3, 1},
    {5, 2},
    {0, 3},
};

static struct i2c_client *sensor_arry[REAL_MAX_SENSOR_NUM + 1] = {0};

int curr_sensor_add(struct i2c_client *client)
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
EXPORT_SYMBOL(curr_sensor_add);

void curr_sensor_del(struct i2c_client *client)
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
EXPORT_SYMBOL(curr_sensor_del);

static int clx_driver_clx8000_get_main_board_curr_number(void *driver)
{
    /* add vendor codes here */
    return TOTAL_SENSOR_NUM;
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
static ssize_t clx_driver_clx8000_get_main_board_curr_alias(void *driver, unsigned int curr_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(curr_index, curr_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char curr_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    unsigned char tmp_buf[PMBUS_NAME_SIZE*3];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        curr_sensor_index = get_sensor_internal_index(sensor_index, curr_index, sensor_map);
        sprintf(node_name, "%s%d%s", CURR_NODE, curr_sensor_index, CURR_DIR);
        if (get_attr_val_by_name(client, node_name, tmp_buf) > 0)
            ret = sprintf(buf, "%s:%x %s", client->name, client->addr, tmp_buf);
    }
    read_unlock(&list_lock);

    return ret;
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
static ssize_t clx_driver_clx8000_get_main_board_curr_type(void *driver, unsigned int curr_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(curr_index, curr_index_range_map);
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
static ssize_t clx_driver_clx8000_get_main_board_curr_max(void *driver, unsigned int curr_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(curr_index, curr_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char curr_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        curr_sensor_index = get_sensor_internal_index(sensor_index, curr_index, sensor_map);
        sprintf(node_name, "%s%d%s", CURR_NODE, curr_sensor_index, CURR_MAX);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);
    
    return ret;
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
static int clx_driver_clx8000_set_main_board_curr_max(void *driver, unsigned int curr_index, const char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(curr_index, curr_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char curr_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        curr_sensor_index = get_sensor_internal_index(sensor_index, curr_index, sensor_map);
        sprintf(node_name, "%s%d%s", CURR_NODE, curr_sensor_index, CURR_MAX);
        ret = set_attr_val_by_name(client, node_name, buf, count);
    }
    read_unlock(&list_lock);

    return ret;
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
static ssize_t clx_driver_clx8000_get_main_board_curr_min(void *driver, unsigned int curr_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(curr_index, curr_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char curr_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        curr_sensor_index = get_sensor_internal_index(sensor_index, curr_index, sensor_map);
        sprintf(node_name, "%s%d%s", CURR_NODE, curr_sensor_index, CURR_MIN);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
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
static int clx_driver_clx8000_set_main_board_curr_min(void *driver, unsigned int curr_index, const char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(curr_index, curr_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char curr_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        curr_sensor_index = get_sensor_internal_index(sensor_index, curr_index, sensor_map);
        sprintf(node_name, "%s%d%s", CURR_NODE, curr_sensor_index, CURR_MIN);
        ret = set_attr_val_by_name(client, node_name, buf, count);
    }
    read_unlock(&list_lock);

    return ret;
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
static ssize_t clx_driver_clx8000_get_main_board_curr_value(void *driver, unsigned int curr_index, char *buf, size_t count)
{
    unsigned char sensor_index = get_psu_sensor_index(curr_index, curr_index_range_map);
    struct i2c_client *client = sensor_arry[sensor_index];
    unsigned char curr_sensor_index;
    unsigned char node_name[PMBUS_NAME_SIZE];
    int ret = -1;

    /* add vendor codes here */
    read_lock(&list_lock);
    if (client != NULL) {
        curr_sensor_index = get_sensor_internal_index(sensor_index, curr_index, sensor_map);
        sprintf(node_name, "%s%d%s", CURR_NODE, curr_sensor_index, CURR_VALUE);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

static int clx_driver_clx8000_current_dev_init(struct current_driver_clx8000 *curr)
{
    return DRIVER_OK;
}

//void clx_driver_clx8000_current_init(struct current_driver_clx8000 **current_driver)
void clx_driver_clx8000_current_init(void **current_driver)
{
    struct current_driver_clx8000 *curr = &driver_current_clx8000;

    CURR_SENSOR_INFO("clx_driver_clx8000_current_init\n");
    clx_driver_clx8000_current_dev_init(curr);
    curr->current_if.get_main_board_curr_number = clx_driver_clx8000_get_main_board_curr_number;
    curr->current_if.get_main_board_curr_alias = clx_driver_clx8000_get_main_board_curr_alias;
    curr->current_if.get_main_board_curr_type = clx_driver_clx8000_get_main_board_curr_type;
    curr->current_if.get_main_board_curr_max = clx_driver_clx8000_get_main_board_curr_max;
    curr->current_if.set_main_board_curr_max = clx_driver_clx8000_set_main_board_curr_max;
    curr->current_if.get_main_board_curr_min = clx_driver_clx8000_get_main_board_curr_min;
    curr->current_if.set_main_board_curr_min = clx_driver_clx8000_set_main_board_curr_min;
    curr->current_if.get_main_board_curr_value = clx_driver_clx8000_get_main_board_curr_value;
    *current_driver = curr;
    CURR_SENSOR_INFO("CURRENT driver clx8000 initialization done.\r\n");
}
//clx_driver_define_initcall(clx_driver_clx8000_current_init);

