/*
 * A CPLD driver for the fn8032_bnf
 *
 * Copyright (C) 2018 Pegatron Corporation.
 * Peter5_Lin <Peter5_Lin@pegatroncorp.com.tw>
 *
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

#undef pegatron_fn8032_bnf_DEBUG
#ifdef pegatron_fn8032_bnf_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#define CPLD_SFP_MAX_GROUP          3
#define SFP_PORT_MAX_NUM            32
#define SFP_EEPROM_SIZE             256
#define QSFP_FIRST_PORT             48
#define CPLDA_SFP_NUM               0
#define CPLDB_SFP_NUM               16
#define CPLDC_SFP_NUM               16
#define CPLDA_ADDRESS               0x74
#define CPLDB_ADDRESS               0x75
#define CPLDC_ADDRESS               0x76
#define CPLDD_ADDRESS               0x18
#define LM75B_ADDRESS               0x4a
#define LM75B_TEMP_REG 	            0x0
#define CPLD_VERSION_REG            0x0
#define SYNC_CONTROL_REG            0x50 /* maurice note for CPLD_B register (MCR : 0x01) */
#define SYNC_CONTROL_2_REG          0x2 /* maurice add for CPLD_C register (MCR : 0x02) */
#define CPLD_SYS_PWR_LED_REG        0x31
#define CPLD_LOC_FAN_LED_REG        0x32
#define CPLD_EEPROM_WRITE_REG       0x1
#define CPLD_PSU_REG                0x11
#define SFP_13_40_SCL_BASE          0x4
#define SFP_1_12_SCL_BASE           0x2
#define SFP_41_56_SCL_BASE          0x5
#define SFP_STATUS_BASE             0x44
#define SFP_RST_BASE                0x48
#define QSFP_PRESENT_ADDRESS        0x44
#define QSFP_RESET_ADDRESS_BASE     0x48
#define QSFP_MODSELN_ADDRESS        0x42
#define QSFP_LOW_POWER_ADDRESS      0x46
#define CPLD_SERIAL_LED_BIT         2 /* maurice for SLED_EN */
#define SYNC_CONTROL_REG_0          0x32
#define SYNC_CONTROL_REG_1          0x31
#define CPLD_EEPROM_WRITE_BIT       1
#define SFP_PRESENT_BASE            0
#define SFP_RXLOSS_BASE             1
#define SFP_TXFAULT_BASE            2
#define SFP_TXDISABLE_BASE          3
#define CPLD_PSU_PWOK_BASE          4
#define CPLD_PSU_PRESENT_BASE       0
#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

static LIST_HEAD(cpld_client_list);
static struct mutex  list_lock;
/* Addresses scanned for pegatron_fn8032_bnf_cpld
 */
static const unsigned short normal_i2c[] = { CPLDA_ADDRESS, CPLDB_ADDRESS, CPLDC_ADDRESS, CPLDD_ADDRESS, LM75B_ADDRESS, I2C_CLIENT_END };

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

int pegatron_fn8032_bnf_cpld_read(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int data = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == addr) {
            data = i2c_smbus_read_byte_data(cpld_node->client, reg);
            DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, addr, reg, data));
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return data;
}
EXPORT_SYMBOL(pegatron_fn8032_bnf_cpld_read);


int pegatron_fn8032_bnf_cpld_write(unsigned short addr, u8 reg, u8 val)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == addr) {
            ret = i2c_smbus_write_byte_data(cpld_node->client, reg, val);
             DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, addr, reg, val));
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(pegatron_fn8032_bnf_cpld_write);

static ssize_t read_cpld_HWversion(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_VERSION_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%02x\n", (data >> 5) & 0x7);
}

static ssize_t read_cpld_SWversion(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_VERSION_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%02x\n", (data & 0x1f));
}

