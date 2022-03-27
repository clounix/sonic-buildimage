#include "pmbus.h"

#define PMBUS_NAME_SIZE 24

#define PB_STATUS_BASE      0
#define PB_STATUS_VOUT_BASE (PB_STATUS_BASE + PMBUS_PAGES)
#define PB_STATUS_IOUT_BASE (PB_STATUS_VOUT_BASE + PMBUS_PAGES)
#define PB_STATUS_FAN_BASE  (PB_STATUS_IOUT_BASE + PMBUS_PAGES)
#define PB_STATUS_FAN34_BASE    (PB_STATUS_FAN_BASE + PMBUS_PAGES)
#define PB_STATUS_TEMP_BASE (PB_STATUS_FAN34_BASE + PMBUS_PAGES)
#define PB_STATUS_INPUT_BASE    (PB_STATUS_TEMP_BASE + PMBUS_PAGES)
#define PB_STATUS_VMON_BASE (PB_STATUS_INPUT_BASE + 1)

#define PB_NUM_STATUS_REG   (PB_STATUS_VMON_BASE + 1)

struct pmbus_sensor {
    struct pmbus_sensor *next;
    char name[PMBUS_NAME_SIZE]; /* sysfs sensor name */
    struct device_attribute attribute;
    u8 page;        /* page number */
    u16 reg;        /* register */
    enum pmbus_sensor_classes class;    /* sensor class */
    bool update;        /* runtime sensor update needed */
    bool convert;       /* Whether or not to apply linear/vid/direct */
    int data;       /* Sensor data.
                   Negative if there was a read error */
};

struct pmbus_data {
    struct device *dev;
    struct device *hwmon_dev;

    u32 flags;      /* from platform data */

    int exponent[PMBUS_PAGES];
                /* linear mode: exponent for output voltages */

    const struct pmbus_driver_info *info;

    int max_attributes;
    int num_attributes;
    struct attribute_group group;
    const struct attribute_group *groups[2];
    struct dentry *debugfs;     /* debugfs device directory */

    struct pmbus_sensor *sensors;

    struct mutex update_lock;
    bool valid;
    unsigned long last_updated; /* in jiffies */

    /*
     * A single status register covers multiple attributes,
     * so we keep them all together.
     */
    u16 status[PB_NUM_STATUS_REG];

    bool has_status_word;       /* device uses STATUS_WORD register */
    int (*read_status)(struct i2c_client *client, int page);

    u8 currpage;
};


#define RANGE_LABEL 0
#define ADDR_LABEL 0
#define LOCATION_LABEL 1
#define SENSOR_OFFSET_LABEL 2

static inline int get_psu_sensor_index(int curr_index, unsigned char (*range_map)[2])
{
    int i = 0;
    while (range_map[i][RANGE_LABEL] != 0) {
        if (curr_index < range_map[i][RANGE_LABEL])
            break;

        i++;
    }

    return range_map[i][LOCATION_LABEL];
}

static inline int get_sensor_internal_index(int sensor_index, int node_index, short (*sensor_map)[3])
{
    return node_index - sensor_map[sensor_index][SENSOR_OFFSET_LABEL];
}

inline int get_attr_val_by_name(struct i2c_client *client, char *node_name, char *buf)
{
    struct pmbus_data *p = i2c_get_clientdata(client);
    struct device_attribute *dev_attr;
    struct attribute *a;
    struct device dev = {0};
    int ret = -1;
    int i;

    for (i=0; p->groups[0]->attrs[i] != NULL; i++) {
        a = p->groups[0]->attrs[i];
        if (memcmp(a->name, node_name, strlen(node_name)) ==0) {
            dev_attr = container_of(a, struct device_attribute, attr);
            dev.parent = &client->dev;
            ret = dev_attr->show(&dev, dev_attr, buf);
            break;
        }
    }

    return ret;
}

inline int set_attr_val_by_name(struct i2c_client *client, char *node_name, const char *buf, size_t count)
{
    struct pmbus_data *p = i2c_get_clientdata(client);
    struct device_attribute *dev_attr;
    struct attribute *a;
    struct device dev = {0};
    int ret = -1;
    int i;

    for (i=0; p->groups[0]->attrs[i] != NULL; i++) {
        a = p->groups[0]->attrs[i];
        if (memcmp(a->name, node_name, strlen(node_name)) ==0) {
            dev_attr = container_of(a, struct device_attribute, attr);
            dev.parent = &client->dev;
            ret = dev_attr->store(&dev, dev_attr, buf, count);
            break;
        }
    }

    return ret;
}
