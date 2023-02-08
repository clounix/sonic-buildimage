#include <linux/io.h>
#include <linux/rwlock.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/string.h>

#include "drv_psu_clx.h"
#include "clx_driver.h"
#include "clounix/pmbus_dev_common.h"

static DEFINE_RWLOCK(list_lock);

#define PER_PSU_TEMP_NUM (3)

#define MAX_PSU_DATA_NUM (2)

#define INDEX_CHECK(index) \
    if (index > MAX_PSU_DATA_NUM) \
        return -ENODATA

#define OUTPUT_NODE "_input"
#define DIR_NODE "_label"
#define DIR_IN "in"
#define DIR_OUT "out"

#define TEMP_CLASS "temp"
#define TEMP_MIN "_min"
#define TEMP_MAX "_crit"
#define TEMP_MAX_HYST "_max"

#define FAN_CLASS "fan"
#define FAN_RATIO "_target"

#define POWER_CLASS "power"
#define CURR_CLASS "curr"
#define VOL_CLASS "in"

#define PSU
#define PSU_SERIAL "mfr_serial"
#define PSU_PART "mfr_location"
#define PSU_TYPE "mfr_model"
#define PSU_HW_VERSION "mfr_revision"
#define PRST "_prst"
#define ACOK "_acok"
#define PWOK "_pwok"
#define LED_STATUS "led_status"

#define PSU_TEMP_INDEX_OFFSET (1)

static struct i2c_client *client_arry[MAX_PSU_DATA_NUM+1] = {0};
static struct device *client_priv_arry[MAX_PSU_DATA_NUM+1] = {0};

struct drv_psu_clx drv_psu;

static unsigned char psu_map[][2] = {
    {0x5a, 1},
    {0x58, 2},
    {0, 0},
};

