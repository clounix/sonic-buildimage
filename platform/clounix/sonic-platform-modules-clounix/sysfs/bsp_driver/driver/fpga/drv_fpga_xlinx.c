#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/msi.h>
#include <linux/notifier.h>
#include <linux/reboot.h>

#include "clounix/clounix_fpga.h"
#include "clounix/io_signal_ctrl.h"
#include "device_driver_common.h"
#include "clx_driver.h"

extern void __iomem *clounix_fpga_base;

#define FPGA_GLOBAL_CFG_BASE 0x100
#define FPGA_RESET_CFG_BASE  (FPGA_GLOBAL_CFG_BASE+0) 
#define P12V_STBY_EN   1
#define RESET_MUX_BIT  4


//static DEFINE_SPINLOCK(fpga_msi_lock);

static struct notifier_block reboot_nb = {0};
static struct notifier_block restart_nb = {0};

#define SYS_LED_BIT (2)
#define SYS_LED_MASK (0x3)
#define sys_led_reg_offset (0x200)
static int sys_led_reboot_work(struct notifier_block *nb, unsigned long action, void *data)
{
    unsigned char reg_data = readb(clounix_fpga_base + sys_led_reg_offset);

    reg_data = reg_data & ~(SYS_LED_MASK << SYS_LED_BIT);
    writeb(reg_data, clounix_fpga_base + sys_led_reg_offset);

    return NOTIFY_DONE;
}

void reset_mux_pca9548(void)
{
    uint32_t data=0;
    if(NULL != clounix_fpga_base){
        data= readl(clounix_fpga_base + FPGA_RESET_CFG_BASE);
        SET_BIT(data, RESET_MUX_BIT);
        writel(data, clounix_fpga_base + FPGA_RESET_CFG_BASE);
        udelay(1);
        CLEAR_BIT(data, RESET_MUX_BIT);
        writel(data, clounix_fpga_base + FPGA_RESET_CFG_BASE);
    }
    return;
}
EXPORT_SYMBOL_GPL(reset_mux_pca9548);

/*
static irqreturn_t clounix_fpga_irq_hd(int irq, void *dev_id)
{
    struct pci_dev *pdev = dev_id;
    unsigned short data;

    spin_lock(&fpga_msi_lock);
    pci_read_config_word(pdev, pdev->msi_cap + PCI_MSI_DATA_32, &data);
    LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s: %x\n", __func__, data);
    spin_unlock(&fpga_msi_lock);

    return IRQ_HANDLED;
}
*/

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
    void __iomem *psu_stat;
    int bit = type - PSU_LED_G;
    int data;

    if (bit < 0)
        return 0;

    psu_stat = clounix_fpga_base + sys_led_reg_offset;
    data = readb(psu_stat);

    data = (data >> bit) & 0x1;

    return data;
}

#define psu_stat_offset (0x608)
static int get_sys_fpga_psu_status(int type, int num)
{
    void __iomem *psu_stat;
    int data;
    int index;

    psu_stat = clounix_fpga_base + psu_stat_offset;
    data = readb(psu_stat);

    switch (type) {
        case PSU_PRST:
            if (num == 0)
                index = 0;
            else
                index = 4;
            break;

        case PSU_ACOK:
            if (num == 0)
                index = 2;
            else
                index = 6;
            break;

        case PSU_PWOK:
            if (num == 0)
                index = 1;
            else
                index = 5;
            break;
    }

    data = (data >> index) & 0x1;
    return data;
}

static void init_io_sig(void)
{
    add_io_sig_desc(PSU_PRST, 0, get_sys_fpga_psu_status, NULL);
    add_io_sig_desc(PSU_PRST, 1, get_sys_fpga_psu_status, NULL);

    add_io_sig_desc(PSU_ACOK, 0, get_sys_fpga_psu_status, NULL);
    add_io_sig_desc(PSU_ACOK, 1, get_sys_fpga_psu_status, NULL);

    add_io_sig_desc(PSU_PWOK, 0, get_sys_fpga_psu_status, NULL);
    add_io_sig_desc(PSU_PWOK, 1, get_sys_fpga_psu_status, NULL);

    add_io_sig_desc(PSU_LED_G, 0, get_sys_led_status, set_sys_led_status);
    add_io_sig_desc(PSU_LED_R, 0, get_sys_led_status, set_sys_led_status);

    add_io_sig_desc(SYS_LED_G, 0, get_sys_led_status, set_sys_led_status);
    add_io_sig_desc(SYS_LED_R, 0, get_sys_led_status, set_sys_led_status);

    add_io_sig_desc(FAN_LED_G, 0, get_sys_led_status, set_sys_led_status);
    add_io_sig_desc(FAN_LED_R, 0, get_sys_led_status, set_sys_led_status);

    add_io_sig_desc(ID_LED_B, 0, get_sys_led_status, set_sys_led_status);

    return;
}

static void rm_io_sig(void)
{
    del_io_sig_desc(PSU_PRST, 0);
    del_io_sig_desc(PSU_PRST, 1);

    del_io_sig_desc(PSU_ACOK, 0);
    del_io_sig_desc(PSU_ACOK, 1);

    del_io_sig_desc(PSU_PWOK, 0);
    del_io_sig_desc(PSU_PWOK, 1);

    del_io_sig_desc(PSU_LED_G, 0);
    del_io_sig_desc(PSU_LED_R, 0);

    del_io_sig_desc(SYS_LED_G, 0);
    del_io_sig_desc(SYS_LED_R, 0);

    del_io_sig_desc(FAN_LED_G, 0);
    del_io_sig_desc(FAN_LED_R, 0);

    del_io_sig_desc(ID_LED_B, 0);

    return;
}

