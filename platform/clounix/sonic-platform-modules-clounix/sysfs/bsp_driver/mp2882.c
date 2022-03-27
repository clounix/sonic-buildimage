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

struct mp2882_data {
    struct pmbus_driver_info info;
    struct pmbus_sensor *sensor;
    int total_curr_resolution;
    int phase_curr_resolution;
    int curr_sense_gain;
};

/*
struct range not_allow_range[] = {
    {0x2, 0x2},
    {0x4, 0xf},
    {0x11, 0x12},
    {0x14, 0x15},
    {0x17, 0x20},
    {0x29, 0x2a},
    {0x2c, 0x32},
    {0x34, 0x45},
    {0x48, 0x49},
    {0x4b, 0x4e},
    {0x50, 0x50},
    {0x52, 0x54},
    {0x56, 0x60},
    {0x62, 0x77},
    {0x7f, 0x82},
    {0x87, 0x87},
    {0x89, 0x8a},
    {0x8e, 0x90},
    {0x92, 0x95},
    {0x97, 0x97},
    {0x9e, 0x9e},
    {0xcf, 0xcf},
    {0x0, 0x0},
};
*/
#define to_mp2882_data(x)  container_of(x, struct mp2882_data, info)

static int mp2882_read_byte_data(struct i2c_client *client, int page, int reg)
{
    return -ENODATA;
}

#define mfr_vout_loop_ctrl_r1 0xb2
#define mfr_vr_cfg1 0xb7
static unsigned short process_vout(struct i2c_client *client, int page, int reg)
{
    unsigned short mfr_vout_cfg;
    unsigned int vout;
    int gain_para[4][2] = {
        {6250, 5},
        {6250, 5},
        {1953, 5},
        {3906, 5},
    };
    int gain_sel;
    int vid_step_sel;

    mfr_vout_cfg = pmbus_read_word_data(client, page, mfr_vout_loop_ctrl_r1);
    vout = pmbus_read_word_data(client, page, reg);
    gain_sel = (mfr_vout_cfg >> 10) & 0x3;
    vid_step_sel = (mfr_vout_cfg >> 14) & 0x3;

    if (vid_step_sel > 1)
        return -ENODATA;

    vout = vout * gain_para[gain_sel][vid_step_sel];
    if (vid_step_sel == 0)
        vout = vout/1000;

    return vout;
}

static unsigned short process_power(struct i2c_client *client, int page, int reg)
{
    unsigned short mfr_vr_cfg;
    unsigned short power;
    unsigned char modulus[] = {2, 3};

    mfr_vr_cfg = pmbus_read_word_data(client, 0, mfr_vr_cfg1);
    mfr_vr_cfg = (mfr_vr_cfg >> 6) & 0x1;
    power = pmbus_read_word_data(client, page, reg);
    power = power >> modulus[mfr_vr_cfg];

    return power;
}

static unsigned int process_iout(struct i2c_client *client, int page, int reg)
{
    unsigned int data;
    unsigned int exp;

    data = pmbus_read_word_data(client, page, reg);
    exp = data >> 11;
    data = data & 0x7ff;

    if (exp == 0x1f)
        data = data / 2;
    else if (exp == 0x1e)
        data = data / 4;
    else
        data = 0;

    return data;
}

static int mp2882_read_word_data(struct i2c_client *client, int page, int reg)
{
    switch (reg) {
        case PMBUS_READ_IOUT:
            return process_iout(client, page, reg);

        case PMBUS_READ_VOUT:
            return process_vout(client, page, reg);

        case PMBUS_READ_POUT:
            return process_power(client, page, reg);

        default:
            break;
    }
    return -ENODATA;
}

