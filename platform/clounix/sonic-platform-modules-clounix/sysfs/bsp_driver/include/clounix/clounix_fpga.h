
#ifndef __CLOUNIX_FPGA_H__
#define __CLOUNIX_FPGA_H__
#include <linux/device.h>
#include "clounix_pub.h"

#define FPGA_PCI_VENDOR_ID 0x10EE
#define FPGA_PCI_DEVICE_ID 0x7021

#define MAX_PORT_NUM                (56)
#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)


struct fpga_device_attribute{
        struct device_attribute dev_attr;
        int index;
};
#define to_fpga_dev_attr(_dev_attr) \
        container_of(_dev_attr, struct fpga_device_attribute, dev_attr)

#define FPGA_ATTR(_name, _mode, _show, _store, _index)        \
        { .dev_attr = __ATTR(_name, _mode, _show, _store),      \
          .index = _index }

#define FPGA_DEVICE_ATTR(_name, _mode, _show, _store, _index) \
struct fpga_device_attribute fpga_dev_attr_##_name          \
        = FPGA_ATTR(_name, _mode, _show, _store, _index)




#endif
