// SPDX-License-Identifier: GPL-2.0-only
/*
 * lpc_cpld.c - Driver for LPC-attached CPLD on Intel Atom C3000
 *
 * Provides sysfs attributes:
 *   /sys/kernel/lpc_cpld/addr  -> CPLD shadow address register (offset 0x00)
 *   /sys/kernel/lpc_cpld/data  -> CPLD shadow data register (offset 0x01)
 *
 * No /dev node is created. Only these two registers are exposed.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define DRIVER_NAME "lpc_cpld"
#define DRIVER_VERSION "5.0"

/* PCI identification */
#define LPC_PCI_VENDOR_INTEL 0x8086
#define LPC_PCI_DEVICE_C3000 0x19dc

/* I/O window for CPLD ¨C base 0x0700 (SHADOW_ADDR) */
#define CPLD_LPC_BASE_DEFAULT 0x0700
#define CPLD_LPC_SIZE 0x100 /* Keep 256 bytes for hardware decode, but only use 0x00 and 0x01 */

/* LPC bridge configuration registers (PCI config space) */
#define LPC_GEN_DEC_RANGE_4 0x90

/* LGIR4 ¨C low 16 bits: base 0x0700 | enable bit 0 */
#define LGIR4_VALUE 0x00000701

/* Module parameters */
static ushort lpc_base = CPLD_LPC_BASE_DEFAULT;
module_param(lpc_base, ushort, 0444);
MODULE_PARM_DESC(lpc_base, "LPC I/O base address for CPLD (default 0x700)");

static uint lgir4_value = LGIR4_VALUE;
module_param(lgir4_value, uint, 0444);
MODULE_PARM_DESC(lgir4_value, "Value to write to LGIR4 (default 0x00000701)");

static bool force_load = true;
module_param(force_load, bool, 0444);
MODULE_PARM_DESC(force_load, "Continue loading even if LGIR4 write verification fails (default true)");

static DEFINE_MUTEX(lpc_lock);

static struct pci_dev *lpc_pdev = NULL;
static bool io_region_claimed = false;

/* kobject for sysfs directory */
static struct kobject *lpc_kobj = NULL;

/* ---------------------------------------------------------------------
 * Internal I/O functions
 * --------------------------------------------------------------------- */
static unsigned char lpc_cpld_read_reg(u16 offset)
{
    unsigned char val = 0xFF;
    unsigned int port = lpc_base + (offset & 0xFF);

    mutex_lock(&lpc_lock);
    val = inb(port);
    pr_debug(DRIVER_NAME ": read offset 0x%02x from port 0x%04x -> 0x%02x\n",
             offset, port, val);
    mutex_unlock(&lpc_lock);

    return val;
}

static void lpc_cpld_write_reg(u16 offset, u8 value)
{
    unsigned int port = lpc_base + (offset & 0xFF);

    mutex_lock(&lpc_lock);
    outb(value, port);
    pr_debug(DRIVER_NAME ": write offset 0x%02x to port 0x%04x <- 0x%02x\n",
             offset, port, value);
    mutex_unlock(&lpc_lock);
}

/* ---------------------------------------------------------------------
 * Sysfs attributes: addr (offset 0x00) and data (offset 0x01)
 * --------------------------------------------------------------------- */
static ssize_t addr_show(struct kobject *kobj, struct kobj_attribute *attr,
                         char *buf)
{
    u8 val = lpc_cpld_read_reg(0x00);
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t addr_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t count)
{
    unsigned long val;
    int ret = kstrtoul(buf, 0, &val);
    if (ret)
        return ret;
    if (val > 0xFF)
        return -EINVAL;
    lpc_cpld_write_reg(0x00, (u8)val);
    return count;
}
static struct kobj_attribute addr_attr = __ATTR(addr, 0644, addr_show, addr_store);

static ssize_t data_show(struct kobject *kobj, struct kobj_attribute *attr,
                         char *buf)
{
    u8 val = lpc_cpld_read_reg(0x01);
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t data_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t count)
{
    unsigned long val;
    int ret = kstrtoul(buf, 0, &val);
    if (ret)
        return ret;
    if (val > 0xFF)
        return -EINVAL;
    lpc_cpld_write_reg(0x01, (u8)val);
    return count;
}
static struct kobj_attribute data_attr = __ATTR(data, 0644, data_show, data_store);

/* ---------------------------------------------------------------------
 * Module initialization
 * --------------------------------------------------------------------- */
