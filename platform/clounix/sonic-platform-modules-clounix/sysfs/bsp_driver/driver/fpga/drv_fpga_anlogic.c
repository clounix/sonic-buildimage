#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/bits.h>

#include "clounix/clounix_fpga.h"
#include "clounix/io_signal_ctrl.h"
#include "device_driver_common.h"
#include "clx_driver.h"

extern void __iomem *clounix_fpga_base;

#define LED_BIT (2)
#define LED_MASK (0x3)
#define sys_led_reg_offset (0x200)

static struct notifier_block reboot_nb = {0};
static struct notifier_block restart_nb = {0};

static int set_sys_led_status(int type, int num, int val)
{
    void __iomem *sysled_stat;
    int bit = type - PSU_LED_G;
    int data;

    if (bit < 0)
        return -EIO;

    sysled_stat = clounix_fpga_base + sys_led_reg_offset;
    data = readb(sysled_stat);

    if (val > 0) {
        data = data | (1 << bit);
    } else {
        data = data & (~(1 << bit));
    }

    writeb(data, sysled_stat);

    return 1;
}

static int get_sys_led_status(int type, int num)
{
    void __iomem *sysled_stat;
    int bit = type - PSU_LED_G;
    int data;

    if (bit < 0)
        return 0;

    sysled_stat = clounix_fpga_base + sys_led_reg_offset;
    data = readb(sysled_stat);

    data = (data >> bit) & 0x1;

    return data;
}

static void init_io_sig(void)
{
    add_io_sig_desc(PSU_LED_G, 0, get_sys_led_status, set_sys_led_status);
    add_io_sig_desc(PSU_LED_R, 0, get_sys_led_status, set_sys_led_status);

    add_io_sig_desc(SYS_LED_G, 0, get_sys_led_status, set_sys_led_status);
    add_io_sig_desc(SYS_LED_R, 0, get_sys_led_status, set_sys_led_status);

    add_io_sig_desc(FAN_LED_G, 0, get_sys_led_status, set_sys_led_status);
    add_io_sig_desc(FAN_LED_R, 0, get_sys_led_status, set_sys_led_status);

    return;
}

static void rm_io_sig(void)
{
    del_io_sig_desc(PSU_LED_G, 0);
    del_io_sig_desc(PSU_LED_R, 0);

    del_io_sig_desc(SYS_LED_G, 0);
    del_io_sig_desc(SYS_LED_R, 0);

    del_io_sig_desc(FAN_LED_G, 0);
    del_io_sig_desc(FAN_LED_R, 0);

    return;
}

static ssize_t led_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    struct fpga_device_attribute *fpga_attr;
    unsigned char green;
    unsigned char red;
    unsigned char blue;
    int type;
    char *led_color[] = {
        "off",
        "green",
        "red",
        "yellow",
        "blue"
    };

    fpga_attr = to_fpga_dev_attr(attr);
    type = PSU_LED_G + fpga_attr->index;
    switch (type) {
        case ID_LED_B:
            blue = read_io_sig_desc(type, 0);
            blue = blue == 0 ? 0 : (blue + 4);
            return sprintf(buf, "%s\n", led_color[blue]);

        default:
            green = read_io_sig_desc(type, 0);
            red = read_io_sig_desc(type + 1, 0);
            red = red << 1;
            return sprintf(buf, "%s\n", led_color[green + red]);
    }
}

static ssize_t led_store(struct device *dev, struct device_attribute *attr,
         const char *buf, size_t count)
{
    struct fpga_device_attribute *fpga_attr;
    char data = *buf - '0';
    char green;
    char red;
    char reg_data;
    int type;

    if (data < 0 || data > LED_MASK)
        return -EPERM;

    reg_data = buf[0] - '0';
    if (reg_data < 0 || reg_data > 3)
        return -EIO;

    green = reg_data & 0x1;
    red = (reg_data >> 1) & 0x1;

    type = PSU_LED_G + fpga_attr->index;
    write_io_sig_desc(type, 0, green);
    write_io_sig_desc(type + 1, 0, red);

    return count;
}

FPGA_DEVICE_ATTR(id_led, S_IRUGO | S_IWUSR, led_show, led_store, 6);
FPGA_DEVICE_ATTR(fan_led, S_IRUGO | S_IWUSR, led_show, led_store, 4);
FPGA_DEVICE_ATTR(sys_led, S_IRUGO | S_IWUSR, led_show, led_store, 2);
FPGA_DEVICE_ATTR(pwr_led, S_IRUGO | S_IWUSR, led_show, led_store, 0);

