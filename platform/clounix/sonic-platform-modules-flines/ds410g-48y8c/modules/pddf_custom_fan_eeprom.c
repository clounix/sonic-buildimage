#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include "pddf_custom_fan.h"

static int *log_level = &fan_log_level;


struct fan_eeprom_priv {
    struct i2c_client   *client;
    struct mutex        lock;
    struct device       *i2c_dev;
};

static struct fan_eeprom_priv *g_priv = NULL;

static int fan_eeprom_wait_bus_tx_done(struct i2c_client *client)
{
    unsigned char val = 0;
    unsigned long timeout = jiffies + FAN_EEPROM_I2C_TIMEOUT;

    do {
        val = 0;
        val = i2c_smbus_read_byte_data(client, FAN_EEPROM_IIC_STATUS_OFFSET);
        if (val & FAN_EEPROM_TX_FINISH_MASK) {
            if (val & FAN_EEPROM_TX_ERROR_MASK) {
                pddf_err(FAN, "fan-eeprom: fan_eeprom_wait_bus_tx_done data ECOMM error\n");
                return -ECOMM;
            }
            return 0;
        }
        usleep_range(5, 10);
    } while (time_before(jiffies, timeout));

    pddf_err(FAN, "fan-eeprom: fan_eeprom_wait_tx_done data ETIMEDOUT error\n");
    return -ETIMEDOUT;
}

static ssize_t read_fan_eeprom_data(void *fan, unsigned int fan_index, char *buf, loff_t offset,
                                        size_t count)
{
    struct fan_eeprom_priv *priv = (struct fan_eeprom_priv *)fan;
    unsigned char val = 0;
    unsigned char reg = 0;
    unsigned int i = 0;
    int ret = 0;

    if (count > FAN_EEPROM_SIZE)
    {
        return -EMSGSIZE;
    }

    val = fan_index;
    reg = FAN_EEPROM_SELECT_OFFSET;
    ret = i2c_smbus_write_byte_data(priv->client, reg, val);

    val = 0x01;
    reg = FAN_EEPROM_DATA_SIZE_OFFSET;
    ret = i2c_smbus_write_byte_data(priv->client, reg, val);

    val = 0x01;
    reg = FAN_EEPROM_IIC_MAGE_OFFSET;
    ret = i2c_smbus_write_byte_data(priv->client, reg, val);

    // LOG_DBG(CLX_DRIVER_TYPES_FAN,"drv_read_fan_eeprom len = %d\r\n",count);

    for (i = 0; i < count; i++)
    {
        val = (offset + i);
        reg = FAN_EEPROM_IIC_REG_OFFSET;
        ret = i2c_smbus_write_byte_data(priv->client, reg, val);

        val = 0x80;
        reg = FAN_EEPROM_IIC_START_OFFSET;
        ret = i2c_smbus_write_byte_data(priv->client, reg, val);

        if (fan_eeprom_wait_bus_tx_done(priv->client) != 0)
        {
            return -ETIMEDOUT;
        }
        else
        {
            val = 0;
            reg = FAN_EEPROM_BYTE_READ_OFFSET;
            val = i2c_smbus_read_byte_data(priv->client, reg);
            buf[i] = val;
            // LOG_DBG(CLX_DRIVER_TYPES_FAN,"drv_read_fan_eeprom i = %d ,buf = 0x%x\r\n",i,buf[i]);
        }
    }

    usleep_range(50, 100);

    return count;
}

