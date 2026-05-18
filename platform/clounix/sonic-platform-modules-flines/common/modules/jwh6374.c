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
#include "pmbus.h"
/* Vendor specific registers. */
#define MFR_RESO_SET 0xA1
#define MFR_PRODUCT_ID 0xB2

#define JWH6374_LOG_ERR(fmt, args...) \
    printk(KERN_ERR "[JWH6374][func:%s line:%d]: " fmt, __func__, __LINE__, ##args);

#define JWH6374_LOG_INFO(fmt, args...) \
    printk(KERN_INFO "[JWH6374][func:%s line:%d]: " fmt, __func__, __LINE__, ##args);


static struct pmbus_platform_data jwh6374_pdata = {0};
static struct pmbus_driver_info jwh6374_info = {0};

struct jwh6374_data
{
    struct pmbus_driver_info info;
    struct pmbus_sensor *sensor;
    int total_curr_resolution;
    int phase_curr_resolution;
    int curr_sense_gain;
    int vol_scal_factor;
};

#define to_jwh6374_data(x) container_of(x, struct jwh6374_data, info)

static int jwh6374_read_byte_data(struct i2c_client *client, int page, int reg)
{
    //JWH6374_LOG_INFO("%s, %d, page: %#x: reg: %#x\n", __FUNCTION__, __LINE__, page, reg);
    return 0;
}

static unsigned short process_vout(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned int vout = 0;
    int vscale_factor = 0;

    const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
    struct jwh6374_data *data = to_jwh6374_data(info);
    vscale_factor = data->vol_scal_factor;

    //JWH6374_LOG_INFO("%s, %d\n", __FUNCTION__, __LINE__);
    vout = pmbus_read_word_data(client, page, 0xff, reg);
    //JWH6374_LOG_INFO("%s, %d, vout: %#x\n", __FUNCTION__, __LINE__, vout);
    vout = ((vout & 0x7ff) * 2500) * vscale_factor;

    return (vout / 1000);
}

static unsigned short process_power(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned short power = 0;
    //JWH6374_LOG_INFO("%s, %d\n", __FUNCTION__, __LINE__);
    power = pmbus_read_word_data(client, page, 0xff, reg);
    //JWH6374_LOG_INFO("%s, %d, power: %#x\n", __FUNCTION__, __LINE__, power);
    power = ((power & 0x7ff) / 4);

    return power;
}

static unsigned short process_iout(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned short iout = 0;
    //JWH6374_LOG_INFO("%s, %d\n", __FUNCTION__, __LINE__);
    iout = pmbus_read_word_data(client, page, 0xff, reg);
    //JWH6374_LOG_INFO("%s, %d, iout: %#x\n", __FUNCTION__, __LINE__, iout);
    iout = ((iout & 0x7ff) / 4);

    return iout;
}

static int jwh6374_read_word_data(struct i2c_client *client, int page, int phase, int reg)
{
    //JWH6374_LOG_INFO("%s, %d, page: %#x, phase: %#x, reg: %#x\n", __FUNCTION__, __LINE__, page, phase, reg);
    switch (reg)
    {
    case PMBUS_READ_IOUT:
        return process_iout(client, page, phase, reg);

    case PMBUS_READ_VOUT:
        return process_vout(client, page, phase, reg);

    case PMBUS_READ_POUT:
        return process_power(client, page, phase, reg);

    default:
        break;
    }
    return -ENODATA;
}

static int jwh6374_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct pmbus_driver_info *info;
    struct jwh6374_data *data;
    unsigned short cfg_data;

    data = devm_kzalloc(&client->dev, sizeof(struct jwh6374_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;
    data->vol_scal_factor = 1;

    client->dev.platform_data = &jwh6374_pdata;
    jwh6374_pdata.flags = PMBUS_SKIP_STATUS_CHECK;

    memcpy(&data->info, &jwh6374_info, sizeof(*info));
    info = &data->info;

    /* start init param */
    info->pages = 2;

    info->format[PSC_VOLTAGE_IN] = linear;
    info->format[PSC_VOLTAGE_OUT] = linear;
    info->format[PSC_CURRENT_OUT] = linear;
    info->format[PSC_POWER] = linear;

    i2c_smbus_write_byte_data(client, PMBUS_PAGE, 0x0);
    cfg_data = 0;
    cfg_data = i2c_smbus_read_word_data(client, MFR_PRODUCT_ID);
    if (cfg_data != 0x74)
    {
        JWH6374_LOG_ERR("MFR_PRODUCT_ID err id = %x\r\n", cfg_data);
        return -ENXIO;
    }
    cfg_data = 0;
    cfg_data = i2c_smbus_read_word_data(client, MFR_RESO_SET);
    cfg_data = cfg_data | 0xF;
    i2c_smbus_write_word_data(client, MFR_RESO_SET, cfg_data);

    i2c_smbus_write_byte_data(client, PMBUS_PAGE, 0x1);
    cfg_data = 0;
    cfg_data = i2c_smbus_read_word_data(client, MFR_RESO_SET);
    cfg_data = cfg_data | 0x3;
    i2c_smbus_write_word_data(client, MFR_RESO_SET, cfg_data);

    i2c_smbus_write_byte_data(client, PMBUS_PAGE, 0x00);

    info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IOUT | PMBUS_HAVE_PIN | PMBUS_HAVE_POUT | PMBUS_HAVE_STATUS_VOUT |
                    PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT;
    info->func[1] = PMBUS_HAVE_VOUT | PMBUS_HAVE_IOUT | PMBUS_HAVE_POUT | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_STATUS_IOUT;

    info->read_byte_data = jwh6374_read_byte_data;
    info->read_word_data = jwh6374_read_word_data;

    if (pmbus_do_probe(client,info) == 0) {
        JWH6374_LOG_INFO("%s, %d\n", __FUNCTION__, __LINE__);
        return 0;
    }

    JWH6374_LOG_ERR("jwh6374_probe ERR\r\n");
    return -1;
}

static const struct i2c_device_id jwh6374_id[] = {
    {"jwh6374", 0},
    {}};

MODULE_DEVICE_TABLE(i2c, jwh6374_id);

static const struct of_device_id __maybe_unused jwh6374_of_match[] = {
    {.compatible = "jwt,jwh6374"},
    {}};
MODULE_DEVICE_TABLE(of, jwh6374_of_match);

static struct i2c_driver jwh6374_driver = {
    .driver = {
        .name = "jwh6374",
        .of_match_table = of_match_ptr(jwh6374_of_match),
    },
    .probe = jwh6374_probe,
    .id_table = jwh6374_id,
};

module_i2c_driver(jwh6374_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@nvidia.com>");
MODULE_DESCRIPTION("PMBus driver for JWT jwh6374 device");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(PMBUS);