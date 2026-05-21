/*******************************************************************************

*  Copyright©[2020-2024] [Hangzhou Embedway Technology Limited]. 

*  All rights reserved.



*  This program is free software: you can redistribute it and/or modify

*  it under the terms of the GNU General Public License as published by

*  the Free Software Foundation, either version 3 of the License, or

*  (at your option) any later version.



*  This program is distributed in the hope that it will be useful,

*  but WITHOUT ANY WARRANTY; without even the implied warranty of

*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

*  GNU General Public License for more details.



*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 

*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 

*  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 

*  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 

*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 

*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 

*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 

*  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 

*  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 

*  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 

*  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#define __STDC_WANT_LIB_EXT1__ 1
#include "pddf_custom_fan.h"

static struct fan_eeprom_priv *g_fan_priv = NULL;

static int *log_level = &fan_log_level;

extern void *get_device_table(char *name);

int sonic_i2c_set_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info);

extern struct pddf_ops_t pddf_fan_ops;

extern FAN_SYSFS_ATTR_DATA data_fan1_input;
extern FAN_SYSFS_ATTR_DATA data_fan2_input;
extern FAN_SYSFS_ATTR_DATA data_fan3_input;
extern FAN_SYSFS_ATTR_DATA data_fan4_input;
extern FAN_SYSFS_ATTR_DATA data_fan5_input;
extern FAN_SYSFS_ATTR_DATA data_fan6_input;
extern FAN_SYSFS_ATTR_DATA data_fan7_input;
extern FAN_SYSFS_ATTR_DATA data_fan8_input;
extern FAN_SYSFS_ATTR_DATA data_fan9_input;
extern FAN_SYSFS_ATTR_DATA data_fan10_input;
extern FAN_SYSFS_ATTR_DATA data_fan1_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan2_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan3_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan4_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan5_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan6_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan7_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan8_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan9_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan10_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan11_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan12_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan_hw_version;
extern FAN_SYSFS_ATTR_DATA data_fan_model_name;
extern FAN_SYSFS_ATTR_DATA data_fan_serial_num;
extern FAN_SYSFS_ATTR_DATA data_fan_part_num;
extern FAN_SYSFS_ATTR_DATA data_fan1_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan2_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan3_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan4_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan5_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan1_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan2_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan3_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan4_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan5_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan_eeprom_size;

extern FAN_SYSFS_ATTR_DATA data_fan1_sn;
extern FAN_SYSFS_ATTR_DATA data_fan2_sn;
extern FAN_SYSFS_ATTR_DATA data_fan3_sn;
extern FAN_SYSFS_ATTR_DATA data_fan4_sn;
extern FAN_SYSFS_ATTR_DATA data_fan5_sn;

extern FAN_SYSFS_ATTR_DATA data_fan1_pn;
extern FAN_SYSFS_ATTR_DATA data_fan2_pn;
extern FAN_SYSFS_ATTR_DATA data_fan3_pn;
extern FAN_SYSFS_ATTR_DATA data_fan4_pn;
extern FAN_SYSFS_ATTR_DATA data_fan5_pn;

extern FAN_SYSFS_ATTR_DATA data_fan1_model_name;
extern FAN_SYSFS_ATTR_DATA data_fan2_model_name;
extern FAN_SYSFS_ATTR_DATA data_fan3_model_name;
extern FAN_SYSFS_ATTR_DATA data_fan4_model_name;
extern FAN_SYSFS_ATTR_DATA data_fan5_model_name;

extern FAN_SYSFS_ATTR_DATA data_fan1_eeprom;
extern FAN_SYSFS_ATTR_DATA data_fan2_eeprom;
extern FAN_SYSFS_ATTR_DATA data_fan3_eeprom;
extern FAN_SYSFS_ATTR_DATA data_fan4_eeprom;
extern FAN_SYSFS_ATTR_DATA data_fan5_eeprom;

static int fan_eeprom_wait_tx_done(struct i2c_client *client)
{
    unsigned char val = 0;
    unsigned long timeout = jiffies + FAN_EEPROM_I2C_TIMEOUT;

    do {
        val = i2c_smbus_read_byte_data(client, FAN_EEPROM_IIC_STATUS_OFFSET);
        if (val & FAN_EEPROM_TX_FINISH_MASK) {
            if (val & FAN_EEPROM_TX_ERROR_MASK) {
                pddf_err(FAN, "fan-eeprom: TX error status=0x%02x\n", val);
                return -ECOMM;
            }
            return 0;
        }
        usleep_range(5, 10);
    } while (time_before(jiffies, timeout));

    pddf_err(FAN, "fan-eeprom: TX timeout\n");
    return -ETIMEDOUT;
}

static int __fan_eeprom_read_byte_nolock(struct fan_eeprom_priv *priv, u32 offset, u8 *val)
{
    int ret;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_REG_OFFSET, offset);
    if (ret < 0) {
        pddf_dbg(FAN, "write reg offset 0x%02x failed: %d\n", offset, ret);
        return ret;
    }

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_START_OFFSET, FAN_EEPROM_IIC_START_MASK);
    if (ret < 0) {
        pddf_dbg(FAN, "start i2c failed: %d\n", ret);
        return ret;
    }

    ret = fan_eeprom_wait_tx_done(priv->client);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_read_byte_data(priv->client, FAN_EEPROM_BYTE_READ_OFFSET);
    if (ret < 0) {
        pddf_dbg(FAN, "read byte at offset 0x%02x failed: %d\n", offset, ret);
        return ret;
    }

    *val = ret;
    return 0;
}

static int fan_eeprom_read_multi(struct fan_eeprom_priv *priv, u8 fan_idx, u32 start_offset, u8 *buf, u32 len)
{
    int ret = 0;
    u32 i;

    if (!priv || !buf)
        return -EINVAL;
    if (start_offset + len > FAN_EEPROM_MAX_SIZE) {
        pddf_err(FAN, "read out of bounds: offset=0x%02x, len=%d, max=%d\n",
                 start_offset, len, FAN_EEPROM_MAX_SIZE);
        return -EINVAL;
    }

    pddf_dbg(FAN, "read fan %d, offset 0x%02x, len %d\n", fan_idx, start_offset, len);

    mutex_lock(&priv->lock);

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_SELECT_OFFSET, fan_idx);
    if (ret < 0) {
        pddf_err(FAN, "select fan %d failed: %d\n", fan_idx, ret);
        goto err_unlock;
    }

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_DATA_SIZE_OFFSET, 1);
    if (ret < 0)
        goto err_unlock;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_MAGE_OFFSET, 1);
    if (ret < 0)
        goto err_unlock;

    for (i = 0; i < len; i++) {
        u8 val;
        ret = __fan_eeprom_read_byte_nolock(priv, start_offset + i, &val);
        if (ret != 0) {
            pddf_err(FAN, "read byte %d at offset 0x%02x failed: %d\n",
                     i, start_offset + i, ret);
            break;
        }
        buf[i] = val;
    }

err_unlock:
    mutex_unlock(&priv->lock);
    return (ret == 0) ? i : ret;
}

static int __fan_eeprom_write_byte_nolock(struct fan_eeprom_priv *priv, u32 offset, u8 val)
{
    int ret;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_REG_OFFSET, offset);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_BYTE_WRITE_OFFSET, val);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_START_OFFSET, FAN_EEPROM_IIC_START_MASK);
    if (ret < 0)
        return ret;

    return fan_eeprom_wait_tx_done(priv->client);
}

static int fan_eeprom_write_multi(struct fan_eeprom_priv *priv, u8 fan_idx, u32 start_offset, const u8 *buf, u32 len)
{
    int ret = 0;
    u32 i;

    if (!priv || !buf)
        return -EINVAL;
    if (start_offset + len > FAN_EEPROM_MAX_SIZE) {
        pddf_err(FAN, "write out of bounds: offset=0x%02x, len=%d, max=%d\n",
                 start_offset, len, FAN_EEPROM_MAX_SIZE);
        return -EINVAL;
    }

    pddf_dbg(FAN, "write fan %d, offset 0x%02x, len %d\n", fan_idx, start_offset, len);

    mutex_lock(&priv->lock);

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_SELECT_OFFSET, fan_idx);
    if (ret < 0)
        goto err_unlock;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_DATA_SIZE_OFFSET, 0x01);
    if (ret < 0)
        goto err_unlock;

    ret = i2c_smbus_write_byte_data(priv->client, FAN_EEPROM_IIC_MAGE_OFFSET, 0x04);
    if (ret < 0)
        goto err_unlock;

    for (i = 0; i < len; i++) {
        ret = __fan_eeprom_write_byte_nolock(priv, start_offset + i, buf[i]);
        if (ret != 0) {
            pddf_err(FAN, "write byte %d at offset 0x%02x failed: %d\n",
                     i, start_offset + i, ret);
            break;
        }
        usleep_range(10000, 12000);
    }

err_unlock:
    mutex_unlock(&priv->lock);
    return (ret == 0) ? i : ret;
}

static void fan_eeprom_copy_str(u8 *data, char *out, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        u8 c = data[i];
        out[i] = (c >= 32 && c <= 126) ? c : '.';
    }
    out[len] = '\0';
}

static int fan_eeprom_read_str(struct fan_eeprom_priv *priv, u8 fan_idx, u32 offset, u32 len, char *buf)
{
    u8 data[32];
    int ret;

    if (len >= sizeof(data)) {
        pddf_err(FAN, "string length %d exceeds buffer size %zu\n", len, sizeof(data));
        return -EINVAL;
    }

    ret = fan_eeprom_read_multi(priv, fan_idx, offset, data, len);
    if (ret != len) {
        pddf_err(FAN, "read string failed: expected %d bytes, got %d\n", len, ret);
        return -EIO;
    }

    fan_eeprom_copy_str(data, buf, len);
    return 0;
}

static ssize_t fan_eeprom_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
    struct i2c_client *client = to_i2c_client(dev);
    struct fan_data *data = i2c_get_clientdata(client);
    FAN_PDATA *pdata = (FAN_PDATA *)(client->dev.platform_data);
    FAN_DATA_ATTR *usr_data = NULL;
    int i, fan_idx;
    int ret;

    if (!g_fan_priv) {
        pddf_err(FAN, "eeprom not initialized yet\n");
        return -ENODEV;
    }

    for (i = 0; i < data->num_attr; i++) {
        if (strcmp(sattr->dev_attr.attr.name, pdata->fan_attrs[i].aname) == 0) {
            usr_data = &pdata->fan_attrs[i];
            break;
        }
    }

    if (!usr_data) {
        pddf_err(FAN, "attribute %s not found in JSON\n", sattr->dev_attr.attr.name);
        return -EINVAL;
    }

    if      (strcmp(sattr->dev_attr.attr.name, "fan1_eeprom") == 0) fan_idx = 0;
    else if (strcmp(sattr->dev_attr.attr.name, "fan2_eeprom") == 0) fan_idx = 1;
    else if (strcmp(sattr->dev_attr.attr.name, "fan3_eeprom") == 0) fan_idx = 2;
    else if (strcmp(sattr->dev_attr.attr.name, "fan4_eeprom") == 0) fan_idx = 3;
    else if (strcmp(sattr->dev_attr.attr.name, "fan5_eeprom") == 0) fan_idx = 4;
    else return -EINVAL;

    pddf_dbg(FAN, "eeprom_show: attr=%s, fan=%d, offset=0x%02x, len=%d\n",
             usr_data->aname, fan_idx, usr_data->offset, usr_data->len);

    ret = fan_eeprom_read_multi(g_fan_priv, fan_idx, usr_data->offset, buf, usr_data->len);
    if (ret < 0) {
        pddf_err(FAN, "eeprom read failed: %d\n", ret);
        return ret;
    }

    return ret;
}

static ssize_t fan_eeprom_store(struct device *dev, struct device_attribute *attr,
                                const char *buf, size_t count)
{
    struct sensor_device_attribute *sattr = to_sensor_dev_attr(attr);
    struct i2c_client *client = to_i2c_client(dev);
    struct fan_data *data = i2c_get_clientdata(client);
    FAN_PDATA *pdata = (FAN_PDATA *)(client->dev.platform_data);
    FAN_DATA_ATTR *usr_data = NULL;
    int i, fan_idx;
    int ret;

    if (!g_fan_priv)
        return -ENODEV;

    for (i = 0; i < data->num_attr; i++) {
        if (strcmp(sattr->dev_attr.attr.name, pdata->fan_attrs[i].aname) == 0) {
            usr_data = &pdata->fan_attrs[i];
            break;
        }
    }

    if (!usr_data) {
        pddf_err(FAN, "attribute %s not found in JSON\n", sattr->dev_attr.attr.name);
        return -EINVAL;
    }

    if      (strcmp(sattr->dev_attr.attr.name, "fan1_eeprom") == 0) fan_idx = 0;
    else if (strcmp(sattr->dev_attr.attr.name, "fan2_eeprom") == 0) fan_idx = 1;
    else if (strcmp(sattr->dev_attr.attr.name, "fan3_eeprom") == 0) fan_idx = 2;
    else if (strcmp(sattr->dev_attr.attr.name, "fan4_eeprom") == 0) fan_idx = 3;
    else if (strcmp(sattr->dev_attr.attr.name, "fan5_eeprom") == 0) fan_idx = 4;
    else return -EINVAL;

    if (count > usr_data->len)
        count = usr_data->len;

    pddf_dbg(FAN, "eeprom_store: attr=%s, fan=%d, offset=0x%02x, len=%zu\n",
             usr_data->aname, fan_idx, usr_data->offset, count);

    ret = fan_eeprom_write_multi(g_fan_priv, fan_idx, usr_data->offset, buf, count);
    if (ret < 0) {
        pddf_err(FAN, "eeprom write failed: %d\n", ret);
        return ret;
    }

    return ret;
}

int sonic_i2c_set_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    int val = 0;
    int reg_val = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    val = painfo->val.intval & udata->mask;

    if (val > 255)
    {
        return -EINVAL;
    }

    /*clounix EVB max PWM is 100*/
    if(val > 100)
    {
        val = 100;
    }

    if (strncmp(udata->devtype, "fpgapci", strlen("fpgapci")) == 0)
    {
        reg_val = ptr_fpgapci_read(udata->devaddr);
        reg_val &= ~(udata->mask << udata->offset);
        reg_val |= (val << udata->offset);
        status = ptr_fpgapci_write(udata->devaddr,reg_val);
    }
    else
    {
        if (udata->len == 1)
            status = i2c_smbus_write_byte_data(client, udata->offset, val);
        else if (udata->len == 2)
        {
            uint8_t val_lsb = val & 0xFF;
            uint8_t val_hsb = (val >> 8) & 0xFF;
            status = i2c_smbus_write_byte_data(client, udata->offset, val_lsb);
            if (status == 0) status = i2c_smbus_write_byte_data(client, udata->offset+1, val_hsb);
        }
        else
        {
            pddf_err(FAN, "PDDF_FAN_ERROR: %s: pwm should be of len 1/2 bytes. Not setting the pwm as the length is %d\n", __FUNCTION__, udata->len);
        }
    }

    return status;
}