static ssize_t write_fan_eeprom_data(void *fan, unsigned int fan_index, char *buf, loff_t offset,
                                         size_t count)
{
    struct fan_eeprom_priv *priv = (struct fan_eeprom_priv *)fan;
    unsigned char val = 0;
    unsigned char reg = 0;
    unsigned int i = 0;
    int ret = 0;

    if (count > FAN_EEPROM_SIZE)
    {
        return -EMSGSIZE;
    }

    val = fan_index;
    reg = FAN_EEPROM_SELECT_OFFSET;
    ret = i2c_smbus_write_byte_data(priv->client, reg, val);

    val = 0x01;
    reg = FAN_EEPROM_DATA_SIZE_OFFSET;
    ret = i2c_smbus_write_byte_data(priv->client, reg, val);

    val = 0x04;
    reg = FAN_EEPROM_IIC_MAGE_OFFSET;
    ret = i2c_smbus_write_byte_data(priv->client, reg, val);

    // LOG_DBG(CLX_DRIVER_TYPES_FAN,"drv_write_fan_eeprom len = %d\r\n",count);

    for (i = 0; i < count; i++)
    {
        val = (offset + i);
        reg = FAN_EEPROM_IIC_REG_OFFSET;
        ret = i2c_smbus_write_byte_data(priv->client, reg, val);

        val = buf[i];
        reg = FAN_EEPROM_BYTE_WRITE_OFFSET;
        ret = i2c_smbus_write_byte_data(priv->client, reg, val);

        // LOG_DBG(CLX_DRIVER_TYPES_FAN,"drv_write_fan_eeprom i = %d ,data = 0x%x\r\n",i,val);

        val = 0x80;
        reg = FAN_EEPROM_IIC_START_OFFSET;
        ret = i2c_smbus_write_byte_data(priv->client, reg, val);

        if (fan_eeprom_wait_bus_tx_done(priv->client) != 0)
        {
            return -ETIMEDOUT;
        }

        usleep_range(6000, 7000);
    }

    return count;
}

static ssize_t fan_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                               char *buf, loff_t offset, size_t count)
{
    struct fan_eeprom_priv *priv = g_priv;
    unsigned int fan_index;
    const char *name = attr->attr.name;
    ssize_t rd_len;

    /* 通过文件名获取fan_idx */
    if      (strcmp(name, "fan1_eeprom") == 0) fan_index = 0;
    else if (strcmp(name, "fan2_eeprom") == 0) fan_index = 1;
    else if (strcmp(name, "fan3_eeprom") == 0) fan_index = 2;
    else if (strcmp(name, "fan4_eeprom") == 0) fan_index = 3;
    else if (strcmp(name, "fan5_eeprom") == 0) fan_index = 4;
    else return -EINVAL;

    memset(buf, 0, count);
    rd_len = read_fan_eeprom_data(priv, fan_index, buf, offset, count);
    if (rd_len < 0)
    {
        pddf_err(FAN, "read fan%u eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
                fan_index, offset, count, rd_len);
        return -EIO;
    }

    pddf_dbg(FAN, "read fan%u eeprom data success, offset:0x%llx, read len:%lu, really read len:%ld.\n",
            fan_index, offset, count, rd_len);

    return rd_len;
}

static ssize_t fan_eeprom_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                                char *buf, loff_t offset, size_t count)
{
    struct fan_eeprom_priv *priv = g_priv;
    unsigned int fan_index;
    const char *name = attr->attr.name;
    ssize_t wr_len;

    /* 通过文件名获取fan_idx */
    if      (strcmp(name, "fan1_eeprom") == 0) fan_index = 0;
    else if (strcmp(name, "fan2_eeprom") == 0) fan_index = 1;
    else if (strcmp(name, "fan3_eeprom") == 0) fan_index = 2;
    else if (strcmp(name, "fan4_eeprom") == 0) fan_index = 3;
    else if (strcmp(name, "fan5_eeprom") == 0) fan_index = 4;
    else return -EINVAL;

    wr_len = write_fan_eeprom_data(priv, fan_index, buf, offset, count);
    if (wr_len < 0)
    {
        pddf_err(FAN, "write fan%u eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
                fan_index, offset, count, wr_len);
        return -EIO;
    }

    pddf_dbg(FAN, "write fan%u eeprom data success, offset:0x%llx, write len:%lu, really write len:%ld.\n",
            fan_index, offset, count, wr_len);

    return wr_len;
}

