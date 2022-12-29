#include <linux/types.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/msi.h>
#include <linux/bits.h>
#include <linux/i2c.h>
#include <linux/pci.h>
#include <linux/types.h>

#include "device_driver_common.h"
#include "clx_driver.h"

#define DEFAULT_RETRY 3

extern void __iomem *clounix_fpga_base;

struct master_conf {
    int offset;
    char *name;
};

static const struct master_conf priv_conf[] = {
    {0x200000, "fpga-psu"},
    {0x210000, "fpga-adm"},
    {0x220000, "fpga-pmbus"},
    {0x230000, "fpga-pll"},
    {0x240000, "fpga-tmp"},
    {0x250000, "fpga-fan"},
};

struct master_priv_data {
    struct i2c_adapter adap;
    struct mutex lock;
    void __iomem *mmio;
};

static struct master_priv_data *group_priv;
static DEFINE_SPINLOCK(fpga_msi_lock);

static int clounix_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    return 0;
}

static int clounix_i2c_smbus_xfer(struct i2c_adapter *adap, unsigned short addr, unsigned short flags, char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    return 0;
}

static u32 clounix_i2c_func(struct i2c_adapter *a)
{
    return ((I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) & (~I2C_FUNC_SMBUS_QUICK)) | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static struct i2c_algorithm clounix_i2c_algo = {
    .smbus_xfer = clounix_i2c_smbus_xfer,
    .master_xfer = clounix_i2c_xfer,
    .functionality = clounix_i2c_func,
};

static irqreturn_t clounix_fpga_irq_hd(int irq, void *dev_id)
{
    struct pci_dev *pdev = dev_id;
    unsigned short data;

    spin_lock(&fpga_msi_lock);
    pci_read_config_word(pdev, pdev->msi_cap + PCI_MSI_DATA_32, &data);
    printk(KERN_ALERT "%s: %x\n", __func__, data);
    spin_unlock(&fpga_msi_lock);

    return IRQ_HANDLED;
}

static int fpga_i2c_reinit(struct master_priv_data *priv, unsigned long after)
{
    return 0;
}

static int irq_nums;
int drv_i2c_anlogic_init(void **driver)
{
    struct pci_dev *pdev = pci_get_device(0x16c3, 0xabcd, NULL);
    struct master_priv_data *priv;
    void __iomem *base;
    struct i2c_adapter *adap;
    int total_adap;
    int i, err;

    if (pdev == NULL) {
        return -ENXIO;
    }

    if (clounix_fpga_base == NULL) {
        return -ENXIO;
    }
    base = pci_get_drvdata(pdev);

    total_adap = sizeof(priv_conf)/sizeof(struct master_conf);
    group_priv = kzalloc(sizeof(struct master_priv_data) * total_adap, GFP_KERNEL);
    if (group_priv == NULL) {
        return -ENOMEM;
    }

    irq_nums = pci_alloc_irq_vectors(pdev, 1, 32, PCI_IRQ_MSI | PCI_IRQ_AFFINITY);
    if (irq_nums < 0) {
        err = irq_nums;
        goto err_vectors;
    }

    for (i=0; i<irq_nums; i++) {
        err = request_irq(pci_irq_vector(pdev, i), clounix_fpga_irq_hd, IRQF_SHARED, pdev->driver->name, pdev);
        if (err < 0) {
            LOG_ERR(CLX_DRIVER_TYPES_I2C_MASTER, "%s[%d] IRQ request fail.\r\n", __func__, __LINE__);
            irq_nums = i;
            goto err_irq;
        }
    }

    priv = group_priv;
    for (i=0; i<total_adap; i++) {
        adap = &priv[i].adap;
        adap->owner = THIS_MODULE;
        adap->algo = &clounix_i2c_algo;
        adap->retries = DEFAULT_RETRY;
        adap->dev.parent = &pdev->dev;
        adap->dev.of_node = pdev->dev.of_node;
        strlcpy(adap->name, priv_conf[i].name, sizeof(adap->name));
        i2c_set_adapdata(adap, &priv[i]);
        priv[i].mmio = base + priv_conf[i].offset;
        mutex_init(&(priv[i].lock));

        err =  fpga_i2c_reinit(&priv[i], 100);
        if (err != 0)
            goto err_i2c_group;

        err = i2c_add_adapter(adap);
        if (err)
            goto err_i2c_group;
    }
    return 0;

err_i2c_group:
    while(--i >= 0) {
        i2c_del_adapter(adap);
    }
err_irq:
    for (i=0; i<irq_nums; i++) {
        free_irq(pci_irq_vector(pdev, i), pdev);
    }
    pci_free_irq_vectors(pdev);
err_vectors:
    kfree(group_priv);

    return err;
}

void drv_i2c_anlogic_exit(void **driver)
{
    struct pci_dev *pdev = pci_get_device(0x16c3, 0xabcd, NULL);
    int i, total_adap;
    struct master_priv_data *priv = group_priv;

    if (clounix_fpga_base == NULL)
        return;

    total_adap = sizeof(priv_conf)/sizeof(struct master_conf);
    for (i=0; i<total_adap; i++) {
        i2c_del_adapter(&(priv[i].adap));
    }

    for (i=0; i<irq_nums; i++) {
        free_irq(pci_irq_vector(pdev, i), pdev);
    }
    pci_free_irq_vectors(pdev);

    kfree(group_priv);
    return;
}