static int sonic_i2c_get_fan_hw_version(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    int val = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;


    val = i2c_smbus_read_byte_data((struct i2c_client *)client, udata->offset);
    
    if (val < 0)
        status = val;
    else
        painfo->val.intval = val & udata->mask;
    return status;
}

static int sonic_i2c_get_fan_rpm(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    int val = 0;
    int val_l = 0, val_h = 0;
    bool skip_neg_check = false;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    if (udata->len == 1)
    {
        val_l = i2c_smbus_read_byte_data((struct i2c_client *)client, udata->offset);
        val_h = i2c_smbus_read_byte_data((struct i2c_client *)client, udata->offset+1);
        val = (val_h << 8) + val_l;
    }
    else if (udata->len ==2)
    {
        val = i2c_smbus_read_word_swapped((struct i2c_client *)client, udata->offset);
    }
    else
    {
        return -EINVAL;
    }

    if (!skip_neg_check && val < 0) {
        status = val;
    } else {
        if (udata->is_divisor) {
            int divisor = val >> 3;
            if (divisor == 0) {
                pddf_err(FAN, "%s: failed to calculate fan rpm, divisor is 0\n", __FUNCTION__);
                return -1;
            } else if (divisor < 0) {
                painfo->val.intval = 0;
            } else {
                painfo->val.intval = udata->mult / divisor;
            }
        } else {
            painfo->val.intval = udata->mult * val;
        }
    }
    return status;
}

