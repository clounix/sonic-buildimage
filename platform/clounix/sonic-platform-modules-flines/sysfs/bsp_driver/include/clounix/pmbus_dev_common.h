#include "pmbus.h"
#include "clx_driver.h"

#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/string.h>
#include <linux/file.h>

#define PMBUS_NAME_SIZE 24

#define PB_STATUS_BASE 0
#define PB_STATUS_VOUT_BASE (PB_STATUS_BASE + PMBUS_PAGES)
#define PB_STATUS_IOUT_BASE (PB_STATUS_VOUT_BASE + PMBUS_PAGES)
#define PB_STATUS_FAN_BASE (PB_STATUS_IOUT_BASE + PMBUS_PAGES)
#define PB_STATUS_FAN34_BASE (PB_STATUS_FAN_BASE + PMBUS_PAGES)
#define PB_STATUS_TEMP_BASE (PB_STATUS_FAN34_BASE + PMBUS_PAGES)
#define PB_STATUS_INPUT_BASE (PB_STATUS_TEMP_BASE + PMBUS_PAGES)
#define PB_STATUS_VMON_BASE (PB_STATUS_INPUT_BASE + 1)
#define PB_NUM_STATUS_REG (PB_STATUS_VMON_BASE + 1)

struct pmbus_data
{
    struct device *dev;
    struct device *hwmon_dev;

    u32 flags; /* from platform data */

    int exponent[PMBUS_PAGES];
    /* linear mode: exponent for output voltages */

    const struct pmbus_driver_info *info;

    int max_attributes;
    int num_attributes;
    struct attribute_group group;
    const struct attribute_group *groups[2];
    struct dentry *debugfs; /* debugfs device directory */

    struct pmbus_sensor *sensors;

    struct mutex update_lock;
    bool valid;
    unsigned long last_updated; /* in jiffies */

    /*
     * A single status register covers multiple attributes,
     * so we keep them all together.
     */
    u16 status[PB_NUM_STATUS_REG];

    bool has_status_word; /* device uses STATUS_WORD register */
    int (*read_status)(struct i2c_client *client, int page);

    u8 currpage;
};

#define RANGE_LABEL 0
#define ADDR_LABEL 0
#define LOCATION_LABEL 1
#define SENSOR_OFFSET_LABEL 2
#define SCALE_FACTOR_LABEL 3

static inline int get_psu_sensor_index(int curr_index, unsigned char (*range_map)[SENSOR_RANGE_MAX])
{
    int i = 0;
    while (range_map[i][RANGE_LABEL] != 0)
    {
        if (curr_index < range_map[i][RANGE_LABEL])
            break;

        i++;
    }

    return range_map[i][LOCATION_LABEL];
}

static inline int get_sensor_internal_index(int sensor_index, int node_index, short (*sensor_map)[SENSOR_COL_MAX])
{
    return node_index + sensor_map[sensor_index][SENSOR_OFFSET_LABEL];
}

static ssize_t read_sysfs_file(const char *filepath, char *buf, size_t size)
{
    struct path path;
    struct file *filp;
    loff_t pos = 0;
    ssize_t ret;

    ret = kern_path(filepath, LOOKUP_FOLLOW, &path);
    if (ret)
        return ret;

    filp = dentry_open(&path, O_RDONLY, current_cred());
    path_put(&path);
    if (IS_ERR(filp))
        return PTR_ERR(filp);

    ret = kernel_read(filp, buf, size - 1, &pos);
    if (ret > 0)
    {
        buf[ret] = '\0';
    }
    else if (ret == 0)
    {
        buf[0] = '\0';
    }

    fput(filp);
    return ret;
}

static int write_sysfs_file(const char *filepath, const char *buf, size_t count)
{
    struct path path;
    struct file *filp;
    ssize_t ret;

    ret = kern_path(filepath, LOOKUP_FOLLOW, &path);
    if (ret)
        return ret;

    filp = dentry_open(&path, O_WRONLY, current_cred());
    path_put(&path);
    if (IS_ERR(filp))
        return PTR_ERR(filp);

    ret = kernel_write(filp, buf, count, &filp->f_pos);
    fput(filp);
    return ret;
}

static int find_pmbus_hwmon_index(struct i2c_client *client, const char *node_name)
{
    if (!node_name)
        return -1;

    char base_path[64];
    char test_path[64];
    int i;

    snprintf(base_path, sizeof(base_path),
             "/sys/bus/i2c/devices/%s/hwmon", dev_name(&client->dev));

    for (i = 1; i <= 16; i++)
    {
        snprintf(test_path, sizeof(test_path),
                 "%s/hwmon%d/%s", base_path, i, node_name);

        struct path path;
        if (kern_path(test_path, LOOKUP_FOLLOW, &path) == 0)
        {
            path_put(&path);
            return i;
        }
    }
    return -1;
}

static inline int get_attr_val_by_name(struct i2c_client *client, const char *node_name, char *buf)
{
    int hwmon_idx;
    char filepath[64];

    if (!node_name || !buf)
        return -EINVAL;

    hwmon_idx = find_pmbus_hwmon_index(client, node_name);
    if (hwmon_idx < 0)
        return -ENODEV;

    snprintf(filepath, sizeof(filepath),
             "/sys/bus/i2c/devices/%s/hwmon/hwmon%d/%s",
             dev_name(&client->dev), hwmon_idx, node_name);

    return read_sysfs_file(filepath, buf, PAGE_SIZE);
}

static inline int set_attr_val_by_name(struct i2c_client *client, const char *node_name, const char *buf, size_t count)
{
    int hwmon_idx;
    char filepath[64];

    if (!node_name || !buf || count == 0)
        return -EINVAL;

    hwmon_idx = find_pmbus_hwmon_index(client, node_name);
    if (hwmon_idx < 0)
        return -ENODEV;

    snprintf(filepath, sizeof(filepath),
             "/sys/bus/i2c/devices/%s/hwmon/hwmon%d/%s",
             dev_name(&client->dev), hwmon_idx, node_name);

    return write_sysfs_file(filepath, buf, count);
}