/*
 * io_signal_ctrl.c
 *
 * This module realize s3ip dev attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/io.h>
#include <linux/rwlock.h>

#include "device_driver_common.h"
#include "clounix/io_signal_ctrl.h"


DEFINE_RWLOCK(priv_lock);
LIST_HEAD(node_head);

static struct io_sig_desc *find_io_sig_desc(int type, int num)
{
    struct io_sig_desc *pos = NULL;
    struct io_sig_desc *tmp = NULL;
    struct io_sig_desc *desc = NULL;

    list_for_each_entry_safe(pos, tmp, &node_head, node) {
        if (pos->type == type && pos->num == num) {
            desc = pos;
            break;
        }
    }

    return desc;
}

int write_io_sig_desc(int type, int num, int val)
{
    struct io_sig_desc *desc = NULL;
    int ret = -EIO;

    read_lock(&priv_lock);
    desc = find_io_sig_desc(type, num);
    if (desc != NULL)
        if (desc->write != NULL)
            ret = desc->write(type, num, val);
    read_unlock(&priv_lock);

    return ret;
}
EXPORT_SYMBOL(write_io_sig_desc);

int read_io_sig_desc(int type, int num)
{
    struct io_sig_desc *desc = NULL;
    int ret = -EIO;

    read_lock(&priv_lock);
    desc = find_io_sig_desc(type, num);
    if (desc != NULL)
        if (desc->read != NULL)
            ret = desc->read(type, num);
    read_unlock(&priv_lock);

    return ret;
}
EXPORT_SYMBOL(read_io_sig_desc);

int add_io_sig_desc(int type, int num, int (*read)(int, int), int (*write)(int, int, int))
{
    struct io_sig_desc *mb;
    struct io_sig_desc *pos = NULL;
    struct io_sig_desc *tmp = NULL;

    mb = kzalloc(sizeof(struct io_sig_desc), GFP_ATOMIC);
    if (mb == NULL)
        return -ENOMEM;
    
    write_lock(&priv_lock);
    list_for_each_entry_safe(pos, tmp, &node_head, node) {
        if (pos->type == type && pos->num == num) {
            kfree(mb);
            write_unlock(&priv_lock);
            return -EPERM;
        }
    }
    mb->type = type;
    mb->num = num;
    mb->read = read;
    mb->write = write;
    list_add_tail(&(mb->node), &node_head);
    write_unlock(&priv_lock);

    return 0;
}
EXPORT_SYMBOL(add_io_sig_desc);

void del_io_sig_desc(int type, int num)
{
    struct io_sig_desc *pos = NULL;
    struct io_sig_desc *tmp = NULL;

    write_lock(&priv_lock);
    list_for_each_entry_safe(pos, tmp, &node_head, node) {
        if (pos->type == type && pos->num == num) {
            list_del_init(&(pos->node));
            kfree(pos);
            break;
        }
    }
    write_unlock(&priv_lock);

    return;
}
EXPORT_SYMBOL(del_io_sig_desc);

static  int __init io_signal_ctrl_init(void)
{
    printk(KERN_ALERT "this is one-wire device layer!\n");
    return 0;
}

static  void __exit io_signal_ctrl_exit(void)
{
    return;
}

module_init(io_signal_ctrl_init);
module_exit(io_signal_ctrl_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("baohx@clounix.com");
MODULE_DESCRIPTION("io_signal_ctrl module");