static ssize_t get_sys_fpga_power_cycle(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    uint32_t data=0;

    if(NULL != clounix_fpga_base){
        data= readl(clounix_fpga_base + FPGA_RESET_CFG_BASE);
    }
    return sprintf(buf, "0x%x\n", (data >> P12V_STBY_EN) & 0x1);
}
static ssize_t set_sys_fpga_power_cycle(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    uint32_t value = 0;
    uint32_t data=0;

    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    if(NULL != clounix_fpga_base){
        data= readl(clounix_fpga_base + FPGA_RESET_CFG_BASE);
        if(1 == value)
            SET_BIT(data, P12V_STBY_EN);
        else
            CLEAR_BIT(data, P12V_STBY_EN);
        writel(data, clounix_fpga_base + FPGA_RESET_CFG_BASE);
    }
    return count;
}
static struct device_attribute attr = __ATTR(power_cycle, S_IRUGO | S_IWUSR, get_sys_fpga_power_cycle, set_sys_fpga_power_cycle);
int drv_xilinx_fpga_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
{
    int err;

    if (pci_find_capability(pdev, PCI_CAP_ID_MSI) == 0) {
        LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s[%d] MSI not support.\r\n", __func__, __LINE__);
        return -EPERM;
    }
        
    err = pci_enable_device(pdev);
    if (err) {
        LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s[%d] can't enbale device.\r\n", __func__, __LINE__);
        return -EPERM;
    }
    
    if (devm_request_mem_region(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0), "clounix_fpga") == 0) {
        LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s[%d] can't request iomem (0x%llx).\r\n", __func__, __LINE__, pci_resource_start(pdev, 0));
        err = -EBUSY;
        goto err_request;
    }
    
    pci_set_master(pdev);
    
    clounix_fpga_base = devm_ioremap(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    if (clounix_fpga_base  == NULL) {
        LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s[%d] ioremap resource fail.\r\n", __func__, __LINE__);
        err = -EIO;
        goto err_ioremap;
    } 
    pci_set_drvdata(pdev, clounix_fpga_base);

    LOG_ERR(CLX_DRIVER_TYPES_FPGA, "support %d msi vector\n", pci_msi_vec_count(pdev));
  //err = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_AFFINITY);
  //if (err < 0) {
  //    LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s[%d] MSI vector alloc fail.\r\n", __func__, __LINE__);
  //    goto err_alloc_msi;
  //}

  //err = request_irq(pci_irq_vector(pdev, 0), clounix_fpga_irq_hd, IRQF_SHARED, pdev->driver->name, pdev);
  //if (err < 0) {
  //    LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s[%d] MSI vector alloc fail.\r\n", __func__, __LINE__);
  //    goto err_irq;
  //}

    reboot_nb.notifier_call = sys_led_reboot_work;
    restart_nb.notifier_call = sys_led_reboot_work;

    register_reboot_notifier(&reboot_nb);
    register_restart_handler(&restart_nb);
    err = sysfs_create_file(&pdev->dev.kobj, &attr.attr);
    if (err) {
	    LOG_ERR(CLX_DRIVER_TYPES_FPGA,  "sysfs_create_file error status %d\r\n", err);
    }
    init_io_sig();

    return 0;

//err_irq:
//  pci_free_irq_vectors(pdev);
//err_alloc_msi:
//  devm_iounmap(&pdev->dev, clounix_fpga_base);
err_ioremap:
    pci_clear_master(pdev);
    devm_release_mem_region(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
err_request:
    pci_disable_device(pdev);

    return err;
}

void drv_xilinx_fpga_remove(struct pci_dev *pdev)
{
    rm_io_sig();
    unregister_reboot_notifier(&reboot_nb);
    unregister_restart_handler(&restart_nb);

  //free_irq(pci_irq_vector(pdev, 0), pdev);
  //pci_free_irq_vectors(pdev);
    devm_iounmap(&pdev->dev, clounix_fpga_base);
    pci_clear_master(pdev);
    devm_release_mem_region(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    pci_disable_device(pdev);
    sysfs_remove_file(&pdev->dev.kobj, &attr.attr);

    return;
}

static const struct pci_device_id drv_xilinx_fpga_pci_tbl[] = {
    { PCI_DEVICE(0x10ee, 0x7021) },
    { 0, },
};
MODULE_DEVICE_TABLE(pci, drv_xilinx_fpga_pci_tbl);

static struct pci_driver clounix_priv_pci_driver = {
    .name       = "drv_fpga_xilinx",
    .id_table   = drv_xilinx_fpga_pci_tbl,
    .probe      = drv_xilinx_fpga_probe,
    .remove     = drv_xilinx_fpga_remove,
    .suspend    = NULL,
    .resume     = NULL,
};

int drv_fpga_xilinx_init(void **driver)
{
    return pci_register_driver(&clounix_priv_pci_driver);
}

void drv_fpga_xilinx_exit(void **driver)
{
    pci_unregister_driver(&clounix_priv_pci_driver);
    return;
}
