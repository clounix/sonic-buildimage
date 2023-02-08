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
#include "drv_fpga_clx.h"
#include "clx_driver.h"

extern void __iomem *clounix_fpga_base;

#define FPGA_GLOBAL_CFG_BASE 0x100
#define FPGA_RESET_CFG_BASE  (FPGA_GLOBAL_CFG_BASE+0) 
#define P12V_STBY_EN   1
#define RESET_MUX_BIT  4


static DEFINE_SPINLOCK(fpga_msi_lock);

static struct notifier_block reboot_nb = {0};
static struct notifier_block restart_nb = {0};


struct int_flag_reg
{
   unsigned int int_flag0_addr;
   unsigned int int_flag1_addr;
   unsigned int int_flag2_addr;
};

struct irq_vector
{
    int irq_mask_bit;
    char *irq_name;
    int mark_type;  /*mark: 1 有细分寄存器要查询；0 无细分寄存器*/
    struct int_flag_reg flag_regs;
};

const struct irq_vector fpga_irq_arry[] = {
    {IRQ_CTRL_RST_PSU_BIT, "psu interrupt",0,{0,0,0}}, /*58*/
    {IRQ_CTRL_RST_TEMP_BIT, "temperature sensor interruput",0,{0,0,0}},
    {IRQ_CTRL_RST_PMBUS_BIT, "PMbus alert interruput",0,{0,0,0}},
    {IRQ_CTRL_RST_USB_BIT, "usb over current  interruput",0,{0,0,0}},
    {IRQ_CTRL_RST_FAN_BIT, "fan alarm interruput",0,{0,0,0}},
    {IRQ_CTRL_RST_SPF_PRES_BIT, "sfp present interruput",1,{FPGA_CPLD0_PRS_TRIG_ADDR,FPGA_CPLD1_PRS_TRIG_ADDR,FPGA_QDD_IRQ_TRIG_ADDR}},
    {IRQ_CTRL_RST_SPF_INT_BIT, "sfp interruput",1,{FPGA_CPLD0_INT_TRIG_ADDR,FPGA_CPLD1_INT_TRIG_ADDR,FPGA_QDD_IRQ_TRIG_ADDR}},
};


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
/*used for FPGA interrupt register test*/
int fpga_irq_test(void)
{
#if 0
    unsigned int irq_flag_g,int_flag0,int_flag1,int_flag2;
    int total_irq,i;

    spin_lock(&fpga_msi_lock);
    /*pci_read_config_word(pdev, pdev->msi_cap + PCI_MSI_DATA_32, &data);*/
    /*get interrupt flag*/
    irq_flag_g = readl(clounix_fpga_base + FPGA_IRQ_TRIG_ADDR);
    /*clear|disable FPGA interrupt flag*/
    writel(IRQ_CTRL_DISABLE_INT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);
    /*writel(0x00000000, clounix_fpga_base + FPGA_DEBUG_TRIG_ADDR);*/
    /*print interrupt information*/
    total_irq = sizeof(fpga_irq_arry) / sizeof(struct irq_vector);
    for(i=0; i<total_irq; i++){
        if( fpga_irq_arry[i].irq_mask_bit & irq_flag_g ){
            LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s: generate %s,irq_flag_g=0x%x.\n", __func__, fpga_irq_arry[i].irq_name,irq_flag_g);
            if(fpga_irq_arry[i].mark_type==1){
                int_flag0 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag0_addr);
                int_flag1 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag1_addr);
                int_flag2 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag2_addr);
                LOG_ERR(CLX_DRIVER_TYPES_FPGA, "cpld0_flag = 0x%x,cpld1_flag = 0x%x,qdd_flag = 0x%x.\n",int_flag0,int_flag1,int_flag2);
            }
            
        }
    }
    /*enable FPGA interrupt */
    writel(IRQ_CTRL_ENABLE_INT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);    
    spin_unlock(&fpga_msi_lock);
#endif
    return 0;

}
EXPORT_SYMBOL_GPL(fpga_irq_test);


static irqreturn_t clounix_fpga_irq_hd(int irq, void *dev_id)
{
    struct pci_dev *pdev = dev_id;
    unsigned short data;
    unsigned int irq_flag_g,int_flag0,int_flag1,int_flag2;
    int total_irq,i;

    spin_lock(&fpga_msi_lock);
    pci_read_config_word(pdev, pdev->msi_cap + PCI_MSI_DATA_32, &data);
    LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s: %x\n", __func__, data);
#if 0
    /*get interrupt flag*/
    irq_flag_g = readl(clounix_fpga_base + FPGA_IRQ_TRIG_ADDR);
    LOG_INFO(CLX_DRIVER_TYPES_FPGA, "%s: generate msi interrupt,irq_flag_g=0x%x.\n", __func__, irq_flag_g);
    /*clear|disable FPGA interrupt flag*/
    writel(IRQ_CTRL_DISABLE_INT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);
    /*writel(0x00000000, clounix_fpga_base + FPGA_DEBUG_TRIG_ADDR);*/
    /*print interrupt information*/
    total_irq = sizeof(fpga_irq_arry) / sizeof(struct irq_vector);
    for(i=0; i<total_irq; i++){
        if( fpga_irq_arry[i].irq_mask_bit & irq_flag_g ){
            LOG_INFO(CLX_DRIVER_TYPES_FPGA, "%s: generate %s,irq_flag_g=0x%x.\n", __func__, fpga_irq_arry[i].irq_name,irq_flag_g);
            if(fpga_irq_arry[i].mark_type==1){
                int_flag0 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag0_addr);
                int_flag1 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag1_addr);
                int_flag2 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag2_addr);
                LOG_INFO(CLX_DRIVER_TYPES_FPGA, "cpld0_flag = 0x%x,cpld1_flag = 0x%x,qdd_flag = 0x%x.\n",int_flag0,int_flag1,int_flag2);
            }
            
        }
    }
    /*enable FPGA interrupt */
    writel(IRQ_CTRL_ENABLE_INT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);    
#endif
    spin_unlock(&fpga_msi_lock);

    return IRQ_HANDLED;
}


static ssize_t get_sys_fpga_power_cycle(struct device *dev, struct device_attribute *da,
             char *buf)
{  
    uint32_t data=0,value = 0;

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
#if 0
    if (pci_find_capability(pdev, PCI_CAP_ID_MSI) == 0) {
        LOG_ERR(CLX_DRIVER_TYPES_FPGA, "%s[%d] MSI not support.\r\n", __func__, __LINE__);
        return -EPERM;
    }
#endif
        
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

    //writel(IRQ_CTRL_CFG_RST_BIT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);
    reboot_nb.notifier_call = sys_led_reboot_work;
    restart_nb.notifier_call = sys_led_reboot_work;

    register_reboot_notifier(&reboot_nb);
    register_restart_handler(&restart_nb);
    err = sysfs_create_file(&pdev->dev.kobj, &attr.attr);
    if (err) {
	LOG_ERR(CLX_DRIVER_TYPES_FPGA,  "sysfs_create_file error status %d\r\n", err);
    }
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
