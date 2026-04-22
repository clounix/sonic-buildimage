#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/notifier.h>

#define FAN_EEPROM_SELECT_OFFSET        (0x20)
#define FAN_EEPROM_IIC_REG_OFFSET       (0x22)
#define FAN_EEPROM_DATA_SIZE_OFFSET     (0x23)
#define FAN_EEPROM_BYTE_READ_OFFSET     (0x25)
#define FAN_EEPROM_IIC_MAGE_OFFSET      (0x26)
#define FAN_EEPROM_IIC_START_OFFSET     (0x27)
#define FAN_EEPROM_IIC_STATUS_OFFSET    (0x28)

#define FAN_EEPROM_IIC_START_MASK       (1 << 7)
#define FAN_EEPROM_TX_FINISH_MASK       (1 << 7)
#define FAN_EEPROM_TX_ERROR_MASK        (1 << 6)

#define FAN_EEPROM_I2C_TIMEOUT          (msecs_to_jiffies(500))
#define FAN_EEPROM_MAX_SIZE             (256)

struct fan_eeprom_priv {
    struct i2c_client   *client;
    struct mutex        lock;
    struct device       *i2c_dev;
};

static struct fan_eeprom_priv *g_priv = NULL;

static int fan_eeprom_wait_tx_done(struct i2c_client *client)
{
    unsigned char val = 0;
    unsigned long timeout = jiffies + FAN_EEPROM_I2C_TIMEOUT;

    do {
        val = i2c_smbus_read_byte_data(client, FAN_EEPROM_IIC_STATUS_OFFSET);
        if (val & FAN_EEPROM_TX_FINISH_MASK) {
            if (val & FAN_EEPROM_TX_ERROR_MASK) {
                pr_err("fan-eeprom: TX error\n");
                return -ECOMM;
            }
            return 0;
        }
        usleep_range(5, 10);
    } while (time_before(jiffies, timeout));

    pr_err("fan-eeprom: TX timeout\n");
    return -ETIMEDOUT;
}

static int __fan_eeprom_read_byte_nolock(struct fan_eeprom_priv *priv, u32 offset, u8 *val)
{
    int ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_REG_OFFSET, offset);
    if (ret < 0) return ret;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_START_OFFSET, FAN_EEPROM_IIC_START_MASK);
    if (ret < 0) return ret;

    ret = fan_eeprom_wait_tx_done(priv->client);
    if (ret < 0) return ret;

    ret = i2c_smbus_read_byte_data(priv->client, FAN_EEPROM_BYTE_READ_OFFSET);
    if (ret < 0) return ret;

    *val = ret;
    return 0;
}

static int fan_eeprom_read_multi(struct fan_eeprom_priv *priv, u8 fan_idx, u32 start_offset, u8 *buf, u32 len)
{
    int ret = 0;
    u32 i;

    if (!priv || !buf) return -EINVAL;

    mutex_lock(&priv->lock);

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_SELECT_OFFSET, fan_idx);
    if (ret < 0) goto err_unlock;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_DATA_SIZE_OFFSET, 1);
    if (ret < 0) goto err_unlock;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_MAGE_OFFSET, 1);
    if (ret < 0) goto err_unlock;

    for (i = 0; i < len; i++) {
        u8 val;
        ret = __fan_eeprom_read_byte_nolock(priv, start_offset + i, &val);
        if (ret != 0) break;
        buf[i] = val;
    }

err_unlock:
    mutex_unlock(&priv->lock);
    return (ret == 0) ? i : ret;
}

static int __fan_eeprom_write_byte_nolock(struct fan_eeprom_priv *priv, u32 offset, u8 val)
{
    int ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_REG_OFFSET, offset);
    if (ret < 0) return ret;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_BYTE_READ_OFFSET, val);
    if (ret < 0) return ret;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_START_OFFSET, FAN_EEPROM_IIC_START_MASK);
    if (ret < 0) return ret;

    return fan_eeprom_wait_tx_done(priv->client);
}

static int fan_eeprom_write_multi(struct fan_eeprom_priv *priv, u8 fan_idx, u32 start_offset, const u8 *buf, u32 len)
{
    int ret = 0;
    u32 i;

    if (!priv || !buf) return -EINVAL;

    mutex_lock(&priv->lock);

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_SELECT_OFFSET, fan_idx);
    if (ret < 0) goto err_unlock;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_DATA_SIZE_OFFSET, 0x01);
    if (ret < 0) goto err_unlock;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_MAGE_OFFSET, 0x04);
    if (ret < 0) goto err_unlock;

    for (i = 0; i < len; i++) {
        ret = __fan_eeprom_write_byte_nolock(priv, start_offset + i, buf[i]);
        if (ret != 0) break;
        usleep_range(6000, 7000);
    }

err_unlock:
    mutex_unlock(&priv->lock);
    return (ret == 0) ? i : ret;
}

