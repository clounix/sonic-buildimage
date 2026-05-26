#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/errno.h>
#include "pddf_client_defs.h"
#include "pddf_led_defs.h"

extern struct pddf_data_attribute pddf_dev_cur_state_attr_fantray1_led;
extern struct pddf_data_attribute pddf_dev_cur_state_attr_fantray2_led;
extern struct pddf_data_attribute pddf_dev_cur_state_attr_fantray3_led;
extern struct pddf_data_attribute pddf_dev_cur_state_attr_fantray4_led;
extern struct pddf_data_attribute pddf_dev_cur_state_attr_fantray5_led;
extern struct pddf_data_attribute pddf_dev_cur_state_attr_fantray6_led;

static ssize_t (*orig_store[6])(struct device *, struct device_attribute *, const char *, size_t);

static ssize_t fantray_led_store_disabled(struct device *dev,
                                          struct device_attribute *attr,
                                          const char *buf, size_t count)
{
    pr_err("pddf_led: Writing to %s is prohibited (read-only)\n", attr->attr.name);
    return -EPERM;
}

static int __init pddf_custom_init(void)
{
    orig_store[0] = pddf_dev_cur_state_attr_fantray1_led.dev_attr.store;
    pddf_dev_cur_state_attr_fantray1_led.dev_attr.store = fantray_led_store_disabled;

    orig_store[1] = pddf_dev_cur_state_attr_fantray2_led.dev_attr.store;
    pddf_dev_cur_state_attr_fantray2_led.dev_attr.store = fantray_led_store_disabled;

    orig_store[2] = pddf_dev_cur_state_attr_fantray3_led.dev_attr.store;
    pddf_dev_cur_state_attr_fantray3_led.dev_attr.store = fantray_led_store_disabled;

    orig_store[3] = pddf_dev_cur_state_attr_fantray4_led.dev_attr.store;
    pddf_dev_cur_state_attr_fantray4_led.dev_attr.store = fantray_led_store_disabled;

    orig_store[4] = pddf_dev_cur_state_attr_fantray5_led.dev_attr.store;
    pddf_dev_cur_state_attr_fantray5_led.dev_attr.store = fantray_led_store_disabled;

    orig_store[5] = pddf_dev_cur_state_attr_fantray6_led.dev_attr.store;
    pddf_dev_cur_state_attr_fantray6_led.dev_attr.store = fantray_led_store_disabled;

    pr_info("pddf_custom_led: All 6 fantray LEDs are now read-only\n");
    return 0;
}

static void __exit pddf_custom_exit(void)
{
    pddf_dev_cur_state_attr_fantray1_led.dev_attr.store = orig_store[0];
    pddf_dev_cur_state_attr_fantray2_led.dev_attr.store = orig_store[1];
    pddf_dev_cur_state_attr_fantray3_led.dev_attr.store = orig_store[2];
    pddf_dev_cur_state_attr_fantray4_led.dev_attr.store = orig_store[3];
    pddf_dev_cur_state_attr_fantray5_led.dev_attr.store = orig_store[4];
    pddf_dev_cur_state_attr_fantray6_led.dev_attr.store = orig_store[5];

    pr_info("pddf_custom_led: Original functionality restored\n");
}

module_init(pddf_custom_init);
module_exit(pddf_custom_exit);

MODULE_AUTHOR("FLKS");
MODULE_DESCRIPTION("Make all 6 fantray LEDs read-only");
MODULE_LICENSE("GPL");
