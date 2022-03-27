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

struct temp_data {
    int temp;
    int ttarget;
    int tjmax;
    unsigned long last_updated;
    unsigned int cpu;
    u32 cpu_core_id;
    u32 status_reg;
    int attr_size;
    bool is_pkg_data;
    bool valid;
    struct sensor_device_attribute sd_attrs[TOTAL_ATTRS];
    char attr_name[TOTAL_ATTRS][CORETEMP_NAME_LENGTH];
    struct attribute *attrs[TOTAL_ATTRS + 1];
    struct attribute_group attr_group;
    struct mutex update_lock;
};


struct platform_data {
    struct device       *hwmon_dev;
    u16         pkg_id;
    struct cpumask      cpumask;
    struct temp_data    *core_data[MAX_CORE_DATA];
    struct device_attribute name_attr;
};

struct sensor_descript {
    unsigned char *adap_name;
    unsigned char addr;
    unsigned char *alias;
};

inline int get_cpu_hwmon_attr_by_name(struct device *dev, unsigned char *node_name, char *buf)
{
    struct platform_data *pdata;
    struct device_attribute *dev_attr;
    struct temp_data *core_data = NULL;
    int i, j;

    if (dev == NULL)
        return -1;

    pdata = dev_get_drvdata(dev);
   
    for (i=0; i<MAX_CORE_DATA; i++) {
        core_data = pdata->core_data[i];
        if (core_data == NULL)
            continue;

        for (j=0; j<core_data->attr_size; j++) {
            if (core_data->attrs[j] != NULL) {
                if (strcmp(core_data->attrs[j]->name, node_name) == 0) {
                    dev_attr = to_dev_attr(core_data->attrs[j]);
                    return dev_attr->show(dev, dev_attr, buf);
                }
            }
        }
    }

    return -1;
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
