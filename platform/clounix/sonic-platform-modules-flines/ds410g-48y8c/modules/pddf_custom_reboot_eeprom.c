#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/device.h>
#include <linux/delay.h>

#define FM24C02F_SIZE    8
#define FM24C02F_MAX_ADDR 0xFF
#define I2C_RETRY_COUNT  5

struct fm24c02f_data {
    struct i2c_client *client;
    struct mutex lock;
    struct bin_attribute eeprom_attr;
};

static int fm24c02f_read_byte(struct fm24c02f_data *data, u8 addr, u8 *val)
{
    struct i2c_msg msg[2];
    u8 addr_buf = addr;
    int ret, i;

    if (addr > FM24C02F_MAX_ADDR)
        return -EINVAL;

    msg[0].addr = data->client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &addr_buf;

    msg[1].addr = data->client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = 1;
    msg[1].buf = val;

    for (i = 0; i < I2C_RETRY_COUNT; i++) {
        ret = i2c_transfer(data->client->adapter, msg, 2);
        if (ret == 2)
            return 0;
        udelay(1000);
    }

    dev_err(&data->client->dev, "read failed addr 0x%02x\n", addr);
    return -ETIMEDOUT;
}

static int fm24c02f_write_byte(struct fm24c02f_data *data, u8 addr, u8 val)
{
    struct i2c_msg msg;
    u8 buf[2];
    int ret, i;

    if (addr > FM24C02F_MAX_ADDR)
        return -EINVAL;

    buf[0] = addr;
    buf[1] = val;

    msg.addr = data->client->addr;
    msg.flags = 0;
    msg.len = 2;
    msg.buf = buf;

    for (i = 0; i < I2C_RETRY_COUNT; i++) {
        ret = i2c_transfer(data->client->adapter, &msg, 1);
        if (ret == 1) {
            mdelay(5);
            return 0;
        }
        mdelay(5);
    }

    dev_err(&data->client->dev, "write failed addr 0x%02x\n", addr);
    return -ETIMEDOUT;
}

static ssize_t eeprom_read(struct file *filp, struct kobject *kobj,
                          struct bin_attribute *bin_attr, char *buf,
                          loff_t off, size_t count)
{
    struct fm24c02f_data *data = bin_attr->private;
    ssize_t ret = 0;
    size_t i;

    if (!data || off >= FM24C02F_SIZE)
        return -EINVAL;
    if (off + count > FM24C02F_SIZE)
        count = FM24C02F_SIZE - off;

    mutex_lock(&data->lock);
    for (i = 0; i < count; i++) {
        u8 val;
        ret = fm24c02f_read_byte(data, off + i, &val);
        if (ret < 0)
            break;
        buf[i] = val;
    }
    mutex_unlock(&data->lock);

    return ret < 0 ? ret : count;
}

static ssize_t eeprom_write(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *bin_attr, char *buf,
                           loff_t off, size_t count)
{
    struct fm24c02f_data *data = bin_attr->private;
    ssize_t ret = 0;
    size_t i;
    if (!data || off >= FM24C02F_SIZE)
        return -EINVAL;
    if (off + count > FM24C02F_SIZE)
        count = FM24C02F_SIZE - off;

    mutex_lock(&data->lock);
    for (i = 0; i < count; i++) {
        ret = fm24c02f_write_byte(data, off + i, buf[i]);
        if (ret < 0)
            break;
    }
    mutex_unlock(&data->lock);

    return ret < 0 ? ret : count;
}

static int fm24c02f_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct fm24c02f_data *data;
    int ret;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "I2C not supported\n");
        return -ENODEV;
    }

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    mutex_init(&data->lock);
    i2c_set_clientdata(client, data);

    sysfs_bin_attr_init(&data->eeprom_attr);
    data->eeprom_attr.attr.name = "eeprom";
    data->eeprom_attr.attr.mode = 0664;
    data->eeprom_attr.read = eeprom_read;
    data->eeprom_attr.write = eeprom_write;
    data->eeprom_attr.size = FM24C02F_SIZE;
    data->eeprom_attr.private = data;

    ret = device_create_bin_file(&client->dev, &data->eeprom_attr);
    if (ret) {
        dev_err(&client->dev, "create sysfs fail %d\n", ret);
        return ret;
    }

    dev_info(&client->dev, "FM24C02F FPGA driver loaded addr 0x%02x\n", client->addr);
    return 0;
}

static void fm24c02f_remove(struct i2c_client *client)
{
    struct fm24c02f_data *data = i2c_get_clientdata(client);
    device_remove_bin_file(&client->dev, &data->eeprom_attr);
    dev_info(&client->dev, "FM24C02F removed\n");
}

static const struct i2c_device_id fm24c02f_id[] = {
    {"fm24c02f-rc", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, fm24c02f_id);

static const struct of_device_id fm24c02f_of_match[] = {
    {.compatible = "ramtron,fm24c02f-rc"},
    {}
};
MODULE_DEVICE_TABLE(of, fm24c02f_of_match);

static struct i2c_driver fm24c02f_driver = {
    .driver = {
        .name = "fm24c02f-rc",
        .of_match_table = fm24c02f_of_match,
    },
    .probe = fm24c02f_probe,
    .remove = fm24c02f_remove,
    .id_table = fm24c02f_id,
};

module_i2c_driver(fm24c02f_driver);
MODULE_LICENSE("GPL");
