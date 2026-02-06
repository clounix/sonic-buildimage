

#include "knet_dev.h"
#include "knet_pci.h"
#include "knet_buffer.h"

#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/mm.h>

LIST_HEAD(dma_buffer_list);
DEFINE_MUTEX(dma_list_mutex);

int
clx_ioctl_dma_alloc(uint32_t unit, unsigned long arg)
{
    struct device *ptr_dev = &clx_misc_dev->clx_pci_dev[unit]->pci_dev->dev;
    struct clx_ioctl_dma_buffer *ioc_dma_buffer;
    size_t copy_size = offsetof(struct clx_ioctl_dma_buffer, virt_addr);
    void *virt_addr;

    ioc_dma_buffer = kmalloc(sizeof(*ioc_dma_buffer), GFP_KERNEL);
    if (!ioc_dma_buffer)
        return -ENOMEM;

    if (copy_from_user(ioc_dma_buffer, (void __user *)arg, copy_size)) {
        dbg_print(DBG_ERR, "copy_from_user failed. unit=%u\n", unit);
        kfree(ioc_dma_buffer);
        return -EFAULT;
    }

    ioc_dma_buffer->size = round_up(ioc_dma_buffer->size, PAGE_SIZE);
    virt_addr =
        dma_alloc_coherent(ptr_dev, ioc_dma_buffer->size, &ioc_dma_buffer->bus_addr, GFP_KERNEL);
    if (!virt_addr) {
        dbg_print(DBG_ERR, "dma_alloc_coherent failed.size=%llx\n", ioc_dma_buffer->size);
        kfree(ioc_dma_buffer);
        return -ENOMEM;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    ioc_dma_buffer->phy_addr = ioc_dma_buffer->bus_addr;
#else
    ioc_dma_buffer->phy_addr = virt_to_phys(virt_addr);
#endif
    ioc_dma_buffer->virt_addr = virt_addr;
    ioc_dma_buffer->alloc_dev = ptr_dev;

    if (copy_to_user((void __user *)arg, ioc_dma_buffer, copy_size)) {
        dbg_print(DBG_ERR, "copy_to_user failed\n");
        dma_free_coherent(ptr_dev, ioc_dma_buffer->size, virt_addr, ioc_dma_buffer->bus_addr);
        kfree(ioc_dma_buffer);
        return -EFAULT;
    }

    mutex_lock(&dma_list_mutex);
    list_add(&ioc_dma_buffer->list, &dma_buffer_list);
    mutex_unlock(&dma_list_mutex);
    dbg_print(DBG_INFO, "alloc bus_addr:0x%llx, phy_addr:0x%llx, size:0x%llx\n",
              ioc_dma_buffer->bus_addr, ioc_dma_buffer->phy_addr, ioc_dma_buffer->size);
    return 0;
}

int
clx_ioctl_dma_free(uint32_t unit, unsigned long arg)
{
    struct device *ptr_dev = &clx_misc_dev->clx_pci_dev[unit]->pci_dev->dev;
    struct clx_ioctl_dma_buffer ioc_dma_buffer;
    struct clx_ioctl_dma_buffer *info;
    size_t copy_size = offsetof(struct clx_ioctl_dma_buffer, virt_addr);

    if (copy_from_user(&ioc_dma_buffer, (void __user *)arg, copy_size)) {
        dbg_print(DBG_ERR, "copy_from_user failed\n");
        return -EFAULT;
    }

    mutex_lock(&dma_list_mutex);
    list_for_each_entry(info, &dma_buffer_list, list)
    {
        if (info->bus_addr == ioc_dma_buffer.bus_addr) {
            dma_free_coherent(ptr_dev, info->size, info->virt_addr, info->bus_addr);
            list_del(&info->list);
            dbg_print(DBG_INFO, "free bus_addr:0x%llx, phy_addr:0x%llx, size:0x%llx\n",
                      ioc_dma_buffer.bus_addr, ioc_dma_buffer.phy_addr, info->size);
            kfree(info);
            break;
        }
    }
    mutex_unlock(&dma_list_mutex);
    return 0;
}

int
cleanup_usr_dma_buffer(void)
{
    struct clx_ioctl_dma_buffer *info, *tmp;
    struct device *ptr_dev = NULL;

    if (clx_misc_dev->pci_dev_num > 0) {
        ptr_dev = &clx_misc_dev->clx_pci_dev[0]->pci_dev->dev;
    } else {
        return -EINVAL;
    }

    mutex_lock(&dma_list_mutex);
    list_for_each_entry_safe(info, tmp, &dma_buffer_list, list)
    {
        dma_free_coherent(ptr_dev, info->size, info->virt_addr, info->bus_addr);
        list_del(&info->list);
        kfree(info);
    }
    mutex_unlock(&dma_list_mutex);

    return 0;
}