/* maurice note CPLD_B Register for Port LED */
static ssize_t show_allled_ctrl(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = SYNC_CONTROL_REG_0;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0xF;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_allled_ctrl(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = SYNC_CONTROL_REG_0;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    data = val | (data & 0xf0);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t show_serial_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = SYNC_CONTROL_REG_1;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, CPLD_SERIAL_LED_BIT, val);

    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_serial_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = SYNC_CONTROL_REG_1;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, CPLD_SERIAL_LED_BIT);
    else
        CLEAR_BIT(data, CPLD_SERIAL_LED_BIT);
    
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

/* maurice add CPLD_C Register for Port LED */
static ssize_t show_allled_ctrl_2(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = SYNC_CONTROL_2_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_allled_ctrl_2(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = SYNC_CONTROL_2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    data = val | (data & 0xfc);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t show_serial_led_2(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = SYNC_CONTROL_2_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, CPLD_SERIAL_LED_BIT, val);

    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_serial_led_2(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = SYNC_CONTROL_2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, CPLD_SERIAL_LED_BIT);
    else
        CLEAR_BIT(data, CPLD_SERIAL_LED_BIT);
    
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}
/* maurice add end */

static ssize_t show_sys_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_SYS_PWR_LED_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> 5) & 0x7;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_sys_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_SYS_PWR_LED_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    data = (val << 5) | (data & 0x1f);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}
static ssize_t show_pwr_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_SYS_PWR_LED_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> 2) & 0x7;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_pwr_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_SYS_PWR_LED_REG;
    long val = 0;


    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    data = (val << 2) | (data & 0xe3);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}
static ssize_t show_loc_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_SYS_PWR_LED_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data) & 0x3;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_loc_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_SYS_PWR_LED_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    data = (val) | (data & 0xFC);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t show_fan_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_LOC_FAN_LED_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x7;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_fan_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_LOC_FAN_LED_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    data = val | (data & 0xf8);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t show_eeprom_write_enable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = CPLD_EEPROM_WRITE_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, reg, val);

    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_eeprom_write_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_EEPROM_WRITE_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, CPLD_EEPROM_WRITE_BIT);
    else
        CLEAR_BIT(data, CPLD_EEPROM_WRITE_BIT);
    
    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t read_psu_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = CPLD_PSU_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (CPLD_PSU_PRESENT_BASE + attr->index), val);

    return sprintf(buf, "%02x\n", val);
}

static ssize_t read_psu_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val=0, reg = CPLD_PSU_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (CPLD_PSU_PWOK_BASE + attr->index), val);

    return sprintf(buf, "%02x\n", val);
}

#define GET_SFP_STATUS_ADDRESS(idx, reg) \
            reg = SFP_STATUS_BASE + ((idx % 16) / 8)

#define GET_SFP_RST_ADDRESS(idx, reg) \
            reg = SFP_RST_BASE + ((idx % 16) / 4)


static ssize_t get_sfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0, val = 0;

    GET_SFP_STATUS_ADDRESS(attr->index, reg);
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, SFP_PRESENT_BASE + (attr->index % 8), val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_sfp_tx_disable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0, val = 0;

    GET_SFP_RST_ADDRESS(attr->index, reg);

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, SFP_TXDISABLE_BASE + (attr->index % 4)*2, val);

    return sprintf(buf, "%d\n", val);
}
static ssize_t set_sfp_tx_disable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    GET_SFP_RST_ADDRESS(attr->index, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);

    if(val)
        SET_BIT(data, SFP_TXDISABLE_BASE + 2*(attr->index % 4));
    else
        CLEAR_BIT(data, SFP_TXDISABLE_BASE + 2*(attr->index % 4));

    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count; 
}

//TODO FIXME FALU {
static ssize_t get_sfp_rx_loss(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0, val = 0;

    GET_SFP_STATUS_ADDRESS(attr->index, reg);
    
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, SFP_RXLOSS_BASE + 4*(attr->index % 2), val);

    return sprintf(buf, "%d\n", val);
}
static ssize_t get_sfp_tx_fault(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0, val = 0;

    GET_SFP_STATUS_ADDRESS(attr->index, reg);
    
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, SFP_TXFAULT_BASE + 4*(attr->index % 2), val);

    return sprintf(buf, "%d\n",val);
}