#if 0
static int
mp2882_current_sense_gain_and_resolution_get(struct i2c_client *client, struct mp2882_data *data)
{
    int ret;

    /*
     * Obtain DrMOS current sense gain of power stage from the register
     * , bits 0-2. The value is selected as below:
     * 00b - 5µA/A, 01b - 8.5µA/A, 10b - 9.7µA/A, 11b - 10µA/A. Other
     * values are reserved.
     */
    ret = i2c_smbus_read_word_data(client, MP2888_MFR_SYS_CONFIG);
    if (ret < 0)
        return ret;

    switch (ret & MP2888_DRMOS_KCS) {
    case 0:
        data->curr_sense_gain = 85;
        break;
    case 1:
        data->curr_sense_gain = 97;
        break;
    case 2:
        data->curr_sense_gain = 100;
        break;
    case 3:
        data->curr_sense_gain = 50;
        break;
    default:
        return -EINVAL;
    }

    /*
     * Obtain resolution selector for total and phase current report and protection.
     * 0: original resolution; 1: half resolution (in such case phase current value should
     * be doubled.
     */
    data->total_curr_resolution = (ret & MP2888_TOTAL_CURRENT_RESOLUTION) >> 3;
    data->phase_curr_resolution = (ret & MP2888_PHASE_CURRENT_RESOLUTION) >> 4;

    return 0;
}

static int
mp2882_read_phase(struct i2c_client *client, struct mp2882_data *data, int page, int phase, u8 reg)
{
    int ret;

    ret = pmbus_read_word_data(client, page, phase, reg);
    if (ret < 0)
        return ret;

    if (!((phase + 1) % 2))
        ret >>= 8;
    ret &= 0xff;

    /*
     * Output value is calculated as: (READ_CSx / 80 – 1.23) / (Kcs * Rcs)
     * where:
     * - Kcs is the DrMOS current sense gain of power stage, which is obtained from the
     *   register MP2888_MFR_VR_CONFIG1, bits 13-12 with the following selection of DrMOS
     *   (data->curr_sense_gain):
     *   00b - 5µA/A, 01b - 8.5µA/A, 10b - 9.7µA/A, 11b - 10µA/A.
     * - Rcs is the internal phase current sense resistor. This parameter depends on hardware
     *   assembly. By default it is set to 1kΩ. In case of different assembly, user should
     *   scale this parameter by dividing it by Rcs.
     * If phase current resolution bit is set to 1, READ_CSx value should be doubled.
     * Note, that current phase sensing, providing by the device is not accurate. This is
     * because sampling of current occurrence of bit weight has a big deviation, especially for
     * light load.
     */
    ret = DIV_ROUND_CLOSEST(ret * 100 - 9800, data->curr_sense_gain);
    ret = (data->phase_curr_resolution) ? ret * 2 : ret;
    /* Scale according to total current resolution. */
    ret = (data->total_curr_resolution) ? ret * 8 : ret * 4;
    return ret;
}

static int
mp2882_read_phases(struct i2c_client *client, struct mp2882_data *data, int page, int phase)
{
    int ret;

    switch (phase) {
    case 0 ... 1:
        ret = mp2882_read_phase(client, data, page, phase, MP2888_MFR_READ_CS1_2);
        break;
    case 2 ... 3:
        ret = mp2882_read_phase(client, data, page, phase, MP2888_MFR_READ_CS3_4);
        break;
    case 4 ... 5:
        ret = mp2882_read_phase(client, data, page, phase, MP2888_MFR_READ_CS5_6);
        break;
    case 6 ... 7:
        ret = mp2882_read_phase(client, data, page, phase, MP2888_MFR_READ_CS7_8);
        break;
    case 8 ... 9:
        ret = mp2882_read_phase(client, data, page, phase, MP2888_MFR_READ_CS9_10);
        break;
    default:
        return -ENODATA;
    }
    return ret;
}

