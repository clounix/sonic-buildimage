// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hardware monitoring driver for MPS Multi-phase Digital VR Controllers
 *
 * Copyright (C) 2020 Nvidia Technologies Ltd.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pmbus.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include "pmbus.h"

#include "clounix/clounix_fpga.h"

struct gwcr_data {
	struct pmbus_driver_info info;
	struct pmbus_sensor *sensor;
	int total_curr_resolution;
	int phase_curr_resolution;
	int curr_sense_gain;
};

struct master_priv_data {
    struct i2c_adapter adap;
    struct mutex lock;
    void __iomem *mmio;
};

static struct pmbus_platform_data gwcr_pdata = {0};
static struct pmbus_driver_info gwcr_info = {0};

ssize_t mfr_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char tmp_buf[I2C_SMBUS_BLOCK_MAX+2] = {0};
    int len, i;
    struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
    struct i2c_client *client = to_i2c_client(dev->parent);

    len = i2c_smbus_read_block_data(client, s_attr->index, tmp_buf);

    if (len > 0) {
        if (s_attr->index == PMBUS_MFR_REVISION) {
            for (i=0; i<len; i++) {
                sprintf(buf, "%x", tmp_buf[i]);
                buf++;
            }
            sprintf(buf, "\n");
            return len+1;
        } else {
            return sprintf(buf, "%s\n", tmp_buf);
        }
    }

    return 0;
}

#define psu_stat_offset (0x608)
#define psu_1_addr (0x58)
#define psu_2_addr (0x5a)
#define i2c_psu_master_offset (0x200000)
#define pwok (1<<1)
#define acok (1<<2)

/* {'green':'0', 'amber':'1', 'off':'2', 'blink_green':'3', 'blink_amber':'4'}, */
#define led_green (0)
#define led_amber (1)
#define led_off (2)
#define led_blink_green (3)
#define stat_word_fail (0x7fec)

ssize_t psu_stat_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct i2c_adapter *adap = client->adapter;
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    void __iomem *psu_stat = (priv->mmio - i2c_psu_master_offset) + psu_stat_offset;

    struct fpga_device_attribute *fpga_dev = container_of(attr, struct fpga_device_attribute, dev_attr);
    unsigned int index = fpga_dev->index;
    unsigned int data;

    index = 1 << index;
    data = readb(psu_stat);
    if (fpga_dev->index > 3)
        data = (data & 0x10) ? data : 0;
    else
        data = (data & 0x1) ? data : 0;

    data = ((data & index) != 0) ? 1 : 0;

    return sprintf(buf, "%d\n", data);
}

ssize_t led_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct i2c_adapter *adap = client->adapter;
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    void __iomem *psu_stat = (priv->mmio - i2c_psu_master_offset) + psu_stat_offset;
    unsigned short psu_stat_data;
    unsigned short stat = readw(psu_stat);
    unsigned int led_status = led_off;
    char *led_status_list[] = {
        "green",
        "amber",
        "off",
        "blink_green",
    };

    if (client->addr == psu_1_addr)
        stat = stat & 0x7;
    else if (client->addr == psu_2_addr)
        stat = (stat >> 4) & 0x7;
    else
        return 0;

    if (stat & acok)
        led_status = led_green;
    else
        led_status = led_off;

    if (stat & pwok)
        led_status = led_blink_green;

    psu_stat_data = pmbus_read_word_data(client, 0, PMBUS_STATUS_WORD);
    if ((psu_stat_data & stat_word_fail) != 0)
        led_status = led_amber;

    return sprintf(buf, "%s\n", led_status_list[led_status]);
}

FPGA_DEVICE_ATTR(psu_1_acok, S_IRUSR, psu_stat_show, NULL, 2);
FPGA_DEVICE_ATTR(psu_1_pwok, S_IRUSR, psu_stat_show, NULL, 1);
FPGA_DEVICE_ATTR(psu_1_prst, S_IRUSR, psu_stat_show, NULL, 0);

