#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include "pddf_client_defs.h"
#include "pddf_xcvr_defs.h"
#include "pddf_xcvr_api.h"

static int *log_level = &xcvr_log_level;

extern XCVR_SYSFS_ATTR_OPS xcvr_ops[];
extern struct sensor_device_attribute sensor_dev_attr_xcvr_led_en;
extern struct sensor_device_attribute sensor_dev_attr_xcvr_red_led;
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);

static int xcvr_fpgapci_read(XCVR_ATTR *info)
{
    int reg_val= 0;
    uint32_t offset = 0;

    if (ptr_fpgapci_read == NULL) {
        pddf_err(XCVR, "Doesn't support FPGAPCI read yet");
        return (-1);
    }

    offset = info->devaddr + info->offset;
    reg_val = ptr_fpgapci_read(offset);
    pddf_dbg(XCVR, "xcvr fpgapci read: offset==0x%x, reg==0x%x", offset, reg_val);

    return reg_val;
}

static int xcvr_fpgapci_write(XCVR_ATTR *info, uint32_t val)
{
    int status= 0;
    uint32_t reg, val_mask = 0, dnd_value = 0, reg_val;
    uint32_t offset = 0;

    if (ptr_fpgapci_read == NULL || ptr_fpgapci_write == NULL) {
        pddf_err(XCVR, "Doesn't support FPGAPCI read or write yet");
        return (-1);
    }

    offset = info->devaddr + info->offset;
    val_mask = BIT_INDEX(info->mask);
    reg_val = ptr_fpgapci_read(offset);
    dnd_value =  reg_val & ~val_mask;

    if (((val == 1) && (info->cmpval != 0)) || ((val == 0) && (info->cmpval == 0)))
        reg = dnd_value | val_mask;
    else
        reg = dnd_value;

    pddf_dbg(XCVR, "xcvr fpgapci write: offset==0x%x, reg==0x%x", offset, reg);
    status = ptr_fpgapci_write(offset, reg);
    return status;
}

static int get_xcvr_module_attr_data(struct i2c_client *client, struct device *dev, 
                            struct device_attribute *da)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    XCVR_ATTR *attr_data = NULL;
    int i;

    for (i=0; i < pdata->len; i++)
    {
        attr_data = &pdata->xcvr_attrs[i];
        if (strcmp(attr_data->aname, attr->dev_attr.attr.name) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int xcvr_red_led_do_get(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status;
    uint32_t red_led;

    if (strcmp(info->devtype, "fpgapci") != 0) {
        pddf_err(XCVR, "Unsupported device type for red LED: %s (only fpgapci allowed)\n", info->devtype);
        return -EINVAL;
    }

    status = xcvr_fpgapci_read(info);
    // 不判断status<0（避免0xFFFFFFFF误判为错误）
    red_led = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
    
    pddf_dbg(XCVR, "Red LED: 0x%x, reg_value=0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x, cmp=0x%x\n",
             red_led, status, info->devaddr, info->mask, info->offset, info->cmpval);

    return red_led;
}

static int xcvr_red_led_do_set(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data, int value)
{
    int status;

    if (strcmp(info->devtype, "fpgapci") != 0) {
        pddf_err(XCVR, "Unsupported device type to set red LED: %s (only fpgapci allowed)\n", info->devtype);
        return -EINVAL;
    }

    status = xcvr_fpgapci_write(info, value);
    if (status < 0) {
        pddf_err(XCVR, "Failed to set red LED: status=%d\n", status);
    }

    return status;
}

static ssize_t xcvr_red_led_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;
    int idx, status = 0;
    int led_state = 0;

    idx = get_xcvr_module_attr_data(client, dev, da);
    if (idx >= 0)
        attr_data = &pdata->xcvr_attrs[idx];
    
    if (attr_data != NULL) {
        attr_ops = &xcvr_ops[attr->index];

        mutex_lock(&data->update_lock);
        if (attr_ops->pre_get != NULL) {
            status = (attr_ops->pre_get)(client, attr_data, data);
            if (status != 0)
                pddf_info(XCVR, "%s[%d]: %s fails for %s attribute. ret %d\n",
                          __FUNCTION__, __LINE__, dev_name(&client->dev), attr_data->aname, status);
        } 
        if (attr_ops->do_get != NULL) {
            led_state = (attr_ops->do_get)(client, attr_data, data);
            if (led_state < 0) {
                pddf_info(XCVR, "%s[%d]: %s fails for %s attribute. ret %d\n",
                          __FUNCTION__, __LINE__, dev_name(&client->dev), attr_data->aname, led_state);
                led_state = 0;
            }
        }
        if (attr_ops->post_get != NULL) {
            status = (attr_ops->post_get)(client, attr_data, data);
            if (status != 0)
                pddf_info(XCVR, "%s[%d]: %s fails for %s attribute. ret %d\n",
                          __FUNCTION__, __LINE__, dev_name(&client->dev), attr_data->aname, status);
        }
        mutex_unlock(&data->update_lock);

        return sprintf(buf, "%d\n", led_state);
    }
    else {
        return sprintf(buf, "%s", "");
    }
}

static ssize_t xcvr_red_led_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    int idx, status;
    uint32_t set_value;

    idx = get_xcvr_module_attr_data(client, dev, da);
    if (idx < 0 || !pdata)
        return -EINVAL;

    if (kstrtoint(buf, 10, &set_value) || (set_value != 0 && set_value != 1))
        return -EINVAL;

    mutex_lock(&data->update_lock);
    status = xcvr_red_led_do_set(client, &pdata->xcvr_attrs[idx], data, set_value);
    mutex_unlock(&data->update_lock);

    return status < 0 ? status : count;
}

