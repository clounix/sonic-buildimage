#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/slab.h>

#include <linux/wait.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "knet_dev.h"
#include "knet_pci.h"
#include "knet_intr.h"
#include "knet_buffer.h"
#include "knet_common.h"
#include "knet_dev_ver.h"
#include "knet_fault_event.h"

clx_device_t *clx_misc_dev;
uint32_t verbosity = (DBG_CRIT | DBG_ERR | DBG_WARN);
uint32_t intr_mode = INTR_MODE_INTX;

static long
clx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    clx_ioctl_cmd_t *ptr_cmd = (clx_ioctl_cmd_t *)&cmd;
    uint32_t unit = ptr_cmd->field.unit;
    clx_ioctl_type_t type = ptr_cmd->field.type;
    long rc = 0;

    switch (type) {
        case CLX_IOCTL_INIT_DEV:
            rc = clx_ioctl_get_pci_dev_info(unit, arg);
            break;
        case CLX_IOCTL_INIT_RSRV_DMA_MEM:
            break;
        case CLX_IOCTL_DEINIT_RSRV_DMA_MEM:
            break;
        case CLX_IOCTL_ALLOC_SYS_DMA_MEM:
            rc = clx_ioctl_dma_alloc(unit, arg);
            break;
        case CLX_IOCTL_FREE_SYS_DMA_MEM:
            rc = clx_ioctl_dma_free(unit, arg);
            break;
        case CLX_IOCTL_CONNECT_ISR:
            rc = clx_intx_connect_isr(unit, arg);
            break;
        case CLX_IOCTL_DISCONNECT_ISR:
            rc = clx_disconnect_isr(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_CREATE_INTF:
            rc = clx_netif_net_dev_create(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_DESTROY_INTF:
            rc = clx_netif_net_dev_destroy(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_GET_INTF:
            rc = clx_netif_get_netdev(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_SET_INTF:
            rc = clx_netif_net_dev_set(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_CREATE_PROFILE:
            rc = clx_netif_rx_profile_create(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_DESTROY_PROFILE:
            rc = clx_netif_rx_profile_destroy(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_GET_PROFILE:
            rc = clx_netif_rx_profile_get(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_GET_INTF_CNT:
            rc = clx_netif_get_netdev_cnt(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_CLEAR_INTF_CNT:
            rc = clx_netif_clear_netdev_cnt(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_WAIT_RX_FREE:
            rc = clx_netif_receive_to_sdk(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_RX_START:
            rc = clx_ioctl_rx_start(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_RX_STOP:
            rc = clx_ioctl_rx_stop(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_DEV_TX:
            rc = clx_netif_send_from_sdk(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_GET_TX_CNT:
            rc = clx_ioctl_get_tx_cnt(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_GET_RX_CNT:
            rc = clx_ioctl_get_rx_cnt(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_CLEAR_TX_CNT:
            rc = clx_ioctl_clear_tx_cnt(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_CLEAR_RX_CNT:
            rc = clx_ioctl_clear_rx_cnt(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_SET_PORT_ATTR:
            rc = clx_netif_set_intf_attr(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_GET_PORT_ATTR:
            rc = clx_netif_get_intf_attr(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_NL_CREATE_NETLINK:
            rc = clx_netif_create_netlink(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_NL_DESTROY_NETLINK:
            rc = clx_netif_destroy_netlink(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_NL_GET_NETLINK:
            rc = clx_netif_get_netlink(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_NL_SET_PKT_MOD:
            rc = clx_netif_set_pkt_mod(unit, arg);
            break;
        case CLX_IOCTL_TYPE_NETIF_NL_GET_PKT_MOD:
            rc = clx_netif_get_pkt_mod(unit, arg);
            break;
        case CLX_IOCTL_TYPE_SET_PORT_MAP:
            rc = clx_netif_set_port_map(unit, arg);
            break;
        case CLX_IOCTL_TYPE_SET_PORT_ATTR:
            rc = clx_netif_set_port_attr(unit, arg);
            break;
        case CLX_IOCTL_TYPE_GET_PORT_ATTR:
            rc = clx_netif_get_port_attr(unit, arg);
            break;
        case CLX_IOCTL_TYPE_SET_IFA_CFG:
            rc = clx_netif_set_ifa_cfg(unit, arg);
            break;
        case CLX_IOCTL_TYPE_GET_IFA_CFG:
            rc = clx_netif_get_ifa_cfg(unit, arg);
            break;
        default:
            break;
    }

    return rc;
}

static ssize_t
clx_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    clx_intr_info_t info;
    int ret;
    unsigned long flags = 0;

    if (count < sizeof(clx_intr_info_t))
        return -EINVAL;

    ret = wait_event_interruptible(clx_misc_dev->isr_wait_queue,
                                   !kfifo_is_empty(&clx_misc_dev->intr_fifo));
    if (ret)
        return ret;

    spin_lock_irqsave(&clx_misc_dev->fifo_lock, flags);
    if (!kfifo_get(&clx_misc_dev->intr_fifo, &info)) {
        spin_unlock_irqrestore(&clx_misc_dev->fifo_lock, flags);
        return -EFAULT;
    }
    spin_unlock_irqrestore(&clx_misc_dev->fifo_lock, flags);

    dbg_print(DBG_INTR, "out info:0x%lx,irq:%d\n", (unsigned long)&info, info.irq);
    if (copy_to_user(buf, &info, sizeof(info)))
        return -EFAULT;

    return sizeof(info);
}

static int
clx_open(struct inode *ptr_inode, struct file *ptr_file)
{
    return (0);
}

static int
clx_release(struct inode *ptr_inode, struct file *ptr_file)
{
    uint32_t unit = 0;
    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        dma_disable_channel(unit);
    }

    dbg_print(DBG_DEBUG, "clx_release\n");
    clx_netlink_deinit();
    clx_netif_deinit();
    cleanup_usr_dma_buffer();

    return (0);
}

static int
clx_mmap(struct file *filp, struct vm_area_struct *vma)
{
    size_t size = vma->vm_end - vma->vm_start;
    int linux_rc = 0;
    uint32_t unit = 0;
    phys_addr_t phy_addr = vma->vm_pgoff << PAGE_SHIFT;
    struct clx_pci_dev_s *clx_pci_dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    struct clx_ioctl_dma_buffer *info;
    // mmap dma buffer
    list_for_each_entry(info, &dma_buffer_list, list)
    {
        if (info->bus_addr == phy_addr) {
            vma->vm_pgoff = 0;
            if (dma_mmap_coherent(info->alloc_dev, vma, info->virt_addr, info->bus_addr,
                                  info->size)) {
                dbg_print(DBG_ERR, "dma_mmap_coherent failed\n");
                return -EAGAIN;
            }
            return 0;
        }
    }
#endif

    // mmap pci bar
    for (unit = 0; unit < CLX_MAX_CHIP_NUM; unit++) {
        clx_pci_dev = clx_misc_dev->clx_pci_dev[unit];
        if (clx_pci_dev == NULL) {
            break;
        }
        if (clx_pci_dev->bar_phys == phy_addr) {
            vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
            break;
        }
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
    vma->vm_flags |= VM_IO;
#else
    vm_flags_set(vma, VM_IO);
#endif
    if (io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
        linux_rc = -EAGAIN;
    }

    return (linux_rc);
}

static ssize_t
rx_enable_test_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", clx_misc_dev->test_perf.rx_enable_test);
}

static ssize_t
rx_enable_test_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%u", &clx_misc_dev->test_perf.rx_enable_test);
    return count;
}

static ssize_t
tx_enable_test_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", clx_misc_dev->test_perf.tx_enable_test);
}

static ssize_t
tx_enable_test_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%u", &clx_misc_dev->test_perf.tx_enable_test);
    return count;
}

static ssize_t
target_len_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", clx_misc_dev->test_perf.target_len);
}

static ssize_t
target_len_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%u", &clx_misc_dev->test_perf.target_len);
    return count;
}

static ssize_t
target_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", clx_misc_dev->test_perf.target_count);
}

static ssize_t
target_count_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%u", &clx_misc_dev->test_perf.target_count);
    return count;
}

static struct kobj_attribute rx_enable_test_attr =
    __ATTR(rx_enable_test, 0660, rx_enable_test_show, rx_enable_test_store);
static struct kobj_attribute tx_enable_test_attr =
    __ATTR(tx_enable_test, 0660, tx_enable_test_show, tx_enable_test_store);
static struct kobj_attribute target_len_attr =
    __ATTR(target_len, 0660, target_len_show, target_len_store);
static struct kobj_attribute target_count_attr =
    __ATTR(target_count, 0660, target_count_show, target_count_store);

static struct attribute *netif_perf_attrs[] = {
    &rx_enable_test_attr.attr,
    &tx_enable_test_attr.attr,
    &target_len_attr.attr,
    &target_count_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = netif_perf_attrs,
};

static uint32_t dbg_uinit = 0;
static uint32_t dbg_channel = 0;
static uint32_t dbg_descriptor_idx = 0;

static ssize_t
unit_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", dbg_uinit);
}

static ssize_t
unit_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%u", &dbg_uinit);
    return count;
}

static ssize_t
channel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", dbg_channel);
}

static ssize_t
channel_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%u", &dbg_channel);
    return count;
}

static ssize_t
descriptor_idx_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", dbg_descriptor_idx);
}

static ssize_t
descriptor_idx_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%u", &dbg_descriptor_idx);
    return count;
}

static ssize_t
descriptor_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int len = 0;

    if (dbg_uinit >= clx_misc_dev->pci_dev_num) {
        len += sprintf(buf + len, "unit:%u, invalid unit, max unit:%u\n", dbg_uinit, clx_misc_dev->pci_dev_num - 1);
        return len;

    }

    len += clx_dma_drv(dbg_uinit)->dbg_descriptor_show(dbg_uinit, dbg_channel, dbg_descriptor_idx, buf);
    return len;
}

static ssize_t
reg_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int len = 0;

    if (dbg_uinit >= clx_misc_dev->pci_dev_num) {
        len += sprintf(buf + len, "unit:%u, invalid unit, max unit:%u\n", dbg_uinit, clx_misc_dev->pci_dev_num - 1);
        return len;
    }

    len += clx_dma_drv(dbg_uinit)->dbg_reg_show(dbg_uinit, dbg_channel, buf);
    return len;
}

static struct kobj_attribute dbg_unit_attr =
    __ATTR(unit, 0660, unit_show, unit_store);
static struct kobj_attribute dbg_channel_attr =
    __ATTR(channel, 0660, channel_show, channel_store);
static struct kobj_attribute dbg_descriptor_idx_attr =
    __ATTR(descriptor_idx, 0660, descriptor_idx_show, descriptor_idx_store);
static struct kobj_attribute dbg_descriptor_attr =
    __ATTR(descriptor, 0440, descriptor_show, NULL);
static struct kobj_attribute dbg_reg_attr =
    __ATTR(reg, 0440, reg_show, NULL);

static struct attribute *dbg_attrs[] = {
    &dbg_unit_attr.attr,
    &dbg_channel_attr.attr,
    &dbg_descriptor_idx_attr.attr,
    &dbg_descriptor_attr.attr,
    &dbg_reg_attr.attr,
    NULL,
};

static struct attribute_group dbg_attr_group = {
    .attrs = dbg_attrs,
};

extern uint32_t fault_event_mgr;

static ssize_t test_fault_event_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", fault_event_mgr);
}

static ssize_t test_fault_event_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    uint32_t unit = 0;

    sscanf(buf, "%u", &fault_event_mgr);
    if (fault_event_mgr & KNET_FAULT_EVENT_MGR_TEST) {
        knet_fault_event_report(KNET_FAULT_EVENT_KENT_DMA_ALLOC_FAIL);
        knet_fault_event_report(KNET_FAULT_EVENT_KENT_NO_AVAILABLE_DESC);
        knet_fault_event_report(KNET_FAULT_EVENT_KENT_DMA_MAP_SINGLE_FAIL);
        knet_fault_event_report(KNET_FAULT_EVENT_KENT_PCI_PROBE_FAIL);
        knet_fault_event_report_with_format(KNET_FAULT_EVENT_KENT_NO_AVAILABLE_DESC,
            "No available descriptor, ring size is %u.", clx_dma_drv(unit)->ring_size);
    }
    return count;
}

static struct kobj_attribute test_fault_event_attr =
    __ATTR(fault_event_mgr, 0660, test_fault_event_show, test_fault_event_store);

static struct attribute *fault_test_attrs[] = {
    &test_fault_event_attr.attr,
    NULL,
};

static struct attribute_group fault_test_attr_group = {
    .attrs = fault_test_attrs,
};

static struct kobject *netif_perf_kobj;
static struct kobject *dbg_kobj;
static struct kobject *fault_test_kobj;

static struct file_operations clx_fops = {
    .owner = THIS_MODULE,
    .read = clx_read,
    .open = clx_open,
    .release = clx_release,
    .unlocked_ioctl = clx_ioctl,
    .mmap = clx_mmap,
};

static int __init
clx_module_init(void)
{
    int rc;

    clx_misc_dev = kzalloc(sizeof(clx_device_t), GFP_ATOMIC);
    if (!clx_misc_dev)
        return -ENOMEM;

    clx_misc_dev->misc_dev.minor = CLX_MISC_MINOR_NUM;
    clx_misc_dev->misc_dev.name = "clx_dev";
    clx_misc_dev->misc_dev.fops = &clx_fops;

    rc = misc_register(&clx_misc_dev->misc_dev);
    if (rc) {
        dbg_print(DBG_ERR, "Failed to register misc device, rc:%d\n", rc);
        kfree(clx_misc_dev);
        return rc;
    }

    spin_lock_init(&clx_misc_dev->fifo_lock);
    init_waitqueue_head(&clx_misc_dev->isr_wait_queue);

    if (kfifo_alloc(&clx_misc_dev->intr_fifo, CLX_MAX_CHIP_NUM * sizeof(clx_intr_info_t),
                    GFP_KERNEL)) {
        dbg_print(DBG_ERR, "Failed to allocate FIFO\n");
        misc_deregister(&clx_misc_dev->misc_dev);
        kfree(clx_misc_dev);
        return -ENOMEM;
    }

    rc = clx_pci_init();
    if (rc != 0) {
        dbg_print(DBG_ERR, "Failed to init PCI, rc = %d\n", rc);
        goto err_free_fifo;
    }

    rc = clx_dma_init();
    if (rc != 0) {
        dbg_print(DBG_ERR, "Failed to init DMA, rc = %d\n", rc);
        goto err_pci_cleanup;
    }

    rc = clx_netif_init();
    if (rc != 0) {
        dbg_print(DBG_ERR, "Failed to init netif, rc = %d\n", rc);
        goto err_dma_cleanup;
    }

    rc = clx_netlink_init();
    if (rc != 0) {
        dbg_print(DBG_ERR, "Failed to init netlink, rc = %d\n", rc);
        goto err_netif_cleanup;
    }

    /* For netif performance test */
    netif_perf_kobj = kobject_create_and_add("perf_test", &__this_module.mkobj.kobj);
    if (!netif_perf_kobj) {
        rc = -ENOMEM;
        dbg_print(DBG_ERR, "Failed to create kobject for netif performance test\n");
        goto err_netlink_cleanup;
    }

    rc = sysfs_create_group(netif_perf_kobj, &attr_group);
    if (rc) {
        dbg_print(DBG_ERR, "Failed to create sysfs group\n");
        kobject_put(netif_perf_kobj);
        goto err_netlink_cleanup;
    }

    /* For dbg pdma buffer */
    dbg_kobj = kobject_create_and_add("dbg", &__this_module.mkobj.kobj);
    if (!dbg_kobj) {
        kobject_put(netif_perf_kobj);
        rc = -ENOMEM;
        dbg_print(DBG_ERR, "Failed to create kobject for dbg pdma buffer\n");
        goto err_netlink_cleanup;
    }

    rc = sysfs_create_group(dbg_kobj, &dbg_attr_group);
    if (rc) {
        dbg_print(DBG_ERR, "Failed to create sysfs group\n");
        kobject_put(netif_perf_kobj);
        kobject_put(dbg_kobj);
        goto err_netlink_cleanup;
    }

    fault_test_kobj = kobject_create_and_add("fault_test", &__this_module.mkobj.kobj);
    if (!dbg_kobj) {
        kobject_put(netif_perf_kobj);
        kobject_put(dbg_kobj);
        rc = -ENOMEM;
        dbg_print(DBG_ERR, "Failed to create kobject for fault test\n");
        goto err_netlink_cleanup;
    }

    rc = sysfs_create_group(fault_test_kobj, &fault_test_attr_group);
    if (rc) {
        dbg_print(DBG_ERR, "Failed to create sysfs group\n");
        kobject_put(netif_perf_kobj);
        kobject_put(dbg_kobj);
        kobject_put(fault_test_kobj);
        goto err_netlink_cleanup;
    }

    return 0;

    /* Error handling and cleanup */
err_netlink_cleanup:
    clx_netlink_deinit();
err_netif_cleanup:
    clx_netif_deinit();
err_dma_cleanup:
    clx_dma_deinit();
err_pci_cleanup:
    clx_pci_deinit();
err_free_fifo:
    kfifo_free(&clx_misc_dev->intr_fifo);
    misc_deregister(&clx_misc_dev->misc_dev);
    kfree(clx_misc_dev);

    return rc;
}

static void __exit
clx_module_exit(void)
{
    uint32_t unit = 0;

    clx_netlink_deinit();
    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        clx_interrupt_deinit(unit);
    }
    clx_netif_deinit();
    clx_dma_deinit();

    sysfs_remove_group(netif_perf_kobj, &attr_group);
    kobject_put(netif_perf_kobj);

    sysfs_remove_group(dbg_kobj, &dbg_attr_group);
    kobject_put(dbg_kobj);

    sysfs_remove_group(fault_test_kobj, &fault_test_attr_group);
    kobject_put(fault_test_kobj);

    clx_pci_deinit();
    dbg_print(DBG_DEBUG, "[debug] clx_pci_deinit\n");
    misc_deregister(&clx_misc_dev->misc_dev);
    dbg_print(DBG_DEBUG, "[debug] misc_deregister\n");

    kfifo_free(&clx_misc_dev->intr_fifo);
    kfree(clx_misc_dev);
    dbg_print(DBG_DEBUG, "[debug] kfree clx_misc_dev\n");
}

module_init(clx_module_init);
module_exit(clx_module_exit);

module_param(intr_mode, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(intr_mode, "0: INTx, 1: MSI, 2: MSIx ");

module_param(verbosity, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(verbosity,
                 "bit0:critical, bit1:error, bit2:warning, bit3:infor,"
                 "bit4:debug, bit5:tx, bit6:rx, bit7:intf, bit8:profile, "
                 "bit9:common, bit10:netlink");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("Clounix Kernel Module");
MODULE_INFO(build_time, "Compiled on " KNET_DEV_VER_COMPILE_TIME);
MODULE_INFO(build_hash, KNET_DEV_VER_GIT_HASH);