//TODO FIXME FALU }
#define GET_QSFP_STATUS_ADDRESS(idx, reg) \
            reg = QSFP_PRESENT_ADDRESS + ((idx % 16) / 8)

#define GET_QSFP_RST_ADDRESS(idx, reg) \
            reg = QSFP_RESET_ADDRESS_BASE + ((idx % 16) / 4)

#define GET_QSFP_LOWPOWER_ADDRESS(idx, reg) \
            reg = QSFP_LOW_POWER_ADDRESS + ((idx % 16) / 8)

#define GET_QSFP_MODSELN_ADDRESS(idx, reg) \
            reg = QSFP_MODSELN_ADDRESS + ((idx % 16) / 8)



static ssize_t get_qsfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg;

    GET_QSFP_STATUS_ADDRESS(attr->index, reg);
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, SFP_PRESENT_BASE + (attr->index % 8), val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_qsfp_reset(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg, data =0;

    GET_QSFP_RST_ADDRESS(attr->index, reg);
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> ((attr->index % 4)*2)) & 0x3;

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_qsfp_reset(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg, data = 0;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    GET_QSFP_RST_ADDRESS(attr->index, reg);
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    CLEAR_BIT(data, (attr->index % 4)*2);
    CLEAR_BIT(data, (attr->index % 4)*2+1);
    data |= (val & 0x3) << ((attr->index % 4)*2);

    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t get_qsfp_lowpower(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg;

    GET_QSFP_LOWPOWER_ADDRESS(attr->index, reg);
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (attr->index % 8), val);
    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_qsfp_lowpower(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    GET_QSFP_LOWPOWER_ADDRESS(attr->index, reg);
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, (attr->index % 8));
    else
        CLEAR_BIT(data, (attr->index % 8));

    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t get_qsfp_modeseln(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = QSFP_MODSELN_ADDRESS;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (attr->index % QSFP_FIRST_PORT), val);
    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_qsfp_modeseln(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = QSFP_MODSELN_ADDRESS;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, (attr->index % QSFP_FIRST_PORT));
    else
        CLEAR_BIT(data, (attr->index % QSFP_FIRST_PORT));

    pegatron_fn8032_bnf_cpld_write(client->addr, reg, data);

    return count;
}

static SENSOR_DEVICE_ATTR(cpld_hw_version,  S_IRUGO, read_cpld_HWversion, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_sw_version,  S_IRUGO, read_cpld_SWversion, NULL, 0);

/* maurice note CPLD_B Register for Port LED */
static SENSOR_DEVICE_ATTR(cpld_allled_ctrl,  S_IRUGO | S_IWUSR, show_allled_ctrl, set_allled_ctrl, 0);
static SENSOR_DEVICE_ATTR(serial_led_enable,  S_IRUGO | S_IWUSR, show_serial_led, set_serial_led, 0);
/* maurice add CPLD_C Register for Port LED */
static SENSOR_DEVICE_ATTR(cpld_allled_ctrl_2,  S_IRUGO | S_IWUSR, show_allled_ctrl_2, set_allled_ctrl_2, 0);
static SENSOR_DEVICE_ATTR(serial_led_enable_2,  S_IRUGO | S_IWUSR, show_serial_led_2, set_serial_led_2, 0);

