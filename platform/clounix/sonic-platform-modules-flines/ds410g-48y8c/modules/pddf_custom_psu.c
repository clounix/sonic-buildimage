/*******************************************************************************

*  Copyright©[2020-2024] [Hangzhou Clounix Technology Limited]. 

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

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include "pddf_psu_defs.h"
#include "pddf_psu_driver.h"
#include "pddf_client_defs.h"

static int *log_level = &psu_log_level;

#define PSU_REG_VOUT_MODE 0x20
#define PSU_REG_READ_VOUT 0x8b
#define PSU_REG_READ_VOUT_MIN 0xa4
#define PSU_REG_READ_VOUT_MAX 0xa5

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf);
ssize_t pddf_show_custom_psu_v_out_min(struct device *dev, struct device_attribute *da, char *buf);
ssize_t pddf_show_custom_psu_v_out_max(struct device *dev, struct device_attribute *da, char *buf);
extern PSU_SYSFS_ATTR_DATA access_psu_v_out;
extern PSU_SYSFS_ATTR_DATA access_psu_v_out_min;
extern PSU_SYSFS_ATTR_DATA access_psu_v_out_max;
extern PSU_SYSFS_ATTR_DATA access_psu_model_name;
extern PSU_SYSFS_ATTR_DATA access_psu_mfr_id;
extern PSU_SYSFS_ATTR_DATA access_psu_serial_num;

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static u8 psu_get_vout_mode(struct i2c_client *client)
{
    int status = 0, retry = 10;
    uint8_t offset = PSU_REG_VOUT_MODE;

    while (retry) {
        status = i2c_smbus_read_byte_data((struct i2c_client *)client, offset);
        if (unlikely(status < 0)) {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0)
    {
        /*printk(KERN_ERR "%s: Get PSU Vout mode failed\n", __func__);*/
        return 0;
    }
    else
    {
        /*printk(KERN_ERR "%s: vout_mode reg value 0x%x\n", __func__, status);*/
        return status;
    }
}

static u16 psu_get_v_value(struct i2c_client *client, uint8_t offset)
{
    int status = 0, retry = 10;

    while (retry) {
        status = i2c_smbus_read_word_data((struct i2c_client *)client, offset);
        if (unlikely(status < 0)) {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0)
    {
        /*printk(KERN_ERR "%s: Get PSU Vout failed\n", __func__);*/
        return 0;
    }
    else
    {
        /*printk(KERN_ERR "%s: vout reg value 0x%x\n", __func__, status);*/
        return status;
    }
}

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int exponent, mantissa;
    int multiplier = 1000;
    u16 value;
    u8 vout_mode;

    value = psu_get_v_value(client, PSU_REG_READ_VOUT);
    vout_mode = psu_get_vout_mode(client);

    if ((vout_mode >> 5) == 0)
        exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
    else
    {
        /*printk(KERN_ERR "%s: Only support linear mode for vout mode\n", __func__);*/
        exponent = 0;
    }
    mantissa = value;
    if (exponent >= 0)
        return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
    else
        return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

ssize_t pddf_show_custom_psu_v_out_min(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int exponent, mantissa;
    int multiplier = 1000;
    u16 value;
    u8 vout_mode;

    value = psu_get_v_value(client, PSU_REG_READ_VOUT_MIN);
    vout_mode = psu_get_vout_mode(client);

    if ((vout_mode >> 5) == 0)
        exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
    else
    {
        /*printk(KERN_ERR "%s: Only support linear mode for vout mode\n", __func__);*/
        exponent = 0;
    }
    mantissa = value;
    if (exponent >= 0)
        return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
    else
        return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}
ssize_t pddf_show_custom_psu_v_out_max(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int exponent, mantissa;
    int multiplier = 1000;
    u16 value;
    u8 vout_mode;
    
    value = psu_get_v_value(client, PSU_REG_READ_VOUT_MAX);
    vout_mode = psu_get_vout_mode(client);

    if ((vout_mode >> 5) == 0)
        exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
    else
    {
        /*printk(KERN_ERR "%s: Only support linear mode for vout mode\n", __func__);*/
        exponent = 0;
    }
    mantissa = value;
    if (exponent >= 0)
        return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
    else
        return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static int sonic_i2c_get_psu_block_custom(void *client, PSU_DATA_ATTR *adata, void *data)
{
    int status = 0, retry = 10;
    struct psu_attr_info *padata = (struct psu_attr_info *)data;
    char buf[32]="";  //temporary placeholder for block data
    uint8_t offset = (uint8_t)adata->offset;
    int data_len = adata->len;

    while (retry)
    {
        status = i2c_smbus_read_block_data((struct i2c_client *)client, offset, buf);
        if (unlikely(status<0))
        {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0)
    {
        buf[0] = '\0';
        pddf_dbg(PSU, "%s unable to read block of data from (0x%x)\n", dev_name(&((struct i2c_client *)client)->dev), ((struct i2c_client *)client)->addr);
    }
    else
    {
        buf[data_len-1] = '\0';
    }

    if (strncmp(adata->devtype, "pmbus", strlen("pmbus")) == 0)
        strncpy(padata->val.strval, buf+1, data_len-1);
    else
        strncpy(padata->val.strval, buf, data_len);

    pddf_dbg(PSU, "%s: status = %d, buf block: %s\n", __FUNCTION__, status, padata->val.strval);
    return 0;
}

static int __init pddf_custom_psu_init(void)
{
    access_psu_v_out.show = pddf_show_custom_psu_v_out;
    access_psu_v_out_min.show = pddf_show_custom_psu_v_out_min;
    access_psu_v_out_max.show = pddf_show_custom_psu_v_out_max;
    access_psu_v_out.do_get = NULL;

    access_psu_model_name.do_get = sonic_i2c_get_psu_block_custom;
    access_psu_mfr_id.do_get = sonic_i2c_get_psu_block_custom;
    access_psu_serial_num.do_get = sonic_i2c_get_psu_block_custom;
    return 0;
}

static void __exit pddf_custom_psu_exit(void)
{
    return;
}

MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_psu_init);
module_exit(pddf_custom_psu_exit);
