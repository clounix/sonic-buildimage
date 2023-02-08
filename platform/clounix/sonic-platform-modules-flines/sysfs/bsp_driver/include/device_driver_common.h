#ifndef _DEVICE_DRIVER_COMMON_H_
#define _DEVICE_DRIVER_COMMON_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/delay.h>

enum LOG_LEVEL{
    ERR  = 0x1,
    WARNING  = 0x2,
    INFO = 0x4,
    DBG  = 0x8,
    ALL  = 0xf
};

#endif /* _DEVICE_DRIVER_COMMON_H_ */