int psu_add_priv(struct i2c_client *client, struct device *dev)
{
    int ret = -ENOMEM;
    int i;

    write_lock(&list_lock);

    i = 0;
    while (psu_map[i][0] != 0) {
        if (client->addr == psu_map[i][0]) {
            if (client_priv_arry[(psu_map[i][1])] == NULL) {
                LOG_DBG(CLX_DRIVER_TYPES_PSU, "add client priv %s\n", client->name);
                client_priv_arry[(psu_map[i][1])] = dev;
                ret = 0;
            }
            break;
        }

        i++;
    }

    write_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(psu_add_priv);

int psu_add(struct i2c_client *client)
{
    int ret = -ENOMEM;
    int i;

    write_lock(&list_lock);
   
    i = 0;
    while (psu_map[i][0] != 0) {
        if (client->addr == psu_map[i][0]) {
            if (client_arry[(psu_map[i][1])] == NULL) {
                LOG_DBG(CLX_DRIVER_TYPES_PSU, "add client %s\n", client->name);
                client_arry[(psu_map[i][1])] = client;
                ret = 0;
            }
            break;
        }

        i++;
    }

    write_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(psu_add);

void psu_del_priv(struct i2c_client *client)
{
    int i;

    write_lock(&list_lock);
    i = 0;
    while (psu_map[i][0] != 0) {
        if (client->addr == psu_map[i][0]) {
            if (client_priv_arry[(psu_map[i][1])] != NULL) {
                LOG_DBG(CLX_DRIVER_TYPES_PSU, "del client priv %s\n", client->name);
                client_priv_arry[(psu_map[i][1])] = NULL;
            }
            break;
        }

        i++;
    }
    write_unlock(&list_lock);

    return;
}
EXPORT_SYMBOL(psu_del_priv);

void psu_del(struct i2c_client *client)
{
    int i;

    write_lock(&list_lock);
    for (i=1; i<=MAX_PSU_DATA_NUM; i++) {
        if (client_arry[i] == client) {
            LOG_DBG(CLX_DRIVER_TYPES_PSU, "del client %s\n", client->name);
            client_arry[i] = NULL;
        }
    }
    write_unlock(&list_lock);

    return;
}
EXPORT_SYMBOL(psu_del);

static int drv_get_psu_number(void *driver)
{
    /* add vendor codes here */
    return MAX_PSU_DATA_NUM;
}

int get_priv_attr_val_by_name(struct device *dev, char *node_name, char *buf)
{
    struct device_attribute *dev_attr;
    struct attribute *a;
    struct attribute **attrs;
    int i, j;

    for (i=0; dev->groups[i] != NULL; i++) {
        attrs = dev->groups[i]->attrs;
        for (j=0; attrs[j] != NULL; j++) {
            a = attrs[j];
            LOG_DBG(CLX_DRIVER_TYPES_PSU, "node: %s\n", a->name);
            if (strcmp(a->name, node_name) == 0) {
                dev_attr = container_of(a, struct device_attribute, attr);
                return dev_attr->show(dev, dev_attr, buf);
            }
        }
    }

    return -1;
}

int get_attr_val_total_by_name(struct i2c_client *client, char *class, char *dir, char *buf)
{
    struct pmbus_data *p = i2c_get_clientdata(client);
    struct device_attribute *dev_attr;
    struct attribute *a;
    struct device dev = {0};
    int ret = 0;
    char tmp_buf[PMBUS_NAME_SIZE*4];
    unsigned char target_name[PMBUS_NAME_SIZE] = {0};
    int i;

    for (i=0; i<p->num_attributes; i++) {
        a = p->group.attrs[i];
        LOG_DBG(CLX_DRIVER_TYPES_PSU, "node: %s\n", a->name);
        if (strcmp(a->name, target_name) == 0) {
            dev_attr = container_of(a, struct device_attribute, attr);
            dev.parent = &client->dev;
            ret = dev_attr->show(&dev, dev_attr, buf);
            memset(target_name, 0, strlen(target_name));
            break;
        } else if (memcmp(a->name, class, strlen(class)) == 0) {
            if (strstr(a->name, DIR_NODE) != NULL) {
                dev_attr = container_of(a, struct device_attribute, attr);
                dev_attr->show(NULL, dev_attr, tmp_buf);
                if (strstr(tmp_buf, dir) != NULL) {
                    memcpy(target_name, a->name, strlen(class)+1);
                    strcat(target_name, OUTPUT_NODE);
                }
            }
        }
    }

    return ret;
}

static int drv_get_psu_temp_number(void *driver, unsigned int psu_index)
{
    return PER_PSU_TEMP_NUM;
}

/*
 * clx_get_psu_model_name - Used to get psu model name,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_model_name(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct device *dev;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    write_lock(&list_lock);
    dev = client_priv_arry[psu_index];
    if (dev != NULL) {
        ret = get_priv_attr_val_by_name(dev, PSU_TYPE, buf);
    }
    write_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_serial_number - Used to get psu serial number,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_serial_number(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct device *dev;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    write_lock(&list_lock);
    dev = client_priv_arry[psu_index];
    if (dev != NULL) {
        ret = get_priv_attr_val_by_name(dev, PSU_SERIAL, buf);
    }
    write_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_part_number - Used to get psu part number,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_part_number(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct device *dev;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    write_lock(&list_lock);
    dev = client_priv_arry[psu_index];
    if (dev != NULL) {
        ret = get_priv_attr_val_by_name(dev, PSU_PART, buf);
    }
    write_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_hardware_version - Used to get psu hardware version,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_hardware_version(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct device *dev;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);
    
    write_lock(&list_lock);
    dev = client_priv_arry[psu_index];
    if (dev != NULL) {
        ret = get_priv_attr_val_by_name(dev, PSU_HW_VERSION, buf);
    }
    write_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_type - Used to get the input type of psu
 * filled the value to buf, input type value define as below:
 * 0: DC
 * 1: AC
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_type(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return sprintf(buf, "1\n");
}

/*
 * clx_get_psu_in_curr - Used to get the input current of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_in_curr(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        ret = get_attr_val_total_by_name(client, CURR_CLASS, DIR_IN, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_in_vol - Used to get the input voltage of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_in_vol(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        ret = get_attr_val_total_by_name(client, VOL_CLASS, DIR_IN, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_in_power - Used to get the input power of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_in_power(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        ret = get_attr_val_total_by_name(client, POWER_CLASS, DIR_IN, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_out_curr - Used to get the output current of psu
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_out_curr(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        ret = get_attr_val_total_by_name(client, CURR_CLASS, DIR_OUT, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_out_vol - Used to get the output voltage of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_out_vol(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        ret = get_attr_val_total_by_name(client, VOL_CLASS, DIR_OUT, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_out_power - Used to get the output power of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_out_power(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        ret = get_attr_val_total_by_name(client, POWER_CLASS, DIR_OUT, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_out_max_power - Used to get the output max power of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_out_max_power(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    return -1;
}

/*
 * clx_get_psu_present_status - Used to get psu present status
 * filled the value to buf, psu present status define as below:
 * 0: ABSENT
 * 1: PRESENT
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_present_status(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct device *dev;
    unsigned char prst = 0;
    unsigned char acok = 0;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    write_lock(&list_lock);
    dev = client_priv_arry[psu_index];
    if (dev != NULL) {
        sprintf(node_name, "psu_%d%s", psu_index, PRST);
        ret = get_priv_attr_val_by_name(dev, node_name, buf);
        if (ret > 0)
            prst = *buf - '0';
        
        sprintf(node_name, "psu_%d%s", psu_index, ACOK);
        ret = get_priv_attr_val_by_name(dev, node_name, buf);
        if (ret > 0)
            acok = *buf - '0';
    }
    write_unlock(&list_lock);

    if (prst == 0)
        ret = sprintf(buf, "0\n");

    if (prst == 1 && acok == 0)
        ret = sprintf(buf,"2\n");

    if (prst == 1 && acok == 1)
        ret = sprintf(buf,"1\n");

    return ret;
}

/*
 * clx_get_psu_in_status - Used to get psu input status
 * filled the value to buf, psu input status define as below:
 * 0: NOT OK
 * 1: OK
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_in_status(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct device *dev;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    write_lock(&list_lock);
    dev = client_priv_arry[psu_index];
    if (dev != NULL) {
        sprintf(node_name, "psu_%d%s", psu_index, ACOK);
        ret = get_priv_attr_val_by_name(dev, node_name, buf);
    }
    write_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_out_status - Used to get psu output status
 * filled the value to buf, psu output status define as below:
 * 0: NOT OK
 * 1: OK
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_out_status(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct device *dev;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    write_lock(&list_lock);
    dev = client_priv_arry[psu_index];
    if (dev != NULL) {
        sprintf(node_name, "psu_%d%s", psu_index, PWOK);
        ret = get_priv_attr_val_by_name(dev, node_name, buf);
    }
    write_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_fan_speed - Used to get psu fan speed
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_fan_speed(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", FAN_CLASS, 1, OUTPUT_NODE);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_fan_ratio - Used to get the ratio of psu fan
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_fan_ratio(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", FAN_CLASS, 1, FAN_RATIO);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_set_psu_fan_ratio - Used to set the ratio of psu fan
 * @psu_index: start with 1
 * @ratio: from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_set_psu_fan_ratio(void *driver, unsigned int psu_index, int ratio)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;
    char buf[16];

    if ((ratio < 0) || (ratio > 100))
        return ret;
    
    sprintf(buf, "%d", ratio);

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", FAN_CLASS, 1, FAN_RATIO);
        ret = set_attr_val_by_name(client, node_name, buf, strlen(buf)+1);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_fan_direction - Used to get psu air flow direction,
 * filled the value to buf, air flow direction define as below:
 * 0: F2B
 * 1: B2F
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_fan_direction(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    return -1;
}

/*
 * clx_get_psu_led_status - Used to get psu led status
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
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_led_status(void *driver, unsigned int psu_index, char *buf, size_t count)
{
    /* org output:
        "green",
        "amber",
        "off",
        "blink_green",
    */ 
    int ret = 0;
    struct device *dev;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);
    write_lock(&list_lock);
    dev = client_priv_arry[psu_index];
    if (dev != NULL) {
        ret = get_priv_attr_val_by_name(dev, LED_STATUS, buf);
    }
    write_unlock(&list_lock);

    switch (*buf) {
        case 'g':
            ret = 1;
            break;
        case 'a':
            ret = 2;
            break;
        case 'o':
            ret = 0;
            break;
        case 'b':
            ret = 4;
            break;
    }

    return sprintf(buf, "%d\n", ret);
}