static SENSOR_DEVICE_ATTR(sys_led,  S_IRUGO | S_IWUSR, show_sys_led, set_sys_led, 0);
static SENSOR_DEVICE_ATTR(pwr_led,  S_IRUGO | S_IWUSR, show_pwr_led, set_pwr_led, 0);
static SENSOR_DEVICE_ATTR(loc_led,  S_IRUGO | S_IWUSR, show_loc_led, set_loc_led, 0);
static SENSOR_DEVICE_ATTR(fan_led,  S_IRUGO | S_IWUSR, show_fan_led, set_fan_led, 0);
static SENSOR_DEVICE_ATTR(eeprom_write_enable,  S_IRUGO | S_IWUSR, show_eeprom_write_enable, set_eeprom_write_enable, 0);
static SENSOR_DEVICE_ATTR(psu_1_present,  S_IRUGO, read_psu_present, NULL, 1);
static SENSOR_DEVICE_ATTR(psu_2_present,  S_IRUGO, read_psu_present, NULL, 0);
static SENSOR_DEVICE_ATTR(psu_1_status,  S_IRUGO, read_psu_status, NULL, 1);
static SENSOR_DEVICE_ATTR(psu_2_status,  S_IRUGO, read_psu_status, NULL, 0);

#define SET_SFP_ATTR(_num)  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_present,  S_IRUGO, get_sfp_present, NULL, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_tx_disable,  S_IRUGO | S_IWUSR, get_sfp_tx_disable, set_sfp_tx_disable, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_rx_loss,  S_IRUGO, get_sfp_rx_loss, NULL, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_tx_fault,  S_IRUGO, get_sfp_tx_fault, NULL, _num-1) 

#define SET_QSFP_ATTR(_num) \
        static SENSOR_DEVICE_ATTR(sfp##_num##_present,  S_IRUGO, get_qsfp_present, NULL, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_reset,  S_IRUGO | S_IWUSR, get_qsfp_reset, set_qsfp_reset, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_lowpower,  S_IRUGO | S_IWUSR, get_qsfp_lowpower, set_qsfp_lowpower, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_modeseln,  S_IRUGO | S_IWUSR, get_qsfp_modeseln, set_qsfp_modeseln, _num-1)
SET_QSFP_ATTR(1);SET_QSFP_ATTR(2);SET_QSFP_ATTR(3);SET_QSFP_ATTR(4);SET_QSFP_ATTR(5);SET_QSFP_ATTR(6);SET_QSFP_ATTR(7);SET_QSFP_ATTR(8);SET_QSFP_ATTR(9);
SET_QSFP_ATTR(10);SET_QSFP_ATTR(11);SET_QSFP_ATTR(12);SET_QSFP_ATTR(13);SET_QSFP_ATTR(14);SET_QSFP_ATTR(15);SET_QSFP_ATTR(16);SET_QSFP_ATTR(17);SET_QSFP_ATTR(18);
SET_QSFP_ATTR(19);SET_QSFP_ATTR(20);SET_QSFP_ATTR(21);SET_QSFP_ATTR(22);SET_QSFP_ATTR(23);SET_QSFP_ATTR(24);SET_QSFP_ATTR(25);SET_QSFP_ATTR(26);SET_QSFP_ATTR(27);
SET_QSFP_ATTR(28);SET_QSFP_ATTR(29);SET_QSFP_ATTR(30);SET_QSFP_ATTR(31);SET_QSFP_ATTR(32);SET_SFP_ATTR(33);SET_SFP_ATTR(34);SET_SFP_ATTR(35);SET_SFP_ATTR(36);
SET_SFP_ATTR(37);SET_SFP_ATTR(38);SET_SFP_ATTR(39);SET_SFP_ATTR(40);SET_SFP_ATTR(41);SET_SFP_ATTR(42);SET_SFP_ATTR(43);SET_SFP_ATTR(44);SET_SFP_ATTR(45);
SET_SFP_ATTR(46);SET_SFP_ATTR(47);SET_SFP_ATTR(48);
SET_QSFP_ATTR(49);SET_QSFP_ATTR(50);SET_QSFP_ATTR(51);SET_QSFP_ATTR(52);SET_QSFP_ATTR(53);SET_QSFP_ATTR(54);
SET_QSFP_ATTR(55);SET_QSFP_ATTR(56); /* maurice add sfp55 ,sfp56 node*/

static struct attribute *pegatron_fn8032_bnf_cpldA_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,
    &sensor_dev_attr_sys_led.dev_attr.attr,
    &sensor_dev_attr_pwr_led.dev_attr.attr,
    &sensor_dev_attr_loc_led.dev_attr.attr,
    &sensor_dev_attr_fan_led.dev_attr.attr,
    &sensor_dev_attr_eeprom_write_enable.dev_attr.attr,

    &sensor_dev_attr_psu_1_present.dev_attr.attr,
    &sensor_dev_attr_psu_2_present.dev_attr.attr,
    &sensor_dev_attr_psu_1_status.dev_attr.attr,
    &sensor_dev_attr_psu_2_status.dev_attr.attr,
    NULL
};

