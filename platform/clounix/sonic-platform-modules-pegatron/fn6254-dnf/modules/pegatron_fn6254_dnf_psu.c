/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>

#undef PEGA_DEBUG
/*#define pega_DEBUG*/
#ifdef PEGA_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#define PSU_58_ADDRESS 0x58
#define PSU_59_ADDRESS 0x59
#define PSU_VOUT_REG 0x7A
#define PSU_IOUT_REG 0x7B
#define PSU_IPUT_REG 0x7C
#define PSU_TEMP_REG 0x7D
#define PSU_VOUT_OVER_VOLTAGE_BIT 7
#define PSU_IOUT_OVER_CURRENT_FAULT_BIT 7
#define PSU_IOUT_OVER_CURRENT_WARNING_BIT 5
#define PSU_IPUT_OVER_CURRENT_WARNING_BIT 1
#define PSU_IPUT_INSUFFICIENT_BIT 3
#define PSU_TEMP_OVER_TEMP_FAULT_BIT 7
#define PSU_TEMP_OVER_TEMP_WARNING_BIT 6

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

struct psu_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

static const unsigned short normal_i2c[] = { PSU_58_ADDRESS, PSU_59_ADDRESS, I2C_CLIENT_END };
static LIST_HEAD(psu_client_list);
static struct mutex  list_lock;

static int pega_fn_6254_dn_f_psu_read(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct psu_client_node *psu_node = NULL;
    int data = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &psu_client_list)
    {
        psu_node = list_entry(list_node, struct psu_client_node, list);
        
        if (psu_node->client->addr == addr) {
            data = i2c_smbus_read_byte_data(psu_node->client, reg);
            DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, addr, reg, data));
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return data;
}

