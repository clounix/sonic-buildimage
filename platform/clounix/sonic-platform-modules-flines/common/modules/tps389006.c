#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/jiffies.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include <linux/hwmon-sysfs.h>
#include "tps389006.h"

extern void __iomem *fpga_ctl_addr;

static int g_tps389006_loglevel = 0x0f;

module_param(g_tps389006_loglevel, int, 0644);
MODULE_PARM_DESC(g_tps389006_loglevel, "The log level(err=0x1,info=0x4, dbg=0x8).\n");

static uint8_t tps389006_i2c_read_byte(struct i2c_client *client, int offset)
{
    int rv = 0;
    rv = i2c_smbus_read_byte_data(client, offset);
    //rv = clx_i2c_read(bus, addr, offset, buf, size);
    return rv;
}

static int32_t tps389006_i2c_write_byte(struct i2c_client *client, int offset, uint8_t data)
{
    int rv = 0;
    rv = i2c_smbus_write_byte_data(client, offset, data);
    //rv = clx_i2c_write(bus, addr, offset, buf, size);
    return rv;
}

static int32_t tps389006_i2c_write_word(struct i2c_client *client, int offset, uint16_t data)
{
    int rv = 0;
    //rv = clx_i2c_write_word(bus, addr, offset, buf);
    rv = i2c_smbus_write_word_data(client, offset, data);
    return rv;
}

static ssize_t chip_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    uint8_t data = 0;

    if (!client)
    {
        TPS389006_LOG_ERR("Invalid i2c_client pointer\n");
        return -1;
    }

    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_VENDOR_ID_REG);
    TPS389006_LOG_INFO("%s, %d, data: %#x\n.", __FUNCTION__, __LINE__, data);
    return sprintf(buf, "%x\r\n", data);
}

static ssize_t tps389006_get_vmon_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_adapter *adapter = client->adapter;
    struct sensor_device_attribute *sensor_attr = container_of(attr, struct sensor_device_attribute, dev_attr);
    uint8_t data = 0, i = 0;
    unsigned int value = 0;
    uint8_t addr30_index_amplification_factor[6] = {2, 2, 2, 3, 3, 11};
    uint8_t addr31_index_amplification_factor[6] = {1, 1, 1, 2, 3, 3};

    if (!client)
    {
        TPS389006_LOG_ERR("Invalid i2c_client pointer\n");
        return -1;
    }
    if (!adapter)
    {
        TPS389006_LOG_ERR("No adapter associated with the client\n");
        return -1;
    }

    do
    {
        data = tps389006_i2c_read_byte(client, TPS_389006_VMON_BANK_SEL_REG);
        TPS389006_LOG_INFO("%s, %d, data: %#x\n.", __FUNCTION__, __LINE__, data);
        if (data != 0x00)
        {
            data = 0x00;
            tps389006_i2c_write_byte(client, TPS_389006_VMON_BANK_SEL_REG, data);
        }
        else
        {
            break;
        }

        usleep_range(500, 1000);

    } while (i++ < 5);
    if (i >= 5)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_BANK_SEL_REG= %x\r\n", data);
        return -1;
    }

    data = 0x00;

    data = tps389006_i2c_read_byte(client, (TPS_389006_VMON_VIN_LVL_BASE_REG + sensor_attr->index));
    TPS389006_LOG_INFO("%s, %d, data: %#x, sensor_attr->index: %d\n.", __FUNCTION__, __LINE__, data, sensor_attr->index);
    if (data != 0)
    {
        if (client->addr == 0x30)
        {
            value = ((((unsigned int)data * 5) + 200) * addr30_index_amplification_factor[sensor_attr->index]);
        }
        else if (client->addr == 0x31)
        {
            value = ((((unsigned int)data * 5) + 200) * addr31_index_amplification_factor[sensor_attr->index]);
        }
        else
        {
            TPS389006_LOG_ERR("client->addr ERR\r\n");
            return -1;
        }
    }
    else
    {
        value = 0;
    }
    TPS389006_LOG_INFO("%s, %d, value: %#x\n.", __FUNCTION__, __LINE__, value);
    return sprintf(buf, "%u\n", value);
}