static ssize_t show_fan_string(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct fan_data *data = i2c_get_clientdata(client);
    FAN_PDATA *pdata = (FAN_PDATA *)(client->dev.platform_data);
    FAN_DATA_ATTR *usr_data = NULL;
    struct fan_attr_info *attr_info = NULL;
    int i;
    char tmp[32] = "N/A";
    u8 fan_idx = 0;
    int ret;

    if (!g_fan_priv) {
        pddf_err(FAN, "%s: eeprom not initialized\n", __FUNCTION__);
        return sprintf(buf, "N/A\n");
    }

    for (i=0;i<data->num_attr;i++)
    {
        if (strcmp(attr->dev_attr.attr.name, pdata->fan_attrs[i].aname) == 0)
        {
            attr_info = &data->attr_info[i];
            usr_data = &pdata->fan_attrs[i];
            break;
        }
    }

    if (attr_info == NULL || usr_data == NULL)
    {
        pddf_err(FAN, "%s: attribute %s not supported\n", __FUNCTION__, attr->dev_attr.attr.name);
        return sprintf(buf, "N/A\n");
    }

    if      (strstr(attr->dev_attr.attr.name, "fan1")) fan_idx = 0;
    else if (strstr(attr->dev_attr.attr.name, "fan2")) fan_idx = 1;
    else if (strstr(attr->dev_attr.attr.name, "fan3")) fan_idx = 2;
    else if (strstr(attr->dev_attr.attr.name, "fan4")) fan_idx = 3;
    else if (strstr(attr->dev_attr.attr.name, "fan5")) fan_idx = 4;

    pddf_dbg(FAN, "show_fan_string: attr=%s, fan=%d, offset=0x%02x, len=%d\n",
             usr_data->aname, fan_idx, usr_data->offset, usr_data->len);

    mutex_lock(&attr_info->update_lock);

    if (time_after(jiffies, attr_info->last_updated + HZ + HZ / 2) || !attr_info->valid)
    {
        attr_info->valid = 0;
        
        ret = fan_eeprom_read_str(g_fan_priv, fan_idx, usr_data->offset, usr_data->len, tmp);
        if (ret != 0) {
            pddf_err(FAN, "read %s for fan %d failed, using N/A\n", usr_data->aname, fan_idx);
            strncpy(tmp, "N/A", sizeof(tmp) - 1);
        } else {
            pddf_dbg(FAN, "read success: value='%s'\n", tmp);
        }

        strncpy(attr_info->val.strval, tmp, sizeof(attr_info->val.strval) - 1);
        attr_info->last_updated = jiffies;
        attr_info->valid = 1;
    }

    mutex_unlock(&attr_info->update_lock);

    return sprintf(buf, "%s\n", attr_info->val.strval);
}