static struct attribute *clx12800_fpga_attrs[] = {
    &fpga_dev_attr_id_led.dev_attr.attr,
    &fpga_dev_attr_fan_led.dev_attr.attr,
    &fpga_dev_attr_sys_led.dev_attr.attr,
    &fpga_dev_attr_pwr_led.dev_attr.attr,
    NULL,
};

static const struct attribute_group clx12800_fpga_group = {
    .attrs = clx12800_fpga_attrs,
};

static int sys_led_reboot_work(struct notifier_block *nb, unsigned long action, void *data)
{
    unsigned char reg_data = readb(clounix_fpga_base + sys_led_reg_offset);

    reg_data = reg_data & ~(LED_MASK << LED_BIT);
    writeb(reg_data, clounix_fpga_base + sys_led_reg_offset);

    return NOTIFY_DONE;
}

int drv_fpga_anlogic_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
{
    int err;
    
    if (pci_find_capability(pdev, PCI_CAP_ID_MSI) == 0) {
        printk(KERN_ERR "%s[%d] MSI not support.\r\n", __func__, __LINE__);
        return -EPERM;
    }
        
    err = pci_enable_device(pdev);
    if (err) {
        printk(KERN_ERR "%s[%d] can't enbale device.\r\n", __func__, __LINE__);
        return -EPERM;
    }
    
    if (request_mem_region(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0), "clounix_fpga") == 0) {
        printk(KERN_ERR "%s[%d] can't request iomem (0x%llx).\r\n", __func__, __LINE__, pci_resource_start(pdev, 0));
        err = -EBUSY;
        goto err_request;
    }
    
    pci_set_master(pdev);
    clounix_fpga_base = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    if (clounix_fpga_base  == NULL) {
        printk(KERN_ERR "%s[%d] ioremap resource fail.\r\n", __func__, __LINE__);
        err = -EIO;
        goto err_ioremap;
    } 
    pci_set_drvdata(pdev, clounix_fpga_base);

    LOG_ERR(CLX_DRIVER_TYPES_FPGA, "support %d msi vector\n", pci_msi_vec_count(pdev));
    err = pci_alloc_irq_vectors(pdev, 1, 32, PCI_IRQ_MSI);
    if (err < 0) {
        printk(KERN_ERR "%s[%d] MSI vector alloc fail.\r\n", __func__, __LINE__);
        goto err_alloc_msi;
    }
    
    err = sysfs_create_group(&pdev->dev.kobj, &clx12800_fpga_group);
    if (err) {
        printk(KERN_ERR "%s[%d] sysfs register fail.\r\n", __func__, __LINE__);
        goto err_sysfs;
    }
    reboot_nb.notifier_call = sys_led_reboot_work;
    restart_nb.notifier_call = sys_led_reboot_work;

    register_reboot_notifier(&reboot_nb);
    register_restart_handler(&restart_nb);

    init_io_sig();

    return 0;

err_sysfs:
    pci_free_irq_vectors(pdev);
err_alloc_msi:
    iounmap(clounix_fpga_base);
err_ioremap:
    pci_clear_master(pdev);
    release_mem_region(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
err_request:
    pci_disable_device(pdev);

    return err;
}

void drv_fpga_anlogic_remove(struct pci_dev *pdev)
{
    rm_io_sig();
    unregister_reboot_notifier(&reboot_nb);
    unregister_restart_handler(&restart_nb);
    
    sysfs_remove_group(&pdev->dev.kobj, &clx12800_fpga_group);
    pci_free_irq_vectors(pdev);
    iounmap(clounix_fpga_base);
    pci_clear_master(pdev);
    release_mem_region(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    pci_disable_device(pdev);

    return;
}

static const struct pci_device_id drv_fpga_anlogic_pci_tbl[] = {
    { PCI_DEVICE(0x16c3, 0xabcd) },
    { 0, },
};
MODULE_DEVICE_TABLE(pci, drv_fpga_anlogic_pci_tbl);

static struct pci_driver clounix_priv_pci_driver = {
    .name       = "drv_fpga_anlogic",
    .id_table   = drv_fpga_anlogic_pci_tbl,
    .probe      = drv_fpga_anlogic_probe,
    .remove     = drv_fpga_anlogic_remove,
    .suspend    = NULL,
    .resume     = NULL,
};

int drv_fpga_anlogic_init(void **driver)
{
    return pci_register_driver(&clounix_priv_pci_driver);
}

void drv_fpga_anlogic_exit(void **driver)
{
    pci_unregister_driver(&clounix_priv_pci_driver);
    return;
}
