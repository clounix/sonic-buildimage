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
#include <linux/io.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include "pmbus.h"

#include "clounix/clounix_fpga.h"
#include "clounix/io_signal_ctrl.h"

extern struct pmbus_driver_info clx_psu_list[2];

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
#define pwok (1<<1)
#define acok (1<<2)

/* {'green':'0', 'amber':'1', 'off':'2', 'blink_green':'3', 'blink_amber':'4'}, */
#define led_green (0)
#define led_amber (1)
#define led_off (2)
#define led_blink_green (3)
#define stat_word_fail (0xecff)

extern void __iomem *clounix_fpga_base;
ssize_t psu_stat_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct fpga_device_attribute *fpga_dev = container_of(attr, struct fpga_device_attribute, dev_attr);
    unsigned int index = fpga_dev->index;
    int io_data;
    int type;

    switch (index) {
        case 0:
        case 1:
            type = PSU_PRST;
            index -= 0;
            break;

        case 2:
        case 3:
            type = PSU_ACOK;
            index -= 2;
            break;

        case 4:
        case 5:
            type = PSU_PWOK;
            index -= 4;
            break;
    }

    io_data = read_io_sig_desc(type, index);
    return (io_data > 0) ? sprintf(buf, "%d\n", io_data) : sprintf(buf, "%d\n", 0);
}

ssize_t led_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    unsigned short psu_stat_data;
    unsigned short witch_psu;
    unsigned int led_status = led_off;
    char *led_status_list[] = {
        "green",
        "amber",
        "off",
        "blink_green",
    };
    int io_data;

    if (client->addr == psu_1_addr)
        witch_psu = 0;
    else if (client->addr == psu_2_addr)
        witch_psu = 1;
    else
        return -EIO;

    io_data = read_io_sig_desc(PSU_ACOK, witch_psu);
    if (io_data == 1)
        led_status = led_green;
    else
        led_status = led_off;

    io_data = read_io_sig_desc(PSU_PWOK, witch_psu);
    if (led_status == led_green && io_data == 1)
        led_status = led_green;

    psu_stat_data = pmbus_read_word_data(client, 0, PMBUS_STATUS_WORD);
    if ((psu_stat_data & stat_word_fail) != 0)
        led_status = led_amber;

    return sprintf(buf, "%s\n", led_status_list[led_status]);
}

FPGA_DEVICE_ATTR(psu_1_prst, S_IRUSR, psu_stat_show, NULL, 0);
FPGA_DEVICE_ATTR(psu_2_prst, S_IRUSR, psu_stat_show, NULL, 1);

FPGA_DEVICE_ATTR(psu_1_acok, S_IRUSR, psu_stat_show, NULL, 2);
FPGA_DEVICE_ATTR(psu_2_acok, S_IRUSR, psu_stat_show, NULL, 3);

FPGA_DEVICE_ATTR(psu_1_pwok, S_IRUSR, psu_stat_show, NULL, 4);
FPGA_DEVICE_ATTR(psu_2_pwok, S_IRUSR, psu_stat_show, NULL, 5);

SENSOR_DEVICE_ATTR(led_status, S_IRUGO, led_status_show, NULL, 0);
SENSOR_DEVICE_ATTR(mfr_id, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_ID);
SENSOR_DEVICE_ATTR(mfr_model, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_MODEL);
SENSOR_DEVICE_ATTR(mfr_revision, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_REVISION);
SENSOR_DEVICE_ATTR(mfr_location, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_LOCATION);
SENSOR_DEVICE_ATTR(mfr_date, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_DATE);
SENSOR_DEVICE_ATTR(mfr_serial, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_SERIAL);

static struct attribute *fpga_psu_attrs[] = {
    &fpga_dev_attr_psu_1_acok.dev_attr.attr,
    &fpga_dev_attr_psu_1_pwok.dev_attr.attr,
    &fpga_dev_attr_psu_1_prst.dev_attr.attr,
    &fpga_dev_attr_psu_2_acok.dev_attr.attr,
    &fpga_dev_attr_psu_2_pwok.dev_attr.attr,
    &fpga_dev_attr_psu_2_prst.dev_attr.attr,
    &sensor_dev_attr_led_status.dev_attr.attr,
    NULL,
};

static struct attribute *psu_mfr_attrs[] = {
    &sensor_dev_attr_mfr_id.dev_attr.attr,
    &sensor_dev_attr_mfr_model.dev_attr.attr,
    &sensor_dev_attr_mfr_revision.dev_attr.attr,
    &sensor_dev_attr_mfr_location.dev_attr.attr,
    &sensor_dev_attr_mfr_date.dev_attr.attr,
    &sensor_dev_attr_mfr_serial.dev_attr.attr,
    NULL,
};

static struct attribute_group common_attr_group = {
    .attrs = psu_mfr_attrs,
};

static struct attribute_group priv_attr_group = {
    .attrs = fpga_psu_attrs,
};

static const struct attribute_group *attr_groups[] = {
    &common_attr_group,
    &priv_attr_group,
    NULL,
};

extern int psu_add(struct i2c_client *client);
extern void psu_del(struct i2c_client *client);
extern int psu_add_priv(struct i2c_client *client, struct device *dev);
extern void psu_del_priv(struct i2c_client *client);
static int clx_psu_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    struct pmbus_driver_info *data;
    struct device *dev = &client->dev;
    struct device *hwmon_dev = NULL;
    int ret;

	data = devm_kzalloc(dev, sizeof(struct pmbus_driver_info), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

    memcpy(data, &clx_psu_list[id->driver_data], sizeof(struct pmbus_driver_info));

    ret = pmbus_do_probe(client, id, data);
    if (ret != 0) {
        goto err_pmbus;
    }

    hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
                                                       client, attr_groups);
    if (IS_ERR(hwmon_dev)) {
        ret = PTR_ERR(hwmon_dev);
        goto err_hwmon_dev;
    }

    psu_add_priv(client, hwmon_dev);
    psu_add(client);

    return ret;

err_hwmon_dev:
    pmbus_do_remove(client);
err_pmbus:
    devm_kfree(&client->dev, data);

    return ret;
}

static int clx_psu_remove(struct i2c_client *client)
{
    psu_del_priv(client);
    psu_del(client);
    pmbus_do_remove(client);

    return 0;
}

static const struct i2c_device_id clx_psu_id[] = {
	{"gw1200d", 0},
	{"yesm1300am", 1},
	{}
};

MODULE_DEVICE_TABLE(i2c, clx_psu_id);

static const struct of_device_id __maybe_unused clx_psu_of_match[] = {
	{.compatible = "greatwall, gw1200d"},
	{.compatible = "3y_power, yesm1300am"},
	{}
};
MODULE_DEVICE_TABLE(of, clx_psu_of_match);

static struct i2c_driver clx_psu_driver = {
	.driver = {
		.name = "clx psu",
		.of_match_table = of_match_ptr(clx_psu_of_match),
	},
	.probe = clx_psu_probe,
	.remove = clx_psu_remove,
	.id_table = clx_psu_id,
};

module_i2c_driver(clx_psu_driver);

MODULE_AUTHOR("baohx@clounix.com");
MODULE_DESCRIPTION("PMBus driver for CLX PSU");
MODULE_LICENSE("GPL v2");