static int get_fan_speed_tolerance(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    painfo->val.intval = FAN_MAX_SPEED_DS410G / 10;

    return status;
}

static int get_fan_speed_target(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    int val = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    val = i2c_smbus_read_byte_data((struct i2c_client *)client, udata->offset);
    if (val < 0)
        status = val;
    else
        painfo->val.intval = val * FAN_MAX_SPEED_DS410G / 100;
 
    return status;
}

static int fan_post_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    struct fan_eeprom_priv *priv;
    int ret = 0;

    pddf_info(FAN, "fan_post_probe called for device %s\n", dev_name(&client->dev));

    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        pddf_err(FAN, "Failed to allocate eeprom private data\n");
        ret = 0;
        goto out;
    }

    mutex_init(&priv->lock);
    priv->client = client;
    g_fan_priv = priv;

    pddf_info(FAN, "fan-eeprom: initialized successfully, priv=%pK\n", g_fan_priv);

out:
    pddf_info(FAN, "fan_post_probe returning %d\n", ret);
    return ret;
}

static int fan_post_remove(struct i2c_client *client)
{
    if (g_fan_priv) {
        mutex_destroy(&g_fan_priv->lock);
        kfree(g_fan_priv);
        g_fan_priv = NULL;
        
        pddf_info(FAN, "fan-eeprom: cleaned up for device %s\n", dev_name(&client->dev));
    }

    return 0;
}