static ssize_t read_psu_vout_over_voltage(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = PSU_VOUT_REG, val = 0;

    data = pega_fn_6254_dn_f_psu_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, PSU_VOUT_OVER_VOLTAGE_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_iout_over_current_fault(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = PSU_IOUT_REG, val = 0;

    data = pega_fn_6254_dn_f_psu_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, PSU_IOUT_OVER_CURRENT_FAULT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_iout_over_current_warning(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = PSU_IOUT_REG, val = 0;

    data = pega_fn_6254_dn_f_psu_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, PSU_IOUT_OVER_CURRENT_WARNING_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_iput_over_current_warning(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = PSU_IPUT_REG, val = 0;

    data = pega_fn_6254_dn_f_psu_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, PSU_IPUT_OVER_CURRENT_WARNING_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_iput_insufficient(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = PSU_IPUT_REG, val = 0;

    data = pega_fn_6254_dn_f_psu_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, PSU_IPUT_INSUFFICIENT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_temp_over_temp_fault(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = PSU_TEMP_REG, val = 0;

    data = pega_fn_6254_dn_f_psu_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, PSU_TEMP_OVER_TEMP_FAULT_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t read_psu_temp_over_temp_warning(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = PSU_TEMP_REG, val = 0;

    data = pega_fn_6254_dn_f_psu_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, PSU_TEMP_OVER_TEMP_WARNING_BIT, val);

    return sprintf(buf, "%d\n", val);
}

static SENSOR_DEVICE_ATTR(vout_over_voltage,  S_IRUGO, read_psu_vout_over_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(iout_over_current_fault,  S_IRUGO, read_psu_iout_over_current_fault, NULL, 0);
static SENSOR_DEVICE_ATTR(iout_over_current_warning,  S_IRUGO, read_psu_iout_over_current_warning, NULL, 0);
static SENSOR_DEVICE_ATTR(iput_over_current_warning,  S_IRUGO, read_psu_iput_over_current_warning, NULL, 0);
static SENSOR_DEVICE_ATTR(iput_insufficient,  S_IRUGO, read_psu_iput_insufficient, NULL, 0);
static SENSOR_DEVICE_ATTR(temp_over_temp_fault,  S_IRUGO, read_psu_temp_over_temp_fault, NULL, 0);
static SENSOR_DEVICE_ATTR(temp_over_temp_warning,  S_IRUGO, read_psu_temp_over_temp_warning, NULL, 0);

static struct attribute *pega_fn_6254_dn_f_psu_attributes[] = {
    &sensor_dev_attr_vout_over_voltage.dev_attr.attr,
    &sensor_dev_attr_iout_over_current_fault.dev_attr.attr,
    &sensor_dev_attr_iout_over_current_warning.dev_attr.attr,
    &sensor_dev_attr_iput_over_current_warning.dev_attr.attr,
    &sensor_dev_attr_iput_insufficient.dev_attr.attr,
    &sensor_dev_attr_temp_over_temp_fault.dev_attr.attr,
    &sensor_dev_attr_temp_over_temp_warning.dev_attr.attr,
    NULL
};

static const struct attribute_group pega_fn_6254_dn_f_psu_group = { .attrs = pega_fn_6254_dn_f_psu_attributes};

static void pega_fn_6254_dn_f_psu_add_client(struct i2c_client *client)
{
    struct psu_client_node *node = kzalloc(sizeof(struct psu_client_node), GFP_KERNEL);
    
    if (!node) {
        dev_dbg(&client->dev, "Can't allocate psu_client_node (0x%x)\n", client->addr);
        return;
    }
    
    node->client = client;
    
    mutex_lock(&list_lock);
    list_add(&node->list, &psu_client_list);
    mutex_unlock(&list_lock);
}

static void pega_fn_6254_dn_f_psu_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
    struct psu_client_node *psu_node = NULL;
    int found = 0;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &psu_client_list)
    {
        psu_node = list_entry(list_node, struct psu_client_node, list);
        
        if (psu_node->client == client) {
            found = 1;
            break;
        }
    }
    
    if (found) {
        list_del(list_node);
        kfree(psu_node);
    }
    
    mutex_unlock(&list_lock);
}

static int pega_fn_6254_dn_f_psu_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{  
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    /* Register sysfs hooks */
    switch(client->addr)
    {
        case PSU_58_ADDRESS:
        case PSU_59_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pega_fn_6254_dn_f_psu_group);
            break;
        default:
            dev_dbg(&client->dev, "i2c_check_psu failed (0x%x)\n", client->addr);
            status = -EIO;
            goto exit;
            break;
    }

    if (status) {
        goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    pega_fn_6254_dn_f_psu_add_client(client);
    
    return 0; 

exit:
    return status;
}

static int pega_fn_6254_dn_f_psu_remove(struct i2c_client *client)
{
    switch(client->addr)
    {
        case PSU_58_ADDRESS:
        case PSU_59_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pega_fn_6254_dn_f_psu_group);
            break;
        default:
            dev_dbg(&client->dev, "i2c_remove_psu failed (0x%x)\n", client->addr);
            break;
    }
  
    pega_fn_6254_dn_f_psu_remove_client(client);  
    return 0;
}

static const struct i2c_device_id pega_fn_6254_dn_f_psu_id[] = {
    { "fn_6254_dn_f_psu", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, pega_fn_6254_dn_f_psu_id);

static struct i2c_driver pega_fn_6254_dn_f_psu_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "pegatron_fn_6254_dn_f_psu",
    },
    .probe      = pega_fn_6254_dn_f_psu_probe,
    .remove     = pega_fn_6254_dn_f_psu_remove,
    .id_table   = pega_fn_6254_dn_f_psu_id,
    .address_list = normal_i2c,
};

static int __init pega_fn_6254_dn_f_psu_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&pega_fn_6254_dn_f_psu_driver);
}

static void __exit pega_fn_6254_dn_f_psu_exit(void)
{
    i2c_del_driver(&pega_fn_6254_dn_f_psu_driver);
}

MODULE_AUTHOR("Peter5 Lin <Peter5_Lin@pegatroncorp.com.tw>");
MODULE_DESCRIPTION("pega_fn_6254_dn_f_psu driver");
MODULE_LICENSE("GPL");

module_init(pega_fn_6254_dn_f_psu_init);
module_exit(pega_fn_6254_dn_f_psu_exit);