SENSOR_DEVICE_ATTR_RO(mon1, tps389006_get_vmon, 0);
SENSOR_DEVICE_ATTR_RO(mon2, tps389006_get_vmon, 1);
SENSOR_DEVICE_ATTR_RO(mon3, tps389006_get_vmon, 2);
SENSOR_DEVICE_ATTR_RO(mon4, tps389006_get_vmon, 3);
SENSOR_DEVICE_ATTR_RO(mon5, tps389006_get_vmon, 4);
SENSOR_DEVICE_ATTR_RO(mon6, tps389006_get_vmon, 5);
DEVICE_ATTR_RO(chip_id);

static struct attribute *tps389006_attrs[] = {
    &sensor_dev_attr_mon1.dev_attr.attr,
    &sensor_dev_attr_mon2.dev_attr.attr,
    &sensor_dev_attr_mon3.dev_attr.attr,
    &sensor_dev_attr_mon4.dev_attr.attr,
    &sensor_dev_attr_mon5.dev_attr.attr,
    &sensor_dev_attr_mon6.dev_attr.attr,
    &dev_attr_chip_id.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = tps389006_attrs,
};

int drv_tps389006_device_init(struct i2c_client *client)
{
    struct i2c_adapter *adapter = client->adapter;
    uint8_t data = 0, i = 0;
    uint16_t value = 0;

    if (!client)
    {
        TPS389006_LOG_ERR("Invalid i2c_client pointer\n");
        return -1;
    }
    if (!adapter)
    {
        TPS389006_LOG_ERR("No adapter associated with the client\n");
        return -1;
    }
    if (fpga_ctl_addr == NULL)
    {
        TPS389006_LOG_ERR("fpga_ctl_addr is not available.\r\n");
        return -1;
    }
    do
    {
        data = tps389006_i2c_read_byte(client, TPS_389006_VMON_STAT_REG);
        TPS389006_LOG_INFO("TPS_389006_VMON_STAT_REG data: %#x\n.", data);
        if (data & (0x01 << 6))
        {
            break;
        }
        usleep_range(500, 1000);
    } while (i++ < 5);
    if (i >= 5)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_STAT_REG = %x\r\n", data);
        return -1;
    }

    if (client->addr == 0x30)
    {
        value = 0xd601;
    }
    else if (client->addr == 0x31)
    {
        value = 0x0001;
    }
    else
    {
        TPS389006_LOG_ERR("client->addr ERR\r\n");
        return -1;
    }
    TPS389006_LOG_INFO("client->addr: %#x, value: %#x\n.", client->addr, value);
    tps389006_i2c_write_word(client, TPS_389006_VMON_BANK_SEL_REG, value);
    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_BANK_SEL_REG);
    TPS389006_LOG_INFO("TPS_389006_VMON_BANK_SEL_REG data: %#x\n.", data);
    if (data != 0x01)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_BANK_SEL_REG = %x\r\n", data);
        return -1;
    }

    if (client->addr == 0x30)
    {
        value = 0x8700;
    }
    else if (client->addr == 0x31)
    {
        value = 0x5100;
    }
    else
    {
        TPS389006_LOG_ERR("client->addr ERR\r\n");
        return -1;
    }
    TPS389006_LOG_INFO("client->addr: %#x, value: %#x\n.", client->addr, value);
    tps389006_i2c_write_word(client, TPS_389006_VMON_MISC_REG, value);
    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_MISC_REG);
    TPS389006_LOG_INFO("TPS_389006_VMON_MISC_REG data: %#x\n.", data);
    if (data != 0x00)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_MISC_REG = %x\r\n", data);
        return -1;
    }

    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_VIN_CH_EN_REG);
    TPS389006_LOG_INFO("TPS_389006_VMON_VIN_CH_EN_REG data: %#x\n.", data);
    if (data != 0x3f)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_VIN_CH_EN_REG = %x\r\n", data);
        return -1;
    }

    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_VRANGE_MULT_REG);
    TPS389006_LOG_INFO("TPS_389006_VMON_VRANGE_MULT_REG data: %#x\n.", data);
    if (data != 0x00)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_VRANGE_MULT_REG = %x\r\n", data);
        return -1;
    }

    data = 0x00;
    tps389006_i2c_write_byte(client, TPS_389006_VMON_BANK_SEL_REG, data);
    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_BANK_SEL_REG);
    TPS389006_LOG_INFO("TPS_389006_VMON_BANK_SEL_REG data: %#x\n.", data);
    if (data != 0x00)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_BANK_SEL_REG = %x\r\n", data);
        return -1;
    }

    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_OFF_STAT_REG);
    TPS389006_LOG_INFO("TPS_389006_VMON_OFF_STAT_REG data: %#x\n.", data);
    if ((data & 0x3F) != 0x00)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_OFF_STAT_REG = %x\r\n", data);
        return -1;
    }

    if (NULL != fpga_ctl_addr)
    {
        data = 0x00;
        data = readl(fpga_ctl_addr + TPS_389006_VMON_ACT_ADDR_REG);

        if (client->addr == 0x30)
        {
            data = (data | 0x01);

            writel(data, fpga_ctl_addr + TPS_389006_VMON_ACT_ADDR_REG);
        }
        else if (client->addr == 0x31)
        {
            data = (data | 0x02);

            writel(data, fpga_ctl_addr + TPS_389006_VMON_ACT_ADDR_REG);
        }
        else
        {
            TPS389006_LOG_ERR("client->addr ERR\r\n");
            return -1;
        }
    }

    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_STAT_REG);
    TPS389006_LOG_INFO("TPS_389006_VMON_STAT_REG data: %#x\n.", data);
    if (data != 0x7e)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_ACT_ADDR_REG TPS_389006_VMON_STAT_REG = %x\r\n", data);
        return -1;
    }

    return 0;
}

