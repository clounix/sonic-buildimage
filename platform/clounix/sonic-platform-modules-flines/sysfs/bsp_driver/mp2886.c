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

#define MP2888_MFR_SYS_CONFIG   0x44
#define MP2888_MFR_READ_CS1_2   0x73
#define MP2888_MFR_READ_CS3_4   0x74
#define MP2888_MFR_READ_CS5_6   0x75
#define MP2888_MFR_READ_CS7_8   0x76
#define MP2888_MFR_READ_CS9_10  0x77
#define MP2888_MFR_VR_CONFIG1   0xe1

#define MP2888_TOTAL_CURRENT_RESOLUTION BIT(3)
#define MP2888_PHASE_CURRENT_RESOLUTION BIT(4)
#define MP2888_DRMOS_KCS        GENMASK(2, 0)
#define MP2888_TEMP_UNIT        10
#define MP2888_MAX_PHASE        10

struct mp2886_data {
    struct pmbus_driver_info info;
    struct pmbus_sensor *sensor;
    int total_curr_resolution;
    int phase_curr_resolution;
    int curr_sense_gain;
};

#define to_mp2886_data(x)  container_of(x, struct mp2886_data, info)

static int mp2886_read_byte_data(struct i2c_client *client, int page, int reg)
{
    return -ENODATA;
}

#define mfr_vout_loop_ctrl_r1 0xb2
#define mfr_vr_cfg1 0xb7
static unsigned short process_vout(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned short vout ;
    vout = pmbus_read_word_data(client, page, phase, reg);
    vout = vout & 0xfff;
    return vout;
}

static unsigned short process_power(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned short mfr_sys_cfg;
    unsigned short power;
    unsigned short resloution ;

    mfr_sys_cfg = pmbus_read_word_data(client, 0, phase, MP2888_MFR_SYS_CONFIG);
    resloution = (mfr_sys_cfg >> 3) & 0x1;
    power = pmbus_read_word_data(client, page, phase, reg);
    if(!resloution)
        power = power >> 1;

    return power;
}

static unsigned int process_iout(struct i2c_client *client, int page, int phase, int reg)
{
    unsigned int data;
    unsigned int resloution;
    unsigned short mfr_sys_cfg;

    mfr_sys_cfg = pmbus_read_word_data(client, page, phase, MP2888_MFR_SYS_CONFIG);
    resloution = (mfr_sys_cfg >> 3) & 0x1;

    data = pmbus_read_word_data(client, page, phase, reg);
    data = data & 0xfff;

    if (resloution)
        data = data / 2;
    else
        data = data / 4;

    return data;
}

static int mp2886_read_word_data(struct i2c_client *client, int page, int phase, int reg)
{
    switch (reg) {
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

static struct pmbus_platform_data mp2886_pdata = {0};
static struct pmbus_driver_info mp2886_info = {0};

extern int vol_sensor_add(struct i2c_client *client);
extern void vol_sensor_del(struct i2c_client *client);
extern int curr_sensor_add(struct i2c_client *client);
extern void curr_sensor_del(struct i2c_client *client);

static int mp2886_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct pmbus_driver_info *info;
    struct mp2886_data *data;
    unsigned short cfg_data;

    data = devm_kzalloc(&client->dev, sizeof(struct mp2886_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    client->dev.platform_data = &mp2886_pdata;
    mp2886_pdata.flags = PMBUS_SKIP_STATUS_CHECK;

    memcpy(&data->info, &mp2886_info, sizeof(*info));
    info = &data->info;

    /* start init param */
    info->pages = 2;

    info->format[PSC_VOLTAGE_IN] = linear;
    info->format[PSC_VOLTAGE_OUT] = linear;
    info->format[PSC_TEMPERATURE] = direct,
    info->format[PSC_CURRENT_OUT] = linear,
    info->format[PSC_POWER] = linear,

    i2c_smbus_write_byte_data(client, PMBUS_PAGE, 0x0);
    cfg_data = i2c_smbus_read_word_data(client, mfr_vr_cfg1);
    cfg_data = cfg_data | (0x3 << 6);
    i2c_smbus_write_word_data(client, mfr_vr_cfg1, cfg_data);

    info->R[PSC_TEMPERATURE] = 0;
    info->m[PSC_TEMPERATURE] = 1;
    info->b[PSC_TEMPERATURE] = 0;

    info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IOUT | PMBUS_HAVE_POUT | PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_VOUT |
                    PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP;
    
    info->func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IOUT | PMBUS_HAVE_POUT | PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_VOUT |
                    PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP;

    info->read_byte_data  = mp2886_read_byte_data;
    info->read_word_data  = mp2886_read_word_data;

    if (pmbus_do_probe(client, info) == 0) {
        vol_sensor_add(client);
        curr_sensor_add(client);
        return 0;
    }

    return -1;
}

int mp2886_remove(struct i2c_client *client)
{
    vol_sensor_del(client);
    curr_sensor_del(client);
    return 0;
}

static const struct i2c_device_id mp2886_id[] = {
    {"mp2886", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, mp2886_id);

static const struct of_device_id __maybe_unused mp2886_of_match[] = {
    {.compatible = "mps,mp2886"},
    {}
};
MODULE_DEVICE_TABLE(of, mp2886_of_match);

static struct i2c_driver mp2886_driver = {
    .driver = {
        .name = "mp2886",
        .of_match_table = of_match_ptr(mp2886_of_match),
    },
    .probe = mp2886_probe,
    .remove = mp2886_remove,
    .id_table = mp2886_id,
};

module_i2c_driver(mp2886_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@nvidia.com>");
MODULE_DESCRIPTION("PMBus driver for MPS MP2886 device");
MODULE_LICENSE("GPL");
