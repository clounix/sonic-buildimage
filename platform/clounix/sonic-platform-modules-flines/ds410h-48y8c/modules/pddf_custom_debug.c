#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/kobject.h>
#include "pddf_client_defs.h"

#define BSP_VERSION     ("1.0")

static ssize_t drv_get_syseeprom_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug syseeprom: \n"
                                        "i2cget -y -f 1 0x57 %s\n", "reg");
    return ret;
}

static ssize_t drv_set_syseeprom_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_transceiver_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug transceiver: \n", "reg");
    return ret;
}

static ssize_t drv_set_transceiver_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_temp_sensor_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug temp sensor: \n", "reg");
    return ret;
}

static ssize_t drv_set_temp_sensor_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_curr_sensor_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug curr sensor: \n", "reg");
    return ret;
}

static ssize_t drv_set_curr_sensor_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_volt_sensor_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug volt sensor: \n", "reg");
    return ret;
}

static ssize_t drv_set_volt_sensor_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_fan_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug fan: \n", "reg");
    return ret;
}

static ssize_t drv_set_fan_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_psu_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug psu: \n", "reg");
    return ret;
}

static ssize_t drv_set_psu_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_sysled_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug sysled: \n", "reg");
    return ret;
}

static ssize_t drv_set_sysled_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_fpga_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug fpga: \n", "reg");
    return ret;
}

static ssize_t drv_set_fpga_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_cpld_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug cpld: \n", "reg");
    return ret;
}

static ssize_t drv_set_cpld_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t drv_get_slot_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = -1;
    ret = scnprintf(buf, PAGE_SIZE, "debug slot: \n", "reg");
    return ret;
}

static ssize_t drv_set_slot_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -EOPNOTSUPP;
}

static ssize_t get_sys_bsp_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", BSP_VERSION);
}
static DEVICE_ATTR(bsp_version,S_IRUGO, get_sys_bsp_version, NULL);
static DEVICE_ATTR(syseeprom, S_IRUGO|S_IWUSR, drv_get_syseeprom_debug, drv_set_syseeprom_debug);
static DEVICE_ATTR(transceiver, S_IRUGO|S_IWUSR, drv_get_transceiver_debug, drv_set_transceiver_debug);
static DEVICE_ATTR(temp_sensor, S_IRUGO|S_IWUSR, drv_get_temp_sensor_debug, drv_set_temp_sensor_debug);
static DEVICE_ATTR(curr_sensor, S_IRUGO|S_IWUSR, drv_get_curr_sensor_debug, drv_set_curr_sensor_debug);
static DEVICE_ATTR(volt_sensor, S_IRUGO|S_IWUSR, drv_get_volt_sensor_debug, drv_set_volt_sensor_debug);
static DEVICE_ATTR(fan, S_IRUGO|S_IWUSR, drv_get_fan_debug, drv_set_fan_debug);
static DEVICE_ATTR(psu, S_IRUGO|S_IWUSR, drv_get_psu_debug, drv_set_psu_debug);
static DEVICE_ATTR(sysled, S_IRUGO|S_IWUSR, drv_get_sysled_debug, drv_set_sysled_debug);
static DEVICE_ATTR(fpga, S_IRUGO|S_IWUSR, drv_get_fpga_debug, drv_set_fpga_debug);
static DEVICE_ATTR(cpld, S_IRUGO|S_IWUSR, drv_get_cpld_debug, drv_set_cpld_debug);
static DEVICE_ATTR(slot, S_IRUGO|S_IWUSR, drv_get_slot_debug, drv_set_slot_debug);

static struct attribute *pddf_custom_debug_attributes[] = {
    &dev_attr_bsp_version.attr,
    &dev_attr_syseeprom.attr,
    &dev_attr_transceiver.attr,
    &dev_attr_temp_sensor.attr,
    &dev_attr_curr_sensor.attr,
    &dev_attr_volt_sensor.attr,
    &dev_attr_fan.attr,
    &dev_attr_psu.attr,
    &dev_attr_sysled.attr,
    &dev_attr_fpga.attr,
    &dev_attr_cpld.attr,
    &dev_attr_slot.attr,
    NULL
};

static const struct attribute_group pddf_custom_debug_attribute_group = {
    .attrs = pddf_custom_debug_attributes,
};

static struct kobject *debug_kobj;
static int __init pddf_custom_debug_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;

    pr_info("PDDF CUSTOM DEBUG MODULE... init\n");
    device_kobj = get_device_i2c_kobj();
    if(!device_kobj)
    {
        return -ENOMEM;
    }

    debug_kobj = kobject_create_and_add("custom_debug", device_kobj);
    if(!debug_kobj)
    {
        return -ENOMEM;
    }
    ret = sysfs_create_group(debug_kobj, &pddf_custom_debug_attribute_group);
    if(ret)
    {
        kobject_put(debug_kobj);
        return ret;
    }
    pr_info("create custom i2c client sysfs group\n");

    return ret;
}

void __exit pddf_custom_debug_exit(void)
{
    pr_info("pddf custom debug module.. exit\n");
    sysfs_remove_group(debug_kobj, &pddf_custom_debug_attribute_group);
    kobject_put(debug_kobj);
    pr_info("removed the kobjects for 'custom_debug'\n");
}

module_init(pddf_custom_debug_init);
module_exit(pddf_custom_debug_exit);

MODULE_AUTHOR("FLKS");
MODULE_DESCRIPTION("pddf custom debug");
MODULE_LICENSE("GPL");
