#ifndef _TPS389006_H_
#define _TPS389006_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/delay.h>

enum LOG_LEVEL
{
    ERR = 0x1,
    WARNING = 0x2,
    INFO = 0x4,
    DBG = 0x8,
    ALL = 0xf
};

#define TPS_389006_VMON_VENDOR_ID_REG 0x00
#define TPS_389006_VMON_STAT_REG 0x30
#define TPS_389006_VMON_BANK_SEL_REG 0xf0
#define TPS_389006_VMON_CTL_REG 0x10
#define TPS_389006_VMON_MISC_REG 0x11
#define TPS_389006_VMON_VIN_CH_EN_REG 0x1e
#define TPS_389006_VMON_VRANGE_MULT_REG 0x1f
#define TPS_389006_VMON_OFF_STAT_REG 0x32
#define TPS_389006_VMON_VIN_LVL_BASE_REG 0x40
#define TPS_389006_VMON_ACT_ADDR_REG 0x0B00

#define SENSOR_DEVICE_ATTR_RO(_name, _func, _index) \
    SENSOR_DEVICE_ATTR(_name, 0444, _func##_show, NULL, _index)

#define TPS389006_LOG_INFO(fmt, args...)                                                        \
    do                                                                                          \
    {                                                                                           \
        if (g_tps389006_loglevel & INFO)                                                        \
        {                                                                                       \
            printk(KERN_INFO "[tps389006][func:%s line:%d]: " fmt, __func__, __LINE__, ##args); \
        }                                                                                       \
    } while (0)

#define TPS389006_LOG_ERR(fmt, args...)                                                        \
    do                                                                                         \
    {                                                                                          \
        if (g_tps389006_loglevel & ERR)                                                        \
        {                                                                                      \
            printk(KERN_ERR "[tps389006][func:%s line:%d]: " fmt, __func__, __LINE__, ##args); \
        }                                                                                      \
    } while (0)

#define TPS389006_LOG_DBG(fmt, args...)                                                          \
    do                                                                                           \
    {                                                                                            \
        if (g_tps389006_loglevel & DBG)                                                          \
        {                                                                                        \
            printk(KERN_DEBUG "[tps389006][func:%s line:%d]: " fmt, __func__, __LINE__, ##args); \
        }                                                                                        \
    } while (0)


#endif /* _TPS389006_H_ */