/* ---------------- EEPROM raw access ---------------- */
static ssize_t fan_eeprom_show(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
    struct fan_eeprom_priv *priv = g_priv;
    const char *name = attr->attr.name;
    u8 fan_idx;
    int ret;

    if (!priv) return -ENODEV;

    if      (strcmp(name, "fan1_eeprom") == 0) fan_idx = 0;
    else if (strcmp(name, "fan2_eeprom") == 0) fan_idx = 1;
    else if (strcmp(name, "fan3_eeprom") == 0) fan_idx = 2;
    else if (strcmp(name, "fan4_eeprom") == 0) fan_idx = 3;
    else if (strcmp(name, "fan5_eeprom") == 0) fan_idx = 4;
    else return -EINVAL;

    ret = fan_eeprom_read_multi(priv, fan_idx, 0, buf, FAN_EEPROM_MAX_SIZE);
    return ret;
}

static ssize_t fan_eeprom_store(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count)
{
    struct fan_eeprom_priv *priv = g_priv;
    const char *name = attr->attr.name;
    u8 fan_idx;
    int ret;

    if (!priv) return -ENODEV;

    if      (strcmp(name, "fan1_eeprom") == 0) fan_idx = 0;
    else if (strcmp(name, "fan2_eeprom") == 0) fan_idx = 1;
    else if (strcmp(name, "fan3_eeprom") == 0) fan_idx = 2;
    else if (strcmp(name, "fan4_eeprom") == 0) fan_idx = 3;
    else if (strcmp(name, "fan5_eeprom") == 0) fan_idx = 4;
    else return -EINVAL;

    if (count > FAN_EEPROM_MAX_SIZE) count = FAN_EEPROM_MAX_SIZE;

    ret = fan_eeprom_write_multi(priv, fan_idx, 0, buf, count);
    return ret;
}

/* ---------------- Custom fan info parser ---------------- */
static void fan_eeprom_copy_str(u8 *data, char *out, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        u8 c = data[i];
        out[i] = (isascii(c) && isprint(c)) ? c : '.';
    }
    out[len] = '\0';
}

static int fan_eeprom_read_str(u8 fan_idx, u32 offset, u32 len, char *buf)
{
    struct fan_eeprom_priv *priv = g_priv;
    u8 data[32];
    int ret;

    if (!priv) return -ENODEV;
    if (len >= sizeof(data)) return -EINVAL;

    ret = fan_eeprom_read_multi(priv, fan_idx, offset, data, len);
    if (ret != len) return -EIO;

    fan_eeprom_copy_str(data, buf, len);
    return 0;
}

static int fan_eeprom_read_u8(u8 fan_idx, u32 offset, u8 *val)
{
    struct fan_eeprom_priv *priv = g_priv;
    u8 data;
    int ret;

    if (!priv) return -ENODEV;

    ret = fan_eeprom_read_multi(priv, fan_idx, offset, &data, 1);
    if (ret != 1) return -EIO;

    *val = data;
    return 0;
}

static int fan_eeprom_read_num(u8 fan_idx, u32 offset, u32 len, char *buf)
{
    struct fan_eeprom_priv *priv = g_priv;
    u8 data[16];
    char tmp[16];
    int ret;

    if (!priv) return -ENODEV;
    if (len >= sizeof(data)) return -EINVAL;

    ret = fan_eeprom_read_multi(priv, fan_idx, offset, data, len);
    if (ret != len) return -EIO;

    fan_eeprom_copy_str(data, tmp, len);
    return snprintf(buf, PAGE_SIZE, "%s\n", tmp);
}

/* ---------------- Sysfs show functions ---------------- */
#define FAN_STR_SHOW(_name, _offset, _len) \
static ssize_t fan_##_name##_show(struct device *dev, struct device_attribute *attr, char *buf) \
{ \
    const char *name = attr->attr.name; \
    u8 fan_idx; \
    char tmp[32]; \
    if (!g_priv) return -ENODEV; \
    if      (strstr(name, "fan1")) fan_idx = 0; \
    else if (strstr(name, "fan2")) fan_idx = 1; \
    else if (strstr(name, "fan3")) fan_idx = 2; \
    else if (strstr(name, "fan4")) fan_idx = 3; \
    else if (strstr(name, "fan5")) fan_idx = 4; \
    else return -EINVAL; \
    if (fan_eeprom_read_str(fan_idx, _offset, _len, tmp) < 0) return -EIO; \
    return snprintf(buf, PAGE_SIZE, "%s\n", tmp); \
}

#define FAN_NUM_SHOW(_name, _offset, _len) \
static ssize_t fan_##_name##_show(struct device *dev, struct device_attribute *attr, char *buf) \
{ \
    const char *name = attr->attr.name; \
    u8 fan_idx; \
    if (!g_priv) return -ENODEV; \
    if      (strstr(name, "fan1")) fan_idx = 0; \
    else if (strstr(name, "fan2")) fan_idx = 1; \
    else if (strstr(name, "fan3")) fan_idx = 2; \
    else if (strstr(name, "fan4")) fan_idx = 3; \
    else if (strstr(name, "fan5")) fan_idx = 4; \
    else return -EINVAL; \
    return fan_eeprom_read_num(fan_idx, _offset, _len, buf); \
}
#define FAN_STR_HEAD_LEN    (0x0f)