static int mp2882_read_word_data_mp2888(struct i2c_client *client, int page, int phase, int reg)
{
    const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
    struct mp2882_data *data = to_mp2882_data(info);
    int ret;

    switch (reg) {
    case PMBUS_READ_VIN:
        ret = pmbus_read_word_data(client, page, phase, reg);
        if (ret <= 0)
            return ret;

        /*
         * READ_VIN requires fixup to scale it to linear11 format. Register data format
         * provides 10 bits for mantissa and 6 bits for exponent. Bits 15:10 are set with
         * the fixed value 111011b.
         */
        ret = (ret & GENMASK(9, 0)) | ((ret & GENMASK(31, 10)) << 1);
        break;
    case PMBUS_OT_WARN_LIMIT:
        ret = pmbus_read_word_data(client, page, phase, reg);
        if (ret < 0)
            return ret;
        /*
         * Chip reports limits in degrees C, but the actual temperature in 10th of
         * degrees C - scaling is needed to match both.
         */
        ret *= MP2888_TEMP_UNIT;
        break;
    case PMBUS_READ_IOUT:
        if (phase != 0xff)
            return mp2882_read_phases(client, data, page, phase);

        ret = pmbus_read_word_data(client, page, phase, reg);
        if (ret < 0)
            return ret;
        /*
         * READ_IOUT register has unused bits 15:12 with fixed value 1110b. Clear these
         * bits and scale with total current resolution. Data is provided in direct format.
         */
        ret &= GENMASK(11, 0);
        ret = data->total_curr_resolution ? ret * 2 : ret;
        break;
    case PMBUS_IOUT_OC_WARN_LIMIT:
        ret = pmbus_read_word_data(client, page, phase, reg);
        if (ret < 0)
            return ret;
        ret &= GENMASK(9, 0);
        /*
         * Chip reports limits with resolution 1A or 2A, if total current resolution bit is
         * set 1. Actual current is reported with 0.25A or respectively 0.5A resolution.
         * Scaling is needed to match both.
         */
        ret = data->total_curr_resolution ? ret * 8 : ret * 4;
        break;
    case PMBUS_READ_POUT:
    case PMBUS_READ_PIN:
        ret = pmbus_read_word_data(client, page, phase, reg);
        if (ret < 0)
            return ret;
        ret = data->total_curr_resolution ? ret * 2 : ret;
        break;
    case PMBUS_POUT_OP_WARN_LIMIT:
        ret = pmbus_read_word_data(client, page, phase, reg);
        if (ret < 0)
            return ret;
        /*
         * Chip reports limits with resolution 1W or 2W, if total current resolution bit is
         * set 1. Actual power is reported with 0.5W or 1W respectively resolution. Scaling
         * is needed to match both.
         */
        ret = data->total_curr_resolution ? ret * 4 : ret * 2;
        break;
    /*
     * The below registers are not implemented by device or implemented not according to the
     * spec. Skip all of them to avoid exposing non-relevant inputs to sysfs.
     */
    case PMBUS_OT_FAULT_LIMIT:
    case PMBUS_UT_WARN_LIMIT:
    case PMBUS_UT_FAULT_LIMIT:
    case PMBUS_VIN_UV_FAULT_LIMIT:
    case PMBUS_VOUT_UV_WARN_LIMIT:
    case PMBUS_VOUT_OV_WARN_LIMIT:
    case PMBUS_VOUT_UV_FAULT_LIMIT:
    case PMBUS_VOUT_OV_FAULT_LIMIT:
    case PMBUS_VIN_OV_WARN_LIMIT:
    case PMBUS_IOUT_OC_LV_FAULT_LIMIT:
    case PMBUS_IOUT_OC_FAULT_LIMIT:
    case PMBUS_POUT_MAX:
    case PMBUS_IOUT_UC_FAULT_LIMIT:
    case PMBUS_POUT_OP_FAULT_LIMIT:
    case PMBUS_PIN_OP_WARN_LIMIT:
    case PMBUS_MFR_VIN_MIN:
    case PMBUS_MFR_VOUT_MIN:
    case PMBUS_MFR_VIN_MAX:
    case PMBUS_MFR_VOUT_MAX:
    case PMBUS_MFR_IIN_MAX:
    case PMBUS_MFR_IOUT_MAX:
    case PMBUS_MFR_PIN_MAX:
    case PMBUS_MFR_POUT_MAX:
    case PMBUS_MFR_MAX_TEMP_1:
        return -ENXIO;
    default:
        return -ENODATA;
    }

    return ret;
}