void drv_tps389006_device_uninit(struct i2c_client *client)
{
    struct i2c_adapter *adapter = client->adapter;
    uint8_t data = 0;
    if (!client)
    {
        TPS389006_LOG_ERR("Invalid i2c_client pointer\n");
        return;
    }
    if (!adapter)
    {
        TPS389006_LOG_ERR("No adapter associated with the client\n");
        return;
    }

    if (NULL != fpga_ctl_addr)
    {
        data = 0x00;
        data = readl(fpga_ctl_addr + TPS_389006_VMON_ACT_ADDR_REG);

        if (client->addr == 0x30)
        {
            data = (data & (~0x01));

            writel(data, fpga_ctl_addr + TPS_389006_VMON_ACT_ADDR_REG);
        }
        else if (client->addr == 0x31)
        {
            data = (data & (~0x02));

            writel(data, fpga_ctl_addr + TPS_389006_VMON_ACT_ADDR_REG);
        }
        else
        {
            TPS389006_LOG_ERR("client->addr ERR\r\n");
            return;
        }
    }

    data = 0x01;
    tps389006_i2c_write_byte(client, TPS_389006_VMON_BANK_SEL_REG, data);
    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_BANK_SEL_REG);
    if (data != 0x01)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_BANK_SEL_REG = %x\r\n", data);
        return;
    }

    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_CTL_REG);
    data = (data | (0x01 << 3));
    tps389006_i2c_write_byte(client, TPS_389006_VMON_CTL_REG, data);

    data = 0x00;
    tps389006_i2c_write_byte(client, TPS_389006_VMON_BANK_SEL_REG, data);
    data = tps389006_i2c_read_byte(client, TPS_389006_VMON_BANK_SEL_REG);

    if (data != 0x00)
    {
        TPS389006_LOG_ERR("TPS_389006_VMON_BANK_SEL_REG = %x\r\n", data);
        return;
    }
}

static int tps389006_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    TPS389006_LOG_INFO("i2c slave:%s addr:%x start probe.", client->name, client->addr);

    drv_tps389006_device_init(client);

    return sysfs_create_group(&(client->dev.kobj), &attr_group);
}

static struct i2c_device_id tps389006_id[] = {
    {"tps389006", 0},
    {}};
MODULE_DEVICE_TABLE(i2c, tps389006_id);

static struct i2c_driver tps389006_driver = {
    .driver = {
        .name = "tps389006",
    },
    .probe = tps389006_probe,
    .id_table = tps389006_id,
};

module_i2c_driver(tps389006_driver);

MODULE_AUTHOR("Bao Hengxi baohx@clounix.com");
MODULE_DESCRIPTION("TPS389006 Voltage Monitor Driver");
MODULE_LICENSE("GPL v2");