static struct attribute *pegatron_fn8032_bnf_cpldB_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_allled_ctrl.dev_attr.attr, /* maurice note CPLD_B Register for Port LED */
    &sensor_dev_attr_serial_led_enable.dev_attr.attr, /* maurice note CPLD_B Register for Port LED */

    &sensor_dev_attr_sfp1_present.dev_attr.attr,
    &sensor_dev_attr_sfp1_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp1_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp1_reset.dev_attr.attr,

    &sensor_dev_attr_sfp2_present.dev_attr.attr,
    &sensor_dev_attr_sfp2_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp2_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp2_reset.dev_attr.attr,

    &sensor_dev_attr_sfp3_present.dev_attr.attr,
    &sensor_dev_attr_sfp3_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp3_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp3_reset.dev_attr.attr,

    &sensor_dev_attr_sfp4_present.dev_attr.attr,
    &sensor_dev_attr_sfp4_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp4_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp4_reset.dev_attr.attr,

    &sensor_dev_attr_sfp5_present.dev_attr.attr, /* maurice add sfp5, sfp6 node*/
    &sensor_dev_attr_sfp5_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp5_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp5_reset.dev_attr.attr,

    &sensor_dev_attr_sfp6_present.dev_attr.attr,
    &sensor_dev_attr_sfp6_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp6_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp6_reset.dev_attr.attr,

    &sensor_dev_attr_sfp7_present.dev_attr.attr,
    &sensor_dev_attr_sfp7_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp7_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp7_reset.dev_attr.attr,

    &sensor_dev_attr_sfp8_present.dev_attr.attr,
    &sensor_dev_attr_sfp8_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp8_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp8_reset.dev_attr.attr,

    &sensor_dev_attr_sfp9_present.dev_attr.attr,
    &sensor_dev_attr_sfp9_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp9_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp9_reset.dev_attr.attr,

    &sensor_dev_attr_sfp10_present.dev_attr.attr,
    &sensor_dev_attr_sfp10_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp10_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp10_reset.dev_attr.attr,
 
    &sensor_dev_attr_sfp11_present.dev_attr.attr,
    &sensor_dev_attr_sfp11_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp11_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp11_reset.dev_attr.attr,

    &sensor_dev_attr_sfp12_present.dev_attr.attr,
    &sensor_dev_attr_sfp12_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp12_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp12_reset.dev_attr.attr,

    &sensor_dev_attr_sfp13_present.dev_attr.attr,
    &sensor_dev_attr_sfp13_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp13_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp13_reset.dev_attr.attr,

    &sensor_dev_attr_sfp14_present.dev_attr.attr,
    &sensor_dev_attr_sfp14_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp14_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp14_reset.dev_attr.attr,

    &sensor_dev_attr_sfp15_present.dev_attr.attr, /* maurice add sfp15, sfp16 node*/
    &sensor_dev_attr_sfp15_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp15_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp15_reset.dev_attr.attr,

    &sensor_dev_attr_sfp16_present.dev_attr.attr,
    &sensor_dev_attr_sfp16_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp16_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp16_reset.dev_attr.attr,

    NULL
};

