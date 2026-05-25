#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include <linux/string.h>
#include "pddf_client_defs.h"
#include "pddf_sysstatus_defs.h"

static int *log_level = &sysstat_log_level;

extern struct pddf_data_attribute attr_cpld4_version;
extern SYSSTATUS_DATA sysstatus_data;
extern int board_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int (*ptr_fpgapci_read)(uint32_t);
extern void* get_device_table(char *name);

static ssize_t (*orig_show)(struct device *dev, struct device_attribute *da, char *buf);

ssize_t show_sysstatus_data_custom(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *sattr = to_sensor_dev_attr(da);
    struct SYSSTATUS_ADDR_ATTR *attr_cfg = NULL;
    struct i2c_client *client_ptr;
    int i, status = 0;
    const char *attr_name = sattr->dev_attr.attr.name;

    for (i = 0; i < MAX_ATTRS; i++) {
        if (strcmp(sysstatus_data.sysstatus_addr_attrs[i].aname, attr_name) == 0) {
            attr_cfg = &sysstatus_data.sysstatus_addr_attrs[i];
            break;
        }
    }

    if (!attr_cfg) {
        pddf_err(SYSSTATUS, "%s not found in sysstatus config\n", attr_name);
        return sprintf(buf, "0x0\n");
    }

    pddf_dbg(SYSSTATUS, "Reading %s: devtype=%s, devaddr=0x%x, offset=0x%x, mask=0x%x\n",
             attr_name, attr_cfg->devtype, attr_cfg->devaddr, 
             attr_cfg->offset, attr_cfg->mask);

    if (strncmp(attr_cfg->devtype, "cpld", 4) == 0) {
        status = board_i2c_cpld_read(attr_cfg->devaddr, attr_cfg->offset);
    } 
    else if (strncmp(attr_cfg->devtype, "fpgapci", 7) == 0) {
        if (!ptr_fpgapci_read) {
            pddf_err(SYSSTATUS, "fpgapci_read not available\n");
            return sprintf(buf, "0x0\n");
        }
        status = ptr_fpgapci_read(attr_cfg->devaddr);
        status = (status & attr_cfg->mask) >> attr_cfg->offset;
    } 
    else if (strncmp(attr_cfg->devtype, "FAN-CTRL", 8) == 0) {
        client_ptr = (struct i2c_client *)get_device_table("FAN-CTRL");
        if (!client_ptr) {
            pddf_err(SYSSTATUS, "FAN-CTRL device not found\n");
            return sprintf(buf, "0x0\n");
        }
        status = i2c_smbus_read_byte_data(client_ptr, attr_cfg->offset);
        status = (status & attr_cfg->mask) >> attr_cfg->offset;
    } 
    else {
        pddf_err(SYSSTATUS, "Unsupported devtype: %s\n", attr_cfg->devtype);
        return sprintf(buf, "0x0\n");
    }

    if (status < 0) {
        pddf_err(SYSSTATUS, "Read %s failed: %d\n", attr_name, status);
        status = 0;
    }

    return sprintf(buf, "0x%x\n", status);
}

static int __init pddf_custom_sysstatus_init(void)
{
    pddf_info(SYSSTATUS, "pddf_custom_sysstatus module loading\n");

    if (!attr_cpld4_version.dev_attr.show) {
        pddf_err(SYSSTATUS, "Original show function is NULL\n");
        return -ENODEV;
    }

    orig_show = attr_cpld4_version.dev_attr.show;
    attr_cpld4_version.dev_attr.show = show_sysstatus_data_custom;

    pddf_info(SYSSTATUS, "cpld4_version show function replaced\n");
    return 0;
}

static void __exit pddf_custom_sysstatus_exit(void)
{
    pddf_info(SYSSTATUS, "pddf_custom_sysstatus module unloading\n");

    if (orig_show) {
        attr_cpld4_version.dev_attr.show = orig_show;
        pddf_info(SYSSTATUS, "Original show function restored\n");
    }
}

module_init(pddf_custom_sysstatus_init);
module_exit(pddf_custom_sysstatus_exit);

MODULE_AUTHOR("FLKS");
MODULE_DESCRIPTION("PDDF custom sysstatus handler for ds410h-48y8c");
MODULE_LICENSE("GPL");
