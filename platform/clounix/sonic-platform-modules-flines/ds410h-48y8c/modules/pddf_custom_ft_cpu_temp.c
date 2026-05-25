#include <linux/module.h>
#include <linux/device.h>
#include <linux/hwmon.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/ctype.h>

#define DRV_NAME        "scpi_temp_thresh_attach"
#define MAX_SENSORS     2
#define DEFAULT_MAX     62000
#define DEFAULT_CRIT    69000

static struct device *scpi_hwmon_dev = NULL;
static long temp_crit_val[MAX_SENSORS] = {[0 ... MAX_SENSORS-1] = DEFAULT_CRIT};
static long temp_max_val[MAX_SENSORS] = {[0 ... MAX_SENSORS-1] = DEFAULT_MAX};

static ssize_t temp_crit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int index = 0;
    if (sscanf(attr->attr.name, "temp%d_crit", &index) == 1)
        index--;
    if (index < 0 || index >= MAX_SENSORS)
        return -EINVAL;
    return sprintf(buf, "%ld\n", temp_crit_val[index]);
}

static ssize_t temp_max_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int index = 0;
    if (sscanf(attr->attr.name, "temp%d_max", &index) == 1)
        index--;
    if (index < 0 || index >= MAX_SENSORS)
        return -EINVAL;
    return sprintf(buf, "%ld\n", temp_max_val[index]);
}

static ssize_t temp_crit_store(struct device *dev, struct device_attribute *attr,
                                const char *buf, size_t count)
{
    int index = 0;
    long val;
    if (kstrtol(buf, 10, &val) != 0)
        return -EINVAL;
    if (sscanf(attr->attr.name, "temp%d_crit", &index) == 1)
        index--;
    if (index < 0 || index >= MAX_SENSORS)
        return -EINVAL;
    temp_crit_val[index] = val;
    return count;
}

static ssize_t temp_max_store(struct device *dev, struct device_attribute *attr,
                               const char *buf, size_t count)
{
    int index = 0;
    long val;
    if (kstrtol(buf, 10, &val) != 0)
        return -EINVAL;
    if (sscanf(attr->attr.name, "temp%d_max", &index) == 1)
        index--;
    if (index < 0 || index >= MAX_SENSORS)
        return -EINVAL;
    temp_max_val[index] = val;
    return count;
}

#define DECLARE_TEMP_FUNCS(_num) \
static ssize_t temp##_num##_crit_show(struct device *dev, struct device_attribute *attr, char *buf) { return temp_crit_show(dev, attr, buf); } \
static ssize_t temp##_num##_crit_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { return temp_crit_store(dev, attr, buf, count); } \
static ssize_t temp##_num##_max_show(struct device *dev, struct device_attribute *attr, char *buf) { return temp_max_show(dev, attr, buf); } \
static ssize_t temp##_num##_max_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { return temp_max_store(dev, attr, buf, count); }

#define DECLARE_TEMP_ATTR(_num) \
    static DEVICE_ATTR_RW(temp##_num##_crit); \
    static DEVICE_ATTR_RW(temp##_num##_max)

DECLARE_TEMP_FUNCS(1);
DECLARE_TEMP_FUNCS(2);

DECLARE_TEMP_ATTR(1);
DECLARE_TEMP_ATTR(2);

static struct attribute *scpi_thresh_attrs[] = {
    &dev_attr_temp1_crit.attr, &dev_attr_temp1_max.attr,
    &dev_attr_temp2_crit.attr, &dev_attr_temp2_max.attr,
    NULL
};

static const struct attribute_group scpi_thresh_group = {
    .attrs = scpi_thresh_attrs,
};

static int is_hwmon_device(struct device *dev, void *data)
{
    if (strncmp(dev_name(dev), "hwmon", 4) == 0) {
        dev_info(dev, "Found candidate hwmon device: %s (parent: %s)\n",
                 dev_name(dev), dev->parent ? dev_name(dev->parent) : "none");
        return 1;
    }
    return 0;
}

static int find_scpi_hwmon_device(void)
{
    struct acpi_device *adev;
    struct device *phy_dev;
    struct device *child_dev;

    adev = acpi_dev_get_first_match_dev("PHYT000D", NULL, -1);
    if (!adev) {
        pr_err("Could not find ACPI device with HID: PHYT000D\n");
        return -ENODEV;
    }

    phy_dev = acpi_get_first_physical_node(adev);
    if (!phy_dev) {
        pr_err("ACPI device PHYT000D has no physical device node\n");
        acpi_dev_put(adev);
        return -ENODEV;
    }

    dev_info(phy_dev, "Found Phytium SCPI physical device: %s\n", dev_name(phy_dev));

    child_dev = device_find_child(phy_dev, NULL, is_hwmon_device);
    if (child_dev) {
        scpi_hwmon_dev = get_device(child_dev);
        put_device(child_dev);
        acpi_dev_put(adev);
        
        dev_info(scpi_hwmon_dev, "Successfully attached to target hwmon device\n");
        return 0;
    }

    pr_err("No hwmon child device found under PHYT000D physical device\n");
    acpi_dev_put(adev);
    return -ENODEV;
}

static int __init scpi_temp_thresh_init(void)
{
    int ret;

    pr_info("Loading Phytium SCPI threshold driver (ACPI-only, safe mode)\n");
    
    ret = find_scpi_hwmon_device();
    if (ret)
        return ret;

    ret = sysfs_create_group(&scpi_hwmon_dev->kobj, &scpi_thresh_group);
    if (ret) {
        pr_err("Failed to create threshold sysfs nodes: %d\n", ret);
        put_device(scpi_hwmon_dev);
        return ret;
    }

    pr_info("ACPI SCPI temperature threshold nodes attached (2 sensors, safe)\n");
    return 0;
}

static void __exit scpi_temp_thresh_exit(void)
{
    if (scpi_hwmon_dev) {
        sysfs_remove_group(&scpi_hwmon_dev->kobj, &scpi_thresh_group);
        put_device(scpi_hwmon_dev);
        pr_info("ACPI SCPI temperature threshold nodes detached cleanly\n");
    }
}

module_init(scpi_temp_thresh_init);
module_exit(scpi_temp_thresh_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phytium SCPI Threshold Attach (ACPI Safe)");
MODULE_DESCRIPTION("Attach tempX_max/tempX_crit to ACPI SCPI hwmon device (HID: PHYT000D, Safe)");
MODULE_VERSION("1.0");
