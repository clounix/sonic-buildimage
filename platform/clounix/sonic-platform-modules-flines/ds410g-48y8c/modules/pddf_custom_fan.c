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

static int *log_level = &fan_log_level;

extern void *get_device_table(char *name);

int sonic_i2c_set_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info);

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
            /* TODO: Check this logic for LE and BE */
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
    pddf_dbg(FAN, "FAN val: %#x, updata->mask: %#x, painfo->val.intval: %#x\n", val, udata->mask, painfo->val.intval);
    return status;
}

static int sonic_i2c_get_fan_rpm(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    int val = 0;
    int val_l = 0, val_h = 0;
    uint32_t offset = 0;
    bool skip_neg_check = false;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    // if (udata->offset >= 0x70 && udata->offset <= 0x7f) {
    //     offset = udata->offset + 0x10;
    // } else if (udata->offset >= 0x80 && udata->offset <= 0x8f) {
    //     offset = udata->offset - 0x10;
    // }
    if (udata->len == 1)
    {
        val_l = i2c_smbus_read_byte_data((struct i2c_client *)client, udata->offset);
        val_h = i2c_smbus_read_byte_data((struct i2c_client *)client, udata->offset+1);
        val = (val_h << 8) + val_l;
        // val_l = i2c_smbus_read_byte_data((struct i2c_client *)client, offset);
        // val_h = i2c_smbus_read_byte_data((struct i2c_client *)client, offset+1);
        // val += ((val_h << 8) + val_l);
    }
    else if (udata->len ==2)
    {
        val = i2c_smbus_read_word_swapped((struct i2c_client *)client, udata->offset);
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
    pddf_dbg(FAN, "%s, %d, offset: %x, val: %x, is_divisor: %#x, mult: %#x, intval: %#x\n", __FUNCTION__, __LINE__, udata->offset,
                                    val, udata->is_divisor, udata->mult, painfo->val.intval);
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
    int i, status = 0;
	char new_str[ATTR_NAME_LEN] = "";
    char model_name[32] = "DFTA0456B2UP209\n";
    char serial_num[32] = "N/A\n";

    for (i=0;i<data->num_attr;i++)
    {
        if (strcmp(attr->dev_attr.attr.name, pdata->fan_attrs[i].aname) == 0 || strcmp(attr->dev_attr.attr.name, new_str) == 0)
        {
			attr_info = &data->attr_info[i];
            usr_data = &pdata->fan_attrs[i];
			strcpy(new_str, "");
        }
    }

    if (attr_info==NULL || usr_data==NULL)
    {
        pddf_err(FAN, "%s is not supported attribute for this client\n", usr_data->aname);
        goto exit;
    }

    mutex_lock(&attr_info->update_lock);

    if (time_after(jiffies, attr_info->last_updated + HZ + HZ / 2) || !attr_info->valid)
	{
        attr_info->valid = 0;
        if (attr->index == FAN_MODEL_NAME ||!strcmp(usr_data->aname, "fan_model_name"))
            strncpy(attr_info->val.strval, model_name, usr_data->len);
        else if (attr->index == FAN_SERIAL_NUM || !strcmp(usr_data->aname, "fan_serial_num"))
            strncpy(attr_info->val.strval, serial_num, usr_data->len);
        else if (attr->index == FAN_PART_NUM || !strcmp(usr_data->aname, "fan_part_num"))
            strncpy(attr_info->val.strval, serial_num, usr_data->len);

        attr_info->last_updated = jiffies;
        attr_info->valid = 1;
    }

    mutex_unlock(&attr_info->update_lock);

	/*Decide the o/p based on attribute type */
	switch(attr->index)
	{
		case FAN_MODEL_NAME:
		case FAN_SERIAL_NUM:
		case FAN_PART_NUM:
		case FAN_HW_VERSION:
            return sprintf(buf, "%s\n", attr_info->val.strval);
            break;
		default:
            pddf_err(FAN, "%s: Unable to find the attribute index for %s\n", __FUNCTION__, usr_data->aname);
			status = 0;
	}

exit:
    /* Even if there is an error, strval is having empty string */
    return sprintf(buf, "%s\n", attr_info->val.strval);
}

static int get_fan_speed_tolerance(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    painfo->val.intval = FAN_MAX_SPEED_DS410G / 10;

    pddf_dbg(FAN, "%s, %d, intval: %#x\n", __FUNCTION__, __LINE__, painfo->val.intval);
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
 
    pddf_dbg(FAN, "%s, %d, offset: %#x, intval: %#x\n", __FUNCTION__, __LINE__, udata->offset, painfo->val.intval);
    return status;
}

static int __init pddf_custom_fan_init(void)
{
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

    return 0;
}

static void __exit pddf_custom_fan_exit(void)
{
    return;
}

MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("pddf custom fan api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_fan_init);
module_exit(pddf_custom_fan_exit);