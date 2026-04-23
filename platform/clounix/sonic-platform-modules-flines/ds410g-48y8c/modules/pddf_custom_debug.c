#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/string.h>
#include "pddf_client_defs.h"

ssize_t sys_bsp_version_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *bsp_version = "1.0";

    return sprintf(buf, "%s\n", bsp_version);
}

ssize_t sys_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "read eeprom: \n"
                        "hexdump -C /sys_switch/syseeprom\n");
}

ssize_t sys_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t xcvr_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "read transciever info: \n"
                        "cat /sys_switch/transceiver/present\n"
                        "hexdump -C /sys_switch/transceiver/eth1/eeprom\n");
}

ssize_t xcvr_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t temp_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "read temp_sensor info: \n"
                        "cat /sys_switch/temp_sensor/temp*/*\n");
}

ssize_t temp_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t curr_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "read curr_sensor info: \n"
                        "cat /sys_switch/curr_sensor/curr*/*\n");
}

ssize_t curr_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t vol_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "read curr_sensor info: \n"
                        "cat /sys_switch/vol_sensor/vol*/*\n");
}

ssize_t vol_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t fan_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "fan speed adjust: \n"
                        "echo <ratio> > /sys_switch/fan/fan*/ratio\n"
                        "check fan speed: \n"
                        "cat /sys_switch/fan/fan*/motor1/speed\n");
}

ssize_t fan_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t psu_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "check PSU info: \n"
                        "cat /sys_switch/psu/psu*/*\n"
                        "check PSU temerature: \n"
                        "cat /sys_switch/psu/psu*/temp*/*\n");
}

ssize_t psu_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t led_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "get led status: \n"
                        "cat /sys_switch/sysled/*\n"
                        "set led status: \n"
                        "echo <status> > /sys_switch/sysled/sys_led_status\n");
}

ssize_t led_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t fpga_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "check FPGA version: \n"
                        "cat /sys_switch/fpga/fpga*/board_version\n");
}

ssize_t fpga_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t cpld_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "check cpld version: \n"
                        "cat /sys_switch/cpld/cpld*/firmware_version\n");
}

ssize_t cpld_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t wdt_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "watchdog debug: \n"
                        "echo 120 > /sys_switch/watchdog/timeout\n"
                        "cat /sys_switch/watchdog/timeleft\n");
}

ssize_t wdt_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

ssize_t slot_debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "slot debug: \n"
                        "cat /sys_switch/slot/number\n");
}

ssize_t slot_debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOSYS;
}

static struct kobj_attribute sys_bsp_version_attr = __ATTR(bsp_version, S_IRUGO | S_IWUSR, sys_bsp_version_debug_show, NULL);
static struct kobj_attribute sys_debug_attr = __ATTR(sys_debug, S_IRUGO | S_IWUSR, sys_debug_show, sys_debug_store);
static struct kobj_attribute xcvr_debug_attr = __ATTR(xcvr_debug, S_IRUGO | S_IWUSR, xcvr_debug_show, xcvr_debug_store);
static struct kobj_attribute temp_debug_attr = __ATTR(temp_debug, S_IRUGO | S_IWUSR, temp_debug_show, temp_debug_store);
static struct kobj_attribute curr_debug_attr = __ATTR(curr_debug, S_IRUGO | S_IWUSR, curr_debug_show, curr_debug_store);
static struct kobj_attribute vol_debug_attr = __ATTR(vol_debug, S_IRUGO | S_IWUSR, vol_debug_show, vol_debug_store);
static struct kobj_attribute fan_debug_attr = __ATTR(fan_debug, S_IRUGO | S_IWUSR, fan_debug_show, fan_debug_store);
static struct kobj_attribute psu_debug_attr = __ATTR(psu_debug, S_IRUGO | S_IWUSR, psu_debug_show, psu_debug_store);
static struct kobj_attribute led_debug_attr = __ATTR(led_debug, S_IRUGO | S_IWUSR, led_debug_show, led_debug_store);
static struct kobj_attribute fpga_debug_attr = __ATTR(fpga_debug, S_IRUGO | S_IWUSR, fpga_debug_show, fpga_debug_store);
static struct kobj_attribute cpld_debug_attr = __ATTR(cpld_debug, S_IRUGO | S_IWUSR, cpld_debug_show, cpld_debug_store);
static struct kobj_attribute wdt_debug_attr = __ATTR(wdt_debug, S_IRUGO | S_IWUSR, wdt_debug_show, wdt_debug_store);
static struct kobj_attribute slot_debug_attr = __ATTR(slot_debug, S_IRUGO | S_IWUSR, slot_debug_show, slot_debug_store);

static struct attribute *pddf_debug_attrs[] = {
    &sys_bsp_version_attr.attr,
    &sys_debug_attr.attr,
    &xcvr_debug_attr.attr,
    &temp_debug_attr.attr,
    &curr_debug_attr.attr,
    &vol_debug_attr.attr,
    &fan_debug_attr.attr,
    &psu_debug_attr.attr,
    &led_debug_attr.attr,
    &fpga_debug_attr.attr,
    &cpld_debug_attr.attr,
    &wdt_debug_attr.attr,
    &slot_debug_attr.attr,
    NULL,
};

static struct attribute_group pddf_debug_attr_group = {
    .attrs = pddf_debug_attrs,
};


static struct kobject *debug_kobj;

static int __init pddf_custom_debug_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj) 
        return -ENOMEM;

    debug_kobj = kobject_create_and_add("pddf_custom", device_kobj);
    if (!debug_kobj)
        return -ENOMEM;

    ret = sysfs_create_group(debug_kobj, &pddf_debug_attr_group);
    if (ret) {
        kobject_put(debug_kobj);
        return ret;
    }

    return 0;
}

static void __exit pddf_custom_debug_exit(void)
{

    sysfs_remove_group(debug_kobj, &pddf_debug_attr_group);

    kobject_put(debug_kobj);

    return;
}


module_init(pddf_custom_debug_init);
module_exit(pddf_custom_debug_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDDF debug interface under /sys/kernel/pddf_debug");