FAN_STR_SHOW(model_name,        (FAN_STR_HEAD_LEN + 0x00), 13)
FAN_STR_SHOW(sn,                (FAN_STR_HEAD_LEN + 0x10), 16)
FAN_STR_SHOW(part_number,       (FAN_STR_HEAD_LEN + 0x21), 8)

static ssize_t fan_num_motors_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char *name = attr->attr.name;
    u8 fan_idx, val;
    if (!g_priv) return -ENODEV;
    if      (strstr(name, "fan1")) fan_idx = 0;
    else if (strstr(name, "fan2")) fan_idx = 1;
    else if (strstr(name, "fan3")) fan_idx = 2;
    else if (strstr(name, "fan4")) fan_idx = 3;
    else if (strstr(name, "fan5")) fan_idx = 4;
    else return -EINVAL;

    if (fan_eeprom_read_u8(fan_idx, 0x40, &val) < 0) return -EIO;
    return snprintf(buf, PAGE_SIZE, "%u\n", val);
}

/* ---------------- Define attributes ---------------- */
#define FAN_CUSTOM_ATTRS(n) \
static DEVICE_ATTR(fan##n##_model_name,     0440, fan_model_name_show, NULL); \
static DEVICE_ATTR(fan##n##_sn,             0440, fan_sn_show, NULL); \
static DEVICE_ATTR(fan##n##_part_number,    0440, fan_part_number_show, NULL); 

FAN_CUSTOM_ATTRS(1);
FAN_CUSTOM_ATTRS(2);
FAN_CUSTOM_ATTRS(3);
FAN_CUSTOM_ATTRS(4);
FAN_CUSTOM_ATTRS(5);

static DEVICE_ATTR(fan1_eeprom, 0660, fan_eeprom_show, fan_eeprom_store);
static DEVICE_ATTR(fan2_eeprom, 0660, fan_eeprom_show, fan_eeprom_store);
static DEVICE_ATTR(fan3_eeprom, 0660, fan_eeprom_show, fan_eeprom_store);
static DEVICE_ATTR(fan4_eeprom, 0660, fan_eeprom_show, fan_eeprom_store);
static DEVICE_ATTR(fan5_eeprom, 0660, fan_eeprom_show, fan_eeprom_store);

static struct device_attribute *fan_eeprom_attrs[] = {
    &dev_attr_fan1_eeprom,
    &dev_attr_fan2_eeprom,
    &dev_attr_fan3_eeprom,
    &dev_attr_fan4_eeprom,
    &dev_attr_fan5_eeprom,

    &dev_attr_fan1_model_name,
    &dev_attr_fan1_sn,
    &dev_attr_fan1_part_number,

    &dev_attr_fan2_model_name,
    &dev_attr_fan2_sn,
    &dev_attr_fan2_part_number,

    &dev_attr_fan3_model_name,
    &dev_attr_fan3_sn,
    &dev_attr_fan3_part_number,

    &dev_attr_fan4_model_name,
    &dev_attr_fan4_sn,
    &dev_attr_fan4_part_number,

    &dev_attr_fan5_model_name,
    &dev_attr_fan5_sn,
    &dev_attr_fan5_part_number,

    NULL
};

static int fan_eeprom_create_attrs(struct device *dev)
{
    int i, ret = 0;
    for (i = 0; fan_eeprom_attrs[i]; i++) {
        ret = device_create_file(dev, fan_eeprom_attrs[i]);
        if (ret)
            pr_err("fan-eeprom: failed to create attr %s\n", fan_eeprom_attrs[i]->attr.name);
    }
    return ret;
}

static void fan_eeprom_remove_attrs(struct device *dev)
{
    int i;
    for (i = 0; fan_eeprom_attrs[i]; i++) {
        device_remove_file(dev, fan_eeprom_attrs[i]);
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
        pr_info("fan-eeprom: pddf_fan device detected\n");

        priv = kzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv)
            return NOTIFY_OK;

        mutex_init(&priv->lock);
        client = to_i2c_client(dev);
        priv->client = client;
        priv->i2c_dev = dev;

        fan_eeprom_create_attrs(dev);
        g_priv = priv;

    } else if (action == BUS_NOTIFY_UNBOUND_DRIVER) {
        if (g_priv) {
            fan_eeprom_remove_attrs(g_priv->i2c_dev);
            kfree(g_priv);
            g_priv = NULL;
            pr_info("fan-eeprom: pddf_fan removed, attrs cleaned up\n");
        }
    }

    return NOTIFY_OK;
}

static struct notifier_block fan_eeprom_nb = {
    .notifier_call = fan_eeprom_i2c_notify,
};

static int __init fan_eeprom_init(void)
{
    int ret = bus_register_notifier(&i2c_bus_type, &fan_eeprom_nb);
    if (ret) {
        pr_err("fan-eeprom: failed to register i2c notifier\n");
        return ret;
    }
    pr_info("fan-eeprom: module loaded\n");
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
    pr_info("fan-eeprom: module unloaded\n");
}

module_init(fan_eeprom_init);
module_exit(fan_eeprom_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDDF Fan EEPROM Custom Fields Driver");
MODULE_AUTHOR("FLKS");
MODULE_SOFTDEP("pre: pddf_fan");