static int mp2882_write_word_data(struct i2c_client *client, int page, int reg, u16 word)
{
    const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
    struct mp2882_data *data = to_mp2882_data(info);

    switch (reg) {
    case PMBUS_OT_WARN_LIMIT:
        word = DIV_ROUND_CLOSEST(word, MP2888_TEMP_UNIT);
        /* Drop unused bits 15:8. */
        word = clamp_val(word, 0, GENMASK(7, 0));
        break;
    case PMBUS_IOUT_OC_WARN_LIMIT:
        /* Fix limit according to total curent resolution. */
        word = data->total_curr_resolution ? DIV_ROUND_CLOSEST(word, 8) :
               DIV_ROUND_CLOSEST(word, 4);
        /* Drop unused bits 15:10. */
        word = clamp_val(word, 0, GENMASK(9, 0));
        break;
    case PMBUS_POUT_OP_WARN_LIMIT:
        /* Fix limit according to total curent resolution. */
        word = data->total_curr_resolution ? DIV_ROUND_CLOSEST(word, 4) :
               DIV_ROUND_CLOSEST(word, 2);
        /* Drop unused bits 15:10. */
        word = clamp_val(word, 0, GENMASK(9, 0));
        break;
    default:
        return -ENODATA;
    }
    return pmbus_write_word_data(client, page, reg, word);
}

static int
mp2882_identify_multiphase(struct i2c_client *client, struct mp2882_data *data,
               struct pmbus_driver_info *info)
{
    int ret;

    ret = i2c_smbus_write_byte_data(client, PMBUS_PAGE, 0);
    if (ret < 0)
        return ret;

    /* Identify multiphase number - could be from 1 to 10. */
    ret = i2c_smbus_read_word_data(client, MP2888_MFR_VR_CONFIG1);
    if (ret <= 0)
        return ret;

    info->phases[0] = ret & GENMASK(3, 0);

    /*
     * The device provides a total of 10 PWM pins, and can be configured to different phase
     * count applications for rail.
     */
    if (info->phases[0] > MP2888_MAX_PHASE)
        return -EINVAL;

    return 0;
}
#endif

static struct pmbus_platform_data mp2882_pdata = {0};
static struct pmbus_driver_info mp2882_info = {0};

extern int vol_sensor_add(struct i2c_client *client);
extern void vol_sensor_del(struct i2c_client *client);
extern int curr_sensor_add(struct i2c_client *client);
extern void curr_sensor_del(struct i2c_client *client);

static int mp2882_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct pmbus_driver_info *info;
    struct mp2882_data *data;
    unsigned short cfg_data;

    data = devm_kzalloc(&client->dev, sizeof(struct mp2882_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    client->dev.platform_data = &mp2882_pdata;
    mp2882_pdata.flags = PMBUS_SKIP_STATUS_CHECK;

    memcpy(&data->info, &mp2882_info, sizeof(*info));
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

    info->read_byte_data  = mp2882_read_byte_data;
    info->read_word_data  = mp2882_read_word_data;

    if (pmbus_do_probe(client, id, info) == 0) {
        vol_sensor_add(client);
        curr_sensor_add(client);
        return 0;
    }

    return -1;
}

int mp2882_remove(struct i2c_client *client)
{
    vol_sensor_del(client);
    curr_sensor_del(client);
    pmbus_do_remove(client);

    return 0;
}

static const struct i2c_device_id mp2882_id[] = {
    {"mp2882", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, mp2882_id);

static const struct of_device_id __maybe_unused mp2882_of_match[] = {
    {.compatible = "mps,mp2882"},
    {}
};
MODULE_DEVICE_TABLE(of, mp2882_of_match);

static struct i2c_driver mp2882_driver = {
    .driver = {
        .name = "mp2882",
        .of_match_table = of_match_ptr(mp2882_of_match),
    },
    .probe = mp2882_probe,
    .remove = mp2882_remove,
    .id_table = mp2882_id,
};

module_i2c_driver(mp2882_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@nvidia.com>");
MODULE_DESCRIPTION("PMBus driver for MPS MP2882 device");
MODULE_LICENSE("GPL");