/*
 * clx_get_psu_temp_alias - Used to identify the location of the temperature sensor of psu,
 * @psu_index: start with 1
 * @temp_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_temp_alias(void *driver, unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    return -1;
}

/*
 * clx_get_psu_temp_type - Used to get the model of temperature sensor of psu,
 * @psu_index: start with 1
 * @temp_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_temp_type(void *driver, unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    return -1;
}

/*
 * clx_get_psu_temp_max - Used to get the maximum threshold of temperature sensor of psu,
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @temp_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_temp_max(void *driver, unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    temp_index += PSU_TEMP_INDEX_OFFSET;
    if (client != NULL) {
        //sprintf(node_name, "%s%d%s", TEMP_CLASS, temp_index, TEMP_MAX);
        //ret = get_attr_val_by_name(client, node_name, buf);
	//is it not support? default seting 100.000'
        ret = (ssize_t)snprintf(buf, count, "%d\n", 100000);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_set_psu_temp_max - Used to set the maximum threshold of temperature sensor of psu,
 * get value from buf and set it to maximum threshold of psu temperature sensor
 * @psu_index: start with 1
 * @temp_index: start with 0
 * @buf: the buf store the data to be set, eg '80.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_set_psu_temp_max(void *driver, unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    temp_index += PSU_TEMP_INDEX_OFFSET;
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", TEMP_CLASS, temp_index, TEMP_MAX);
        ret = set_attr_val_by_name(client, node_name, buf, count);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_temp_min - Used to get the minimum threshold of temperature sensor of psu,
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @temp_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_temp_min(void *driver, unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    temp_index += PSU_TEMP_INDEX_OFFSET;
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", TEMP_CLASS, temp_index, TEMP_MIN);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_set_psu_temp_min - Used to set the minimum threshold of temperature sensor of psu,
 * get value from buf and set it to minimum threshold of psu temperature sensor
 * @psu_index: start with 1
 * @temp_index: start with 0
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_set_psu_temp_min(void *driver, unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);
    read_lock(&list_lock);
    client = client_arry[psu_index];
    temp_index += PSU_TEMP_INDEX_OFFSET;
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", TEMP_CLASS, temp_index, TEMP_MIN);
        ret = set_attr_val_by_name(client, node_name, buf, count);
    }
    read_unlock(&list_lock);

    return ret;
}

/*
 * clx_get_psu_temp_value - Used to get the input value of temperature sensor of psu
 * filled the value to buf, and the value keep three decimal places
 * @psu_index: start with 1
 * @temp_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_psu_temp_value(void *driver, unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);
    
    read_lock(&list_lock);
    client = client_arry[psu_index];
    temp_index += PSU_TEMP_INDEX_OFFSET;
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", TEMP_CLASS, temp_index, OUTPUT_NODE);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

static ssize_t drv_get_psu_temp_max_hyst(void *driver, unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    temp_index += PSU_TEMP_INDEX_OFFSET;
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", TEMP_CLASS, temp_index, TEMP_MAX_HYST);
        ret = get_attr_val_by_name(client, node_name, buf);
    }
    read_unlock(&list_lock);

    return ret;
}

int drv_set_psu_temp_max_hyst(void *driver, unsigned int psu_index, unsigned int temp_index,
                   const char *buf, size_t count)
{
    int ret = 0;
    char node_name[PMBUS_NAME_SIZE];
    struct i2c_client *client;

    /* add vendor codes here */
    INDEX_CHECK(psu_index);

    read_lock(&list_lock);
    client = client_arry[psu_index];
    temp_index += PSU_TEMP_INDEX_OFFSET;
    if (client != NULL) {
        sprintf(node_name, "%s%d%s", TEMP_CLASS, temp_index, TEMP_MAX_HYST);
        ret = set_attr_val_by_name(client, node_name, buf, count);
    }
    read_unlock(&list_lock);

    return ret;
}

