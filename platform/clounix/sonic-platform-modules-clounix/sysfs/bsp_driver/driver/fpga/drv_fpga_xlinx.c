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
#include "device_driver_common.h"
#include "clx_driver.h"

extern void __iomem *clounix_fpga_base;

#define FPGA_GLOBAL_CFG_BASE 0x100
#define FPGA_RESET_CFG_BASE  (FPGA_GLOBAL_CFG_BASE+0) 
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
    unregister_reboot_notifier(&reboot_nb);
    unregister_restart_handler(&restart_nb);

  //free_irq(pci_irq_vector(pdev, 0), pdev);
  //pci_free_irq_vectors(pdev);
    devm_iounmap(&pdev->dev, clounix_fpga_base);
    pci_clear_master(pdev);
    devm_release_mem_region(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    pci_disable_device(pdev);

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