#define DECLARE_FAN_BIN_ATTR(n) \
    static struct bin_attribute fan_bin_attr_##n = \
        __BIN_ATTR(fan##n##_eeprom, 0644, fan_eeprom_read, fan_eeprom_write, FAN_EEPROM_SIZE)

DECLARE_FAN_BIN_ATTR(1);
DECLARE_FAN_BIN_ATTR(2);
DECLARE_FAN_BIN_ATTR(3);
DECLARE_FAN_BIN_ATTR(4);
DECLARE_FAN_BIN_ATTR(5);

static struct bin_attribute *fan_bin_attrs[] = {
    &fan_bin_attr_1,
    &fan_bin_attr_2,
    &fan_bin_attr_3,
    &fan_bin_attr_4,
    &fan_bin_attr_5,
    NULL,
};

static int fan_eeprom_create_attrs(void *data)
{
    int i, ret = 0;
    struct fan_eeprom_priv *priv = (struct fan_eeprom_priv *)data;
    struct device *dev = priv->i2c_dev;

    for (i = 0; fan_bin_attrs[i] != NULL; i++) {
        ret = sysfs_create_bin_file(&dev->kobj, fan_bin_attrs[i]);
        if (ret) {
            pr_err("fan-eeprom: failed to create attr %s\n", fan_bin_attrs[i]->attr.name);
            break;
        }
    }
    return ret;
}

static void fan_eeprom_remove_attrs(struct device *dev)
{
    int i;

    for (i = 0; fan_bin_attrs[i] != NULL; i++) {
        sysfs_remove_bin_file(&dev->kobj, fan_bin_attrs[i]);
    }
}

static int fan_eeprom_i2c_notify(struct notifier_block *nb, unsigned long action, void *data)
{
    struct device *dev = data;
    struct i2c_client *client;
    struct fan_eeprom_priv *priv;

    if (dev->bus != &i2c_bus_type)
        return NOTIFY_OK;

    if (!dev->driver || strcmp(dev->driver->name, "pddf_fan") != 0)
        return NOTIFY_OK;

    if (action == BUS_NOTIFY_BOUND_DRIVER) {
        pddf_info(FAN, "fan-eeprom: pddf_fan device detected\n");

        priv = kzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv)
            return NOTIFY_OK;

        mutex_init(&priv->lock);
        client = to_i2c_client(dev);
        priv->client = client;
        priv->i2c_dev = dev;

        fan_eeprom_create_attrs(priv);
        g_priv = priv;

    } else if (action == BUS_NOTIFY_UNBOUND_DRIVER) {
        if (g_priv) {
            fan_eeprom_remove_attrs(g_priv->i2c_dev);
            kfree(g_priv);
            g_priv = NULL;
            pddf_info(FAN, "fan-eeprom: pddf_fan removed, attrs cleaned up\n");
        }
    }

    return NOTIFY_OK;
}

static struct notifier_block fan_eeprom_nb = {
    .notifier_call = fan_eeprom_i2c_notify,
};

static int __init fan_eeprom_init(void)
{
    int ret;

    ret = bus_register_notifier(&i2c_bus_type, &fan_eeprom_nb);
    if (ret) {
        pddf_err(FAN, "fan-eeprom: failed to register i2c notifier\n");
        return ret;
    }

    pddf_info(FAN, "fan-eeprom: module loaded, waiting for pddf_fan...\n");
    return 0;
}

static void __exit fan_eeprom_exit(void)
{
    bus_unregister_notifier(&i2c_bus_type, &fan_eeprom_nb);

    if (g_priv) {
        fan_eeprom_remove_attrs(g_priv->i2c_dev);
        kfree(g_priv);
        g_priv = NULL;
    }

    pddf_info(FAN, "fan-eeprom: module unloaded\n");
}

module_init(fan_eeprom_init);
module_exit(fan_eeprom_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDDF Fan EEPROM Addon for pddf_fan driver");
MODULE_AUTHOR("FLKS");
MODULE_SOFTDEP("pre: pddf_fan");
