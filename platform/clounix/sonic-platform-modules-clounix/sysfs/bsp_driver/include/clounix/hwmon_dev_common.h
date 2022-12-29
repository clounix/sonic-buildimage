#ifndef _HWMON_DEV_COMMON_H_
#define _HWMON_DEV_COMMON_H_

#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define BASE_SYSFS_ATTR_NO  2   /* Sysfs Base attr no for coretemp */
#define NUM_REAL_CORES      128 /* Number of Real cores per cpu */
#define MAX_CORE_DATA       (NUM_REAL_CORES + BASE_SYSFS_ATTR_NO)
#define TOTAL_ATTRS     (MAX_CORE_ATTRS + 1)
#define CORETEMP_NAME_LENGTH    19
#define MAX_CORE_ATTRS      4

#define MAX_SYSFS_ATTR_NAME_LENGTH  32
#define to_hwmon_device(d) container_of(d, struct hwmon_device, dev)
#define to_dev_attr(a) container_of(a, struct device_attribute, attr)

struct hwmon_device {
     const char *name;
     struct device dev;
     const struct hwmon_chip_info *chip;

     struct attribute_group group;
     const struct attribute_group **groups;
};

struct s3ip_cpu_temp_data {
    struct device *dev;
    struct attribute **attrs;
    int auto_inc;
};

struct platform_data {
    struct device       *hwmon_dev;
    u16         pkg_id;
    struct cpumask      cpumask;
    struct temp_data    *core_data[MAX_CORE_DATA];
    struct device_attribute name_attr;
};

inline int get_cpu_hwmon_attr_by_name(struct s3ip_cpu_temp_data *data, unsigned char *node_name, char *buf)
{
    struct device_attribute *dev_attr = NULL;
    int i;

    i = 0;
    while (data->attrs[i] != NULL) {
        if (strcmp(data->attrs[i]->name, node_name) == 0) {
            dev_attr = to_dev_attr(data->attrs[i]);
            return dev_attr->show(data->dev, dev_attr, buf);
        }

        i++;
    }

    return -ENODATA;
}

inline int get_sensor_index(struct i2c_client *client, struct sensor_descript *sensor_map_index)
{
   int i;

   for (i=0; sensor_map_index[i].adap_name != NULL; i++) {
        if (strcmp(sensor_map_index[i].adap_name, client->adapter->name) != 0)
            continue;

        if (sensor_map_index[i].addr != client->addr)
            continue;

        return i;
   }

   return -ENODATA;
}

inline int get_hwmon_attr_by_name(struct device *dev, unsigned char *node_name, char *buf)
{
    struct hwmon_device *hwdev;
    struct device_attribute *dev_attr;
    struct attribute *a;
    int i;
    
    if (dev == NULL)
        return -1;

    hwdev = to_hwmon_device(dev);
    for (i=0; hwdev->group.attrs[i] != NULL; i++) {
        a = hwdev->group.attrs[i];
        if (strcmp(a->name, node_name) == 0) {
            dev_attr = to_dev_attr(a);
            return dev_attr->show(dev, dev_attr, buf);
        }
    }

    return -1;
}

inline int set_hwmon_attr_by_name(struct device *dev, unsigned char *node_name, const char *buf, size_t count)
{
    struct hwmon_device *hwdev;
    struct device_attribute *dev_attr;
    struct attribute *a;
    int i;

    if (dev == NULL)
        return -1;

    hwdev = to_hwmon_device(dev);
    for (i=0; hwdev->group.attrs[i] != NULL; i++) {
        a = hwdev->group.attrs[i];
        if (strcmp(a->name, node_name) == 0) {
            dev_attr = to_dev_attr(a);
            return dev_attr->store(dev, dev_attr, buf, count);
        }
    }

    return -1;
}
#endif