static int xcvr_red_led_en_do_get(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status;
    uint32_t red_led_en;

    if (strcmp(info->devtype, "fpgapci") != 0) {
        pddf_err(XCVR, "Unsupported device type for red LED enable: %s (only fpgapci allowed)\n", info->devtype);
        return -EINVAL;
    }

    status = xcvr_fpgapci_read(info);
    // 不判断status<0（避免0xFFFFFFFF误判为错误）
    red_led_en = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
    
    pddf_dbg(XCVR, "Red LED Enable: 0x%x, reg_value=0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x, cmp=0x%x\n",
             red_led_en, status, info->devaddr, info->mask, info->offset, info->cmpval);

    return red_led_en;
}

static int xcvr_red_led_en_do_set(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data, int value)
{
    int status;

    if (strcmp(info->devtype, "fpgapci") != 0) {
        pddf_err(XCVR, "Unsupported device type to set red LED enable: %s (only fpgapci allowed)\n", info->devtype);
        return -EINVAL;
    }

    status = xcvr_fpgapci_write(info, value);
    if (status < 0) {
        pddf_err(XCVR, "Failed to set red LED enable: status=%d\n", status);
    }

    return status;
}

static ssize_t xcvr_red_led_en_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;
    int idx, status = 0;
    int en_state = 0;

    idx = get_xcvr_module_attr_data(client, dev, da);
    if (idx >= 0)
        attr_data = &pdata->xcvr_attrs[idx];
    
    if (attr_data != NULL) {
        attr_ops = &xcvr_ops[attr->index];

        mutex_lock(&data->update_lock);
        if (attr_ops->pre_get != NULL) {
            status = (attr_ops->pre_get)(client, attr_data, data);
            if (status != 0)
                pddf_info(XCVR, "%s[%d]: %s fails for %s attribute. ret %d\n",
                          __FUNCTION__, __LINE__, dev_name(&client->dev), attr_data->aname, status);
        } 
        if (attr_ops->do_get != NULL) {
            en_state = (attr_ops->do_get)(client, attr_data, data);
            if (en_state < 0) {
                pddf_info(XCVR, "%s[%d]: %s fails for %s attribute. ret %d\n",
                          __FUNCTION__, __LINE__, dev_name(&client->dev), attr_data->aname, en_state);
                en_state = 0;
            }
        }
        if (attr_ops->post_get != NULL) {
            status = (attr_ops->post_get)(client, attr_data, data);
            if (status != 0)
                pddf_info(XCVR, "%s[%d]: %s fails for %s attribute. ret %d\n",
                          __FUNCTION__, __LINE__, dev_name(&client->dev), attr_data->aname, status);
        }
        mutex_unlock(&data->update_lock);

        return sprintf(buf, "%d\n", en_state);
    }
    else {
        return sprintf(buf, "%s", "");
    }
}

static ssize_t xcvr_red_led_en_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    int idx, status;
    uint32_t set_value;

    idx = get_xcvr_module_attr_data(client, dev, da);
    if (idx < 0 || !pdata)
        return -EINVAL;

    if (kstrtoint(buf, 10, &set_value) || (set_value != 0 && set_value != 1))
        return -EINVAL;

    mutex_lock(&data->update_lock);
    status = xcvr_red_led_en_do_set(client, &pdata->xcvr_attrs[idx], data, set_value);
    mutex_unlock(&data->update_lock);

    return status < 0 ? status : count;
}

static int __init pddf_custom_xcvr_init(void)
{
    pddf_info(XCVR, "pddf_custom_xcvr module loading\n");

    xcvr_ops[XCVR_LED_EN].show = xcvr_red_led_en_show;
    xcvr_ops[XCVR_LED_EN].do_get = xcvr_red_led_en_do_get;
    xcvr_ops[XCVR_LED_EN].store = xcvr_red_led_en_store;
    xcvr_ops[XCVR_LED_EN].do_set = NULL;

    xcvr_ops[XCVR_RED_LED].show = xcvr_red_led_show;
    xcvr_ops[XCVR_RED_LED].do_get = xcvr_red_led_do_get;
    xcvr_ops[XCVR_RED_LED].store = xcvr_red_led_store;
    xcvr_ops[XCVR_RED_LED].do_set = NULL;

    sensor_dev_attr_xcvr_led_en.dev_attr.show = xcvr_red_led_en_show;
    sensor_dev_attr_xcvr_led_en.dev_attr.store = xcvr_red_led_en_store;

    sensor_dev_attr_xcvr_red_led.dev_attr.show = xcvr_red_led_show;
    sensor_dev_attr_xcvr_red_led.dev_attr.store = xcvr_red_led_store;

    pddf_info(XCVR, "Successfully replaced handlers\n");
    return 0;
}

static void __exit pddf_custom_xcvr_exit(void)
{
    pddf_info(XCVR, "pddf_custom_xcvr module exit..\n");
    return ;
}

module_init(pddf_custom_xcvr_init);
module_exit(pddf_custom_xcvr_exit);

MODULE_AUTHOR("FLKS");
MODULE_DESCRIPTION("PDDF custom XCVR handler");
MODULE_LICENSE("GPL");