static int __init pddf_custom_fan_init(void)
{
    pddf_info(FAN, "pddf_custom_fan_init called (late init)\n");

    data_fan1_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan2_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan3_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan4_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan5_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan6_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan7_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan8_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan9_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan10_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan11_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan12_pwm.do_set = sonic_i2c_set_fan_pwm_custom;

    data_fan_hw_version.show = fan_show_default;
    data_fan_hw_version.do_get = sonic_i2c_get_fan_hw_version;
    data_fan1_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan2_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan3_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan4_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan5_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan6_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan7_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan8_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan9_input.do_get = sonic_i2c_get_fan_rpm;
    data_fan10_input.do_get = sonic_i2c_get_fan_rpm;

    data_fan_model_name.show = show_fan_string;
    data_fan_serial_num.show = show_fan_string;
    data_fan_part_num.show = show_fan_string;

    data_fan1_speed_tolerance.do_get = get_fan_speed_tolerance;
    data_fan2_speed_tolerance.do_get = get_fan_speed_tolerance;
    data_fan3_speed_tolerance.do_get = get_fan_speed_tolerance;
    data_fan4_speed_tolerance.do_get = get_fan_speed_tolerance;
    data_fan5_speed_tolerance.do_get = get_fan_speed_tolerance;
    data_fan1_speed_target.do_get = get_fan_speed_target;
    data_fan2_speed_target.do_get = get_fan_speed_target;
    data_fan3_speed_target.do_get = get_fan_speed_target;
    data_fan4_speed_target.do_get = get_fan_speed_target;
    data_fan5_speed_target.do_get = get_fan_speed_target;

    data_fan1_eeprom.show = fan_eeprom_show;
    data_fan1_eeprom.store = fan_eeprom_store;
    data_fan2_eeprom.show = fan_eeprom_show;
    data_fan2_eeprom.store = fan_eeprom_store;
    data_fan3_eeprom.show = fan_eeprom_show;
    data_fan3_eeprom.store = fan_eeprom_store;
    data_fan4_eeprom.show = fan_eeprom_show;
    data_fan4_eeprom.store = fan_eeprom_store;
    data_fan5_eeprom.show = fan_eeprom_show;
    data_fan5_eeprom.store = fan_eeprom_store;

    data_fan1_model_name.show = show_fan_string;
    data_fan2_model_name.show = show_fan_string;
    data_fan3_model_name.show = show_fan_string;
    data_fan4_model_name.show = show_fan_string;
    data_fan5_model_name.show = show_fan_string;

    data_fan1_sn.show = show_fan_string;
    data_fan2_sn.show = show_fan_string;
    data_fan3_sn.show = show_fan_string;
    data_fan4_sn.show = show_fan_string;
    data_fan5_sn.show = show_fan_string;

    data_fan1_pn.show = show_fan_string;
    data_fan2_pn.show = show_fan_string;
    data_fan3_pn.show = show_fan_string;
    data_fan4_pn.show = show_fan_string;
    data_fan5_pn.show = show_fan_string;

    pddf_fan_ops.post_probe = fan_post_probe;
    pddf_fan_ops.post_remove = fan_post_remove;

    pddf_info(FAN, "pddf_fan_ops.post_probe set to %pK\n", pddf_fan_ops.post_probe);
    pddf_info(FAN, "pddf_custom_fan_init completed successfully\n");

    return 0;
}

static void __exit pddf_custom_fan_exit(void)
{
    return;
}

MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("pddf custom fan api with eeprom support");
MODULE_LICENSE("GPL");

module_init(pddf_custom_fan_init);
module_exit(pddf_custom_fan_exit);