static struct attribute *pegatron_fn8032_bnf_cpldC_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_allled_ctrl.dev_attr.attr, /* maurice note CPLD_A Register for Port LED */
    &sensor_dev_attr_serial_led_enable.dev_attr.attr, /* maurice note CPLD_A Register for Port LED */

    /* maurice remove 17-32 from cpldC */
    &sensor_dev_attr_sfp17_present.dev_attr.attr,
    &sensor_dev_attr_sfp17_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp17_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp17_reset.dev_attr.attr,

    &sensor_dev_attr_sfp18_present.dev_attr.attr,
    &sensor_dev_attr_sfp18_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp18_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp18_reset.dev_attr.attr,

    &sensor_dev_attr_sfp19_present.dev_attr.attr,
    &sensor_dev_attr_sfp19_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp19_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp19_reset.dev_attr.attr,

    &sensor_dev_attr_sfp20_present.dev_attr.attr,
    &sensor_dev_attr_sfp20_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp20_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp20_reset.dev_attr.attr,

    &sensor_dev_attr_sfp21_present.dev_attr.attr,
    &sensor_dev_attr_sfp21_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp21_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp21_reset.dev_attr.attr,

    &sensor_dev_attr_sfp22_present.dev_attr.attr,
    &sensor_dev_attr_sfp22_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp22_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp22_reset.dev_attr.attr,

    &sensor_dev_attr_sfp23_present.dev_attr.attr,
    &sensor_dev_attr_sfp23_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp23_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp23_reset.dev_attr.attr,

    &sensor_dev_attr_sfp24_present.dev_attr.attr,
    &sensor_dev_attr_sfp24_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp24_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp24_reset.dev_attr.attr,

    &sensor_dev_attr_sfp25_present.dev_attr.attr, /* maurice add sfp25, sfp26 node*/
    &sensor_dev_attr_sfp25_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp25_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp25_reset.dev_attr.attr,

    &sensor_dev_attr_sfp26_present.dev_attr.attr,
    &sensor_dev_attr_sfp26_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp26_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp26_reset.dev_attr.attr,

    &sensor_dev_attr_sfp27_present.dev_attr.attr,
    &sensor_dev_attr_sfp27_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp27_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp27_reset.dev_attr.attr,

    &sensor_dev_attr_sfp28_present.dev_attr.attr,
    &sensor_dev_attr_sfp28_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp28_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp28_reset.dev_attr.attr,

    &sensor_dev_attr_sfp29_present.dev_attr.attr,
    &sensor_dev_attr_sfp29_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp29_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp29_reset.dev_attr.attr,

    &sensor_dev_attr_sfp30_present.dev_attr.attr,
    &sensor_dev_attr_sfp30_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp30_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp30_reset.dev_attr.attr,

    &sensor_dev_attr_sfp31_present.dev_attr.attr,
    &sensor_dev_attr_sfp31_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp31_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp31_reset.dev_attr.attr,

    &sensor_dev_attr_sfp32_present.dev_attr.attr,
    &sensor_dev_attr_sfp32_lowpower.dev_attr.attr,
    &sensor_dev_attr_sfp32_modeseln.dev_attr.attr,
    &sensor_dev_attr_sfp32_reset.dev_attr.attr,

    NULL
};

static struct attribute *pegatron_fn8032_bnf_cpldD_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,
	NULL
};

static ssize_t get_lm75b_temp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev); 
    u16 data = 0;  
    int exponent, mantissa;
    int multiplier = 1;
    u8 reg = LM75B_TEMP_REG;

    data = pegatron_fn8032_bnf_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT"%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
	sprintf(buf, "%d\n", data);
	return strlen(buf);

}

static SENSOR_DEVICE_ATTR(lm75b_temp,  S_IRUGO, get_lm75b_temp, NULL, 0);