FPGA_DEVICE_ATTR(psu_2_acok, S_IRUSR, psu_stat_show, NULL, 6);
FPGA_DEVICE_ATTR(psu_2_pwok, S_IRUSR, psu_stat_show, NULL, 5);
FPGA_DEVICE_ATTR(psu_2_prst, S_IRUSR, psu_stat_show, NULL, 4);

SENSOR_DEVICE_ATTR(led_status, S_IRUGO, led_status_show, NULL, 0);
SENSOR_DEVICE_ATTR(mfr_id, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_ID);
SENSOR_DEVICE_ATTR(mfr_model, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_MODEL);
SENSOR_DEVICE_ATTR(mfr_revision, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_REVISION);
SENSOR_DEVICE_ATTR(mfr_location, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_LOCATION);
SENSOR_DEVICE_ATTR(mfr_date, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_DATE);
SENSOR_DEVICE_ATTR(mfr_serial, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_SERIAL);

static struct attribute *gwcr_psu_attrs[] = {
    &fpga_dev_attr_psu_1_acok.dev_attr.attr,
    &fpga_dev_attr_psu_1_pwok.dev_attr.attr,
    &fpga_dev_attr_psu_1_prst.dev_attr.attr,
    &fpga_dev_attr_psu_2_acok.dev_attr.attr,
    &fpga_dev_attr_psu_2_pwok.dev_attr.attr,
    &fpga_dev_attr_psu_2_prst.dev_attr.attr,
    &sensor_dev_attr_led_status.dev_attr.attr,

    &sensor_dev_attr_mfr_id.dev_attr.attr,
    &sensor_dev_attr_mfr_model.dev_attr.attr,
    &sensor_dev_attr_mfr_revision.dev_attr.attr,
    &sensor_dev_attr_mfr_location.dev_attr.attr,
    &sensor_dev_attr_mfr_date.dev_attr.attr,
    &sensor_dev_attr_mfr_serial.dev_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = gwcr_psu_attrs,
};

static const struct attribute_group *attr_groups[] = {
    &attr_group,
    NULL,
};

extern int psu_add(struct i2c_client *client);
extern void psu_del(struct i2c_client *client);
extern int psu_add_priv(struct i2c_client *client, struct device *dev);
extern void psu_del_priv(struct device *dev);
static struct device *hwmon_dev = NULL;
static int gwcr_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    struct pmbus_driver_info *info;
    struct gwcr_data *data;
    struct device *dev = &client->dev;
    int ret;

	data = devm_kzalloc(&client->dev, sizeof(struct gwcr_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

    client->dev.platform_data = &gwcr_pdata;

	memcpy(&data->info, &gwcr_info, sizeof(*info));
	info = &data->info;

    /* start init param */
    info->pages = 2;

    info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_PIN |
                    PMBUS_HAVE_POUT | PMBUS_HAVE_FAN12 | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_TEMP3 |
                    PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP | PMBUS_HAVE_STATUS_FAN12;

    hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
                                                       client, attr_groups);
    if (IS_ERR(hwmon_dev))
        return PTR_ERR(hwmon_dev);

    psu_add_priv(client, hwmon_dev);

    ret = pmbus_do_probe(client, id, info);
    if (ret == 0) {
        psu_add(client);
    }

    return ret;
}

static int gwcr_remove(struct i2c_client *client)
{
    psu_del_priv(hwmon_dev);
    psu_del(client);
    pmbus_do_remove(client);

    return 0;
}

static const struct i2c_device_id gwcr_id[] = {
	{"gw1200d", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, gwcr_id);

static const struct of_device_id __maybe_unused gwcr_of_match[] = {
	{.compatible = "greatwall,gw1200d"},
	{}
};
MODULE_DEVICE_TABLE(of, gwcr_of_match);

static struct i2c_driver gwcr_driver = {
	.driver = {
		.name = "gwcr",
		.of_match_table = of_match_ptr(gwcr_of_match),
	},
	.probe = gwcr_probe,
	.remove = gwcr_remove,
	.id_table = gwcr_id,
};

module_i2c_driver(gwcr_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@nvidia.com>");
MODULE_DESCRIPTION("PMBus driver for GreatWall PSU");
MODULE_LICENSE("GPL v2");