static int drv_psu_dev_init(struct drv_psu_clx *psu)
{
    return DRIVER_OK;
}

int drv_psu_clx_init(void **psu_driver)
{
    struct drv_psu_clx *psu = &drv_psu;
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_PSU, "clx_drive_psu_init\n");
    ret = drv_psu_dev_init(psu);
    if (ret != DRIVER_OK)
        return ret;
    psu->psu_if.get_psu_number = drv_get_psu_number;
    psu->psu_if.get_psu_temp_number = drv_get_psu_temp_number;
    psu->psu_if.get_psu_model_name = drv_get_psu_model_name;
    psu->psu_if.get_psu_serial_number = drv_get_psu_serial_number;
    psu->psu_if.get_psu_part_number = drv_get_psu_part_number;
    psu->psu_if.get_psu_hardware_version = drv_get_psu_hardware_version;
    psu->psu_if.get_psu_type = drv_get_psu_type;
    psu->psu_if.get_psu_in_curr = drv_get_psu_in_curr;
    psu->psu_if.get_psu_in_vol = drv_get_psu_in_vol;
    psu->psu_if.get_psu_in_power = drv_get_psu_in_power;
    psu->psu_if.get_psu_out_curr = drv_get_psu_out_curr;
    psu->psu_if.get_psu_out_vol = drv_get_psu_out_vol;
    psu->psu_if.get_psu_out_power = drv_get_psu_out_power;
    psu->psu_if.get_psu_out_max_power = drv_get_psu_out_max_power;
    psu->psu_if.get_psu_present_status = drv_get_psu_present_status;
    psu->psu_if.get_psu_in_status = drv_get_psu_in_status;
    psu->psu_if.get_psu_out_status = drv_get_psu_out_status;
    psu->psu_if.get_psu_fan_speed = drv_get_psu_fan_speed;
    psu->psu_if.get_psu_fan_ratio = drv_get_psu_fan_ratio;
    psu->psu_if.set_psu_fan_ratio = drv_set_psu_fan_ratio;
    psu->psu_if.get_psu_fan_direction = drv_get_psu_fan_direction;
    psu->psu_if.get_psu_led_status = drv_get_psu_led_status;
    psu->psu_if.get_psu_temp_alias = drv_get_psu_temp_alias;
    psu->psu_if.get_psu_temp_type = drv_get_psu_temp_type;
    psu->psu_if.get_psu_temp_max = drv_get_psu_temp_max;
    psu->psu_if.set_psu_temp_max = drv_set_psu_temp_max;
    psu->psu_if.get_psu_temp_min = drv_get_psu_temp_min;
    psu->psu_if.set_psu_temp_min = drv_set_psu_temp_min;
    psu->psu_if.get_psu_temp_value = drv_get_psu_temp_value;

    psu->psu_if.get_psu_temp_max_hyst = drv_get_psu_temp_max_hyst;
    psu->psu_if.set_psu_temp_max_hyst = drv_set_psu_temp_max_hyst;

    *psu_driver = psu;
    LOG_INFO(CLX_DRIVER_TYPES_PSU, "PSU driver initialization done.\n");
    return DRIVER_OK;
}