static struct attribute *pegatron_fn8032_bnf_lm75b_attributes[] = {
	&sensor_dev_attr_lm75b_temp.dev_attr.attr,
	NULL
};

static const struct attribute_group pegatron_fn8032_bnf_cpldA_group = { .attrs = pegatron_fn8032_bnf_cpldA_attributes};
static const struct attribute_group pegatron_fn8032_bnf_cpldB_group = { .attrs = pegatron_fn8032_bnf_cpldB_attributes};
static const struct attribute_group pegatron_fn8032_bnf_cpldC_group = { .attrs = pegatron_fn8032_bnf_cpldC_attributes};
static const struct attribute_group pegatron_fn8032_bnf_cpldD_group = { .attrs = pegatron_fn8032_bnf_cpldD_attributes};
static const struct attribute_group pegatron_fn8032_bnf_lm75b_group = { .attrs = pegatron_fn8032_bnf_lm75b_attributes};

static void pegatron_fn8032_bnf_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);
    
    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }
    
    node->client = client;
    
    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void pegatron_fn8032_bnf_cpld_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }
    
    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }
    
    mutex_unlock(&list_lock);
}

static int pegatron_fn8032_bnf_cpld_probe(struct i2c_client *client,
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
        case CPLDA_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8032_bnf_cpldA_group);
            break;
        case CPLDB_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8032_bnf_cpldB_group);
            break;
        case CPLDC_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8032_bnf_cpldC_group);
            break;
		case CPLDD_ADDRESS:	
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8032_bnf_cpldD_group);
            break;
		case LM75B_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &pegatron_fn8032_bnf_lm75b_group);
			break;
        default:
            dev_dbg(&client->dev, "i2c_check_CPLD failed (0x%x)\n", client->addr);
            status = -EIO;
            goto exit;
            break;
    }

    if (status) {
        goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    pegatron_fn8032_bnf_cpld_add_client(client);
    
    return 0; 

exit:
    return status;
}

static int pegatron_fn8032_bnf_cpld_remove(struct i2c_client *client)
{
    switch(client->addr)
    {
        case CPLDA_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8032_bnf_cpldA_group);
            break;
        case CPLDB_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8032_bnf_cpldB_group);
            break;
        case CPLDC_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8032_bnf_cpldC_group);
            break;
		case CPLDD_ADDRESS:	
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8032_bnf_cpldD_group);
            break;
		case LM75B_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &pegatron_fn8032_bnf_lm75b_group);
			break;
        default:
            dev_dbg(&client->dev, "i2c_remove_CPLD failed (0x%x)\n", client->addr);
            break;
    }

  
    pegatron_fn8032_bnf_cpld_remove_client(client);  
    return 0;
}

static const struct i2c_device_id pegatron_fn8032_bnf_cpld_id[] = {
    { "fn8032_bnf_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, pegatron_fn8032_bnf_cpld_id);

static struct i2c_driver pegatron_fn8032_bnf_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "pegatron_fn8032_bnf_cpld",
    },
    .probe      = pegatron_fn8032_bnf_cpld_probe,
    .remove     = pegatron_fn8032_bnf_cpld_remove,
    .id_table   = pegatron_fn8032_bnf_cpld_id,
    .address_list = normal_i2c,
};

static int __init pegatron_fn8032_bnf_cpld_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&pegatron_fn8032_bnf_cpld_driver);
}

static void __exit pegatron_fn8032_bnf_cpld_exit(void)
{
    i2c_del_driver(&pegatron_fn8032_bnf_cpld_driver);
}

MODULE_AUTHOR("Peter5 Lin <Peter5_Lin@pegatroncorp.com.tw>");
MODULE_DESCRIPTION("pegatron_fn8032_bnf_cpld driver");
MODULE_LICENSE("GPL");

module_init(pegatron_fn8032_bnf_cpld_init);
module_exit(pegatron_fn8032_bnf_cpld_exit);