static int __init lpc_cpld_init(void)
{
    int ret = 0;
    u32 reg_val = 0;

    pr_info(DRIVER_NAME ": loading version %s\n", DRIVER_VERSION);

    lpc_pdev = pci_get_device(LPC_PCI_VENDOR_INTEL, LPC_PCI_DEVICE_C3000, NULL);
    if (!lpc_pdev)
    {
        pr_err(DRIVER_NAME ": LPC controller (8086:19dc) not found\n");
        return -ENODEV;
    }

    ret = pci_enable_device(lpc_pdev);
    if (ret)
    {
        pr_err(DRIVER_NAME ": failed to enable PCI device\n");
        goto err_put_pci;
    }

    if (!request_region(lpc_base, CPLD_LPC_SIZE, DRIVER_NAME))
    {
        pr_err(DRIVER_NAME ": I/O region 0x%x-0x%x already in use\n",
               lpc_base, lpc_base + CPLD_LPC_SIZE - 1);
        ret = -EBUSY;
        goto err_disable_pci;
    }
    io_region_claimed = true;

    /* Configure LGIR4 ¨C write only low 16 bits */
    ret = pci_read_config_dword(lpc_pdev, LPC_GEN_DEC_RANGE_4, &reg_val);
    if (ret)
    {
        pr_err(DRIVER_NAME ": failed to read LGIR4\n");
        goto err_release_region;
    }
    pr_info(DRIVER_NAME ": LGIR4 current = 0x%08x\n", reg_val);

    ret = pci_write_config_dword(lpc_pdev, LPC_GEN_DEC_RANGE_4, lgir4_value);
    if (ret)
    {
        pr_err(DRIVER_NAME ": failed to write LGIR4\n");
        goto err_release_region;
    }

    ret = pci_read_config_dword(lpc_pdev, LPC_GEN_DEC_RANGE_4, &reg_val);
    if (ret || (reg_val & 0xFFFF) != (lgir4_value & 0xFFFF))
    {
        pr_warn(DRIVER_NAME ": LGIR4 verification mismatch: expected 0x%04x, read 0x%04x (full=0x%08x)\n",
                (uint16_t)(lgir4_value & 0xFFFF), (uint16_t)(reg_val & 0xFFFF), reg_val);
        if (!force_load)
        {
            pr_err(DRIVER_NAME ": force_load disabled, aborting\n");
            ret = -EIO;
            goto err_release_region;
        }
        pr_info(DRIVER_NAME ": force_load enabled, continuing despite mismatch\n");
    }
    else
    {
        pr_info(DRIVER_NAME ": LGIR4 set to 0x%08x (I/O window 0x%x-0x%x enabled)\n",
                reg_val, lpc_base, lpc_base + CPLD_LPC_SIZE - 1);
    }

    /* Create sysfs directory under /sys/kernel/ */
    lpc_kobj = kobject_create_and_add(DRIVER_NAME, kernel_kobj);
    if (!lpc_kobj)
    {
        pr_err(DRIVER_NAME ": failed to create sysfs kobject\n");
        ret = -ENOMEM;
        goto err_release_region;
    }

    /* Create addr and data attributes */
    ret = sysfs_create_file(lpc_kobj, &addr_attr.attr);
    if (ret)
    {
        pr_err(DRIVER_NAME ": failed to create addr sysfs file\n");
        goto err_kobject_del;
    }

    ret = sysfs_create_file(lpc_kobj, &data_attr.attr);
    if (ret)
    {
        pr_err(DRIVER_NAME ": failed to create data sysfs file\n");
        sysfs_remove_file(lpc_kobj, &addr_attr.attr);
        goto err_kobject_del;
    }

    pr_info(DRIVER_NAME ": sysfs /sys/kernel/%s/addr and data created\n", DRIVER_NAME);
    pr_info(DRIVER_NAME ": driver loaded successfully (force_load=%d)\n", force_load);
    return 0;

err_kobject_del:
    kobject_put(lpc_kobj);
    lpc_kobj = NULL;
err_release_region:
    if (io_region_claimed)
    {
        release_region(lpc_base, CPLD_LPC_SIZE);
        io_region_claimed = false;
    }
err_disable_pci:
    pci_disable_device(lpc_pdev);
err_put_pci:
    pci_dev_put(lpc_pdev);
    lpc_pdev = NULL;
    return ret;
}

static void __exit lpc_cpld_exit(void)
{
    /* Remove sysfs files and directory */
    if (lpc_kobj)
    {
        sysfs_remove_file(lpc_kobj, &data_attr.attr);
        sysfs_remove_file(lpc_kobj, &addr_attr.attr);
        kobject_put(lpc_kobj);
        lpc_kobj = NULL;
    }

    /* Release I/O region */
    if (io_region_claimed)
    {
        release_region(lpc_base, CPLD_LPC_SIZE);
        io_region_claimed = false;
    }

    /* Disable and release PCI device */
    if (lpc_pdev)
    {
        pci_disable_device(lpc_pdev);
        pci_dev_put(lpc_pdev);
        lpc_pdev = NULL;
    }

    pr_info(DRIVER_NAME ": driver unloaded\n");
}

module_init(lpc_cpld_init);
module_exit(lpc_cpld_exit);

MODULE_AUTHOR("Your Name <you@example.com>");
MODULE_DESCRIPTION("LPC CPLD driver with sysfs addr/data (no /dev) for Intel Atom C3000");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

static const struct pci_device_id lpc_cpld_pci_ids[] = {
    {PCI_DEVICE(LPC_PCI_VENDOR_INTEL, LPC_PCI_DEVICE_C3000)},
    {
        0,
    }};
MODULE_DEVICE_TABLE(pci, lpc_cpld_pci_ids);
