
#ifndef __PEGATRON_PUB_H__
#define __PEGATRON_PUB_H__

#include <linux/printk.h>

#ifndef KBUILD_MODNAME
#define KBUILD_MODNAME __FILE__
#endif

#define clounix_pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
enum log_level
{
    LOG_ERR = 0x01,
    LOG_WARNING = 0x02,
    LOG_INFO = 0x04,
    LOG_DEBUG = 0x08,
};

#define pega_print(level, fmt, ...)                        \
    do                                                     \
    {                                                      \
        if (LOG_##level & loglevel)                        \
            printk(KERN_DEBUG clounix_pr_fmt(fmt), ##__VA_ARGS__); \
    } while (0)

#define MAX_DEBUG_INFO_LEN  1024

#endif
