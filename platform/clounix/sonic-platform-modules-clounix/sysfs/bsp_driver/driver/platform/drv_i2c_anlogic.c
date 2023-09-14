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
#include <linux/i2c-smbus.h>

#include "device_driver_common.h"
#include "clx_driver.h"

#define DEFAULT_RETRY 3

#define FPGA_I2C_BASE (0x800)
#define FPGA_I2C_MGR_CFG (FPGA_I2C_BASE + 0x00)
#define FPGA_I2C_MGR_CTRL (FPGA_I2C_BASE + 0x04)
#define FPGA_I2C_MGR_STAT (FPGA_I2C_BASE + 0x08)
#define FPGA_I2C_MGR_TX (FPGA_I2C_BASE + 0x10)
#define FPGA_I2C_MGR_RX (FPGA_I2C_BASE + 0x40)
#define FPGA_I2C_MGR_MUX (FPGA_I2C_BASE + 0x70)

#define I2C_CFG_RST (GENMASK(31, 31))
#define I2C_CFG_ENABLE (GENMASK(30, 30))
#define I2C_CFG_ABORT (GENMASK(29, 29))
#define I2C_CFG_STAT_CLR (GENMASK(28, 28))
#define I2C_CFG_START (GENMASK(24, 24))
#define I2C_CFG_BSP (GENMASK(15, 14))
#define I2C_CFG_ACK_POL (GENMASK(13, 13))
#define I2C_CFG_CLK_DIV (GENMASK(10, 0))

#define I2C_STAT_BUS_BUSY (GENMASK(31, 31))
#define I2C_STAT_BUS_ABORT (GENMASK(30, 30))
#define I2C_STAT_BUS_ERR (GENMASK(23, 16))

#define I2C_RX_BYTE(byte) ((byte) << 0)
#define I2C_TX_BYTE(byte) ((byte) << 8)
#define I2C_SLAVE_ADDR(addr) ((addr) << 17)

#define CLOUNIX_I2C_TIMEOUT (msecs_to_jiffies(100))

#define MAX_DATA_LEN (32)

/* for fpga msi irq */
#define mgr_irq_ctl (0x700)
#define mgr_irq_ctl_4 (0x710)
#define mgr_irq_stat_4 (0x720)

#define SMBUS_ALERT_ENABLE_MASK  (0x1)
#define PLL_ALERT_MASK (0x1)
#define TEMP_ALERT_MASK (0x2)
#define PMBUS_ALERT_MASK (0x4)

#define MSI_SMBUS_ALERT_IRQ (0x0)
#define SMBUS_ALERT_ARA (0x0c)

extern void __iomem *clounix_fpga_base;

static struct mutex mux_lock;

struct master_conf {
    int offset;
    char *name;
};

static const struct master_conf priv_conf[] = {
    {0x0, "fpga-idprom"},
    {0x1, "fpga-temp"},
    {0x2, "fpga-pll"},
    {0x3, "fpga-pol"},
    {0x4, "fpga-adm1166"},
};

struct master_priv_data {
    struct i2c_adapter adap;
    int mux;
    void __iomem *mmio;
};

static struct master_priv_data *group_priv;

static void force_delay(unsigned int us)
{
    unsigned long time_out;

    time_out = jiffies + usecs_to_jiffies(us);
    while (time_after(jiffies, time_out) == 0) {};

    return;
}

static void disable_fpga_smbus_alert_irq(struct pci_dev *pdev)
{
    void __iomem *fpga_bar_base = pci_get_drvdata(pdev);
    unsigned int data;

    if (fpga_bar_base == NULL)
        return;

    data = readl(fpga_bar_base + mgr_irq_ctl);
    data &= ~(SMBUS_ALERT_ENABLE_MASK);
    writel(data, fpga_bar_base + mgr_irq_ctl);

    writel(PLL_ALERT_MASK | TEMP_ALERT_MASK | PMBUS_ALERT_MASK, fpga_bar_base + mgr_irq_ctl_4);

    return;
}

static void enable_fpga_smbus_alert_irq(struct pci_dev *pdev)
{
    void __iomem *fpga_bar_base = pci_get_drvdata(pdev);
    unsigned int data;

    if (fpga_bar_base == NULL)
        return;

    data = readl(fpga_bar_base + mgr_irq_ctl);
    data |= SMBUS_ALERT_ENABLE_MASK;
    writel(data, fpga_bar_base + mgr_irq_ctl);

    writel(PLL_ALERT_MASK | TEMP_ALERT_MASK | PMBUS_ALERT_MASK, fpga_bar_base + mgr_irq_ctl_4);

    return;
}

static int fpga_i2c_reinit(struct master_priv_data *priv, unsigned int after_us)
{

    writel(I2C_CFG_RST | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);

    force_delay(after_us);

    writel(I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);

    return 0;
}

static irqreturn_t clounix_fpga_smbus_alert_hd(int irq, void *dev_id)
{
    struct pci_dev *pdev = dev_id;
    void __iomem *fpga_bar_base = pci_get_drvdata(pdev);
    unsigned int data;

    if (fpga_bar_base == NULL)
        goto irq_exit;

    data = readl(fpga_bar_base + mgr_irq_stat_4);
    /* this is alert process, may done in the furtrue.
    if ((data & PLL_ALERT_MASK) != 0) {

    }

    if ((data & TEMP_ALERT_MASK) != 0) {

    }

    if ((data & PMBUS_ALERT_MASK) != 0) {

    }
    */
    data = data & (PLL_ALERT_MASK | TEMP_ALERT_MASK | PMBUS_ALERT_MASK);
    writel(data, fpga_bar_base + mgr_irq_ctl_4);

irq_exit:
    return IRQ_HANDLED;
}

static int wait_busy_ide(struct master_priv_data *priv)
{
    unsigned long time_out;

    time_out = jiffies + CLOUNIX_I2C_TIMEOUT;
    while ((readl(priv->mmio + FPGA_I2C_MGR_STAT) & I2C_STAT_BUS_BUSY) != 0) {
        if (time_after(jiffies, time_out) == 1) {
            fpga_i2c_reinit(priv, 15);
            return -1;
        }
    }

    return 0;
}

static int clounix_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned int ctrl_data;
    int i, j, k;
    unsigned int rd_data;
    unsigned int tmp_buf[8 + 1] = {0};

    for (k=0; k<num; k++) {
        p = &msgs[k];
        if (p->flags & I2C_M_TEN)
            goto out;
        if (p->len > (I2C_SMBUS_BLOCK_MAX + 2))
            goto out;

        if (wait_busy_ide(priv) != 0)
            return -EBUSY;

        writel(priv->mux, priv->mmio + FPGA_I2C_MGR_MUX);

        ctrl_data = I2C_SLAVE_ADDR(p->addr);
        if (p->flags & I2C_M_RD) {
            ctrl_data = ctrl_data | I2C_RX_BYTE(p->len);
            writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
            writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);

            if (wait_busy_ide(priv) != 0)
                goto out;

            for (i=0; i<p->len; ) {
                rd_data = readl(priv->mmio + FPGA_I2C_MGR_RX);
                for (j=0; j<sizeof(int); j++) {
                    if (p->len > (i+j)) {
                        p->buf[p->len-(i+j) - 1] = (rd_data >> (j*8)) & 0xff;
                    } else {
                        break;
                    }
                }
                i += j;
            }
        } else {
            ctrl_data = ctrl_data | I2C_TX_BYTE(p->len);
            writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);

            i = 0;
            while (i<p->len) {
                tmp_buf[i/sizeof(int)] |= p->buf[i] << (24-8*(i%sizeof(int)));
                i++;
            }

            for (i=i/4; i>=0; i--) {
                writel(tmp_buf[i], priv->mmio + FPGA_I2C_MGR_TX);
            }

            writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
        }
    }

    return num;

out:
    return -EIO;
}

static int clounix_i2c_smbus_xfer(struct i2c_adapter *adap, unsigned short addr, unsigned short flags, char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    unsigned int ctrl_data;
    int i, j;
    unsigned int rd_data;
    unsigned int wt_data;
    unsigned char tmp_buf[I2C_SMBUS_BLOCK_MAX + 1] = {0};

    if (wait_busy_ide(priv) != 0)
        return -EBUSY;

    writel(priv->mux, priv->mmio + FPGA_I2C_MGR_MUX);
    switch(size) {
        case I2C_SMBUS_BYTE:
            ctrl_data = I2C_SLAVE_ADDR(addr);
            if (read_write == I2C_SMBUS_READ) {
                ctrl_data = ctrl_data | I2C_RX_BYTE(1);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel(I2C_CFG_START | I2C_CFG_ENABLE | I2C_CFG_ACK_POL, priv->mmio + FPGA_I2C_MGR_CFG);
                if (wait_busy_ide(priv) != 0)
                    goto err_out;

                data->byte = readl(priv->mmio + FPGA_I2C_MGR_RX) & 0xff;
            } else {
                ctrl_data = ctrl_data | I2C_TX_BYTE(1);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel((command << 24), priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
            }
            break;
        case I2C_SMBUS_BYTE_DATA:
            ctrl_data = I2C_SLAVE_ADDR(addr);
            if (read_write == I2C_SMBUS_READ) {
                ctrl_data = ctrl_data | I2C_TX_BYTE(1) | I2C_RX_BYTE(1);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel((command << 24), priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE | I2C_CFG_ACK_POL, priv->mmio + FPGA_I2C_MGR_CFG);
                if (wait_busy_ide(priv) != 0)
                    goto err_out;
                
                data->byte = readl(priv->mmio + FPGA_I2C_MGR_RX) & 0xff;
            } else {
                ctrl_data = ctrl_data | I2C_TX_BYTE(2);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel((command << 24) | (data->byte << 16), priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
            }
            break;
        case I2C_SMBUS_WORD_DATA:
            ctrl_data = I2C_SLAVE_ADDR(addr);
            if (read_write == I2C_SMBUS_READ) {
                ctrl_data = ctrl_data | I2C_TX_BYTE(1) | I2C_RX_BYTE(2);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel(command << 24, priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE | I2C_CFG_ACK_POL, priv->mmio + FPGA_I2C_MGR_CFG);
                if (wait_busy_ide(priv) != 0)
                    goto err_out;
                
                data->word = readl(priv->mmio + FPGA_I2C_MGR_RX) & 0xffff;
                data->word = ntohs(data->word);
            } else {
                ctrl_data = ctrl_data | I2C_TX_BYTE(3);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                wt_data = command << 24;
                wt_data |= (data->word & 0xff) << 16;
                wt_data |= ((data->word >> 8 ) & 0xff) << 8;
                writel(wt_data, priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
            }
            break;
        case I2C_SMBUS_BLOCK_DATA:
            ctrl_data = I2C_SLAVE_ADDR(addr);
            if (read_write == I2C_SMBUS_READ) {
                ctrl_data = ctrl_data | I2C_TX_BYTE(1) | I2C_RX_BYTE(I2C_SMBUS_BLOCK_MAX+1);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel(command << 24, priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE | I2C_CFG_ACK_POL, priv->mmio + FPGA_I2C_MGR_CFG);

                if (wait_busy_ide(priv) != 0)
                    return -EIO;
              
                for (i=0; i<I2C_SMBUS_BLOCK_MAX+1; ) {
                    rd_data = readl(priv->mmio + FPGA_I2C_MGR_RX);
                    for (j=0; j<sizeof(int); j++) {
                        if (I2C_SMBUS_BLOCK_MAX >= (i+j)) {
                            tmp_buf[I2C_SMBUS_BLOCK_MAX - (i+j)] = (rd_data >> (j*8)) & 0xff;
                        }
                    }
                    i += j;
                }

                for (i=0; i<=I2C_SMBUS_BLOCK_MAX; i++) {
                    if (tmp_buf[i] != 0 && tmp_buf[i] <= I2C_SMBUS_BLOCK_MAX) {
                        memcpy(data->block, &tmp_buf[i], tmp_buf[i]+1);
                        return 0;
                    }
                }
                goto err_out;
            } else {
                i = 0;
                while (i <= data->block[0]) {
                    tmp_buf[i/sizeof(int)] |= data->block[i] << (24-8*(i%sizeof(int)));
                    i++;
                }

                for (i=i/4; i>=0; i--) {
                    writel(tmp_buf[i], priv->mmio + FPGA_I2C_MGR_TX);
                }
                writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
            }
            break;
    }

    return 0;

err_out:
    return -EIO;
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


void lock_fpga_i2c_bus(struct i2c_adapter *adap, unsigned int flags)
{
    mutex_lock(&mux_lock);
}

int trylock_fpga_i2c_bus(struct i2c_adapter *adap, unsigned int flags)
{
    return -EPERM;
}

void unlock_fpga_i2c_bus(struct i2c_adapter *adap, unsigned int flags)
{
    mutex_unlock(&mux_lock);
}

static struct i2c_lock_operations lock_ops = {
    .lock_bus = lock_fpga_i2c_bus,
    .trylock_bus = NULL,
    .unlock_bus = unlock_fpga_i2c_bus,
};

int drv_i2c_anlogic_init(void **driver)
{
    struct pci_dev *pdev = pci_get_device(0x16c3, 0xabcd, NULL);
    struct master_priv_data *priv;
    void __iomem *base;
    struct i2c_adapter *adap;
    struct i2c_smbus_alert_setup setup;
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

    err = request_threaded_irq(pci_irq_vector(pdev, MSI_SMBUS_ALERT_IRQ), NULL, clounix_fpga_smbus_alert_hd, IRQF_SHARED | IRQF_ONESHOT,  pdev->driver->name, pdev);
    if (err < 0) {
        printk(KERN_ERR "%s[%d] IRQ request fail.\r\n", __func__, __LINE__);
        goto err_irq;
    }

    mutex_init(&mux_lock);

    priv = group_priv;
    for (i=0; i<total_adap; i++) {
        adap = &priv[i].adap;
        adap->owner = THIS_MODULE;
        adap->lock_ops = &lock_ops;
        adap->algo = &clounix_i2c_algo;
        adap->retries = DEFAULT_RETRY;
        adap->dev.parent = &pdev->dev;
        adap->dev.of_node = pdev->dev.of_node;
        adap->timeout = CLOUNIX_I2C_TIMEOUT;
        adap->retries = 2;

        strlcpy(adap->name, priv_conf[i].name, sizeof(adap->name));
        i2c_set_adapdata(adap, &priv[i]);
        priv[i].mmio = base;
        priv[i].mux = priv_conf[i].offset;

        err =  fpga_i2c_reinit(&priv[i], 15);
        if (err != 0)
            goto err_i2c_group;

        err = i2c_add_adapter(adap);
        if (err)
            goto err_i2c_group;
    }

    setup.irq = pci_irq_vector(pdev, MSI_SMBUS_ALERT_IRQ);
    if (setup.irq >= 0) {
        i2c_setup_smbus_alert(&priv[1].adap , &setup);
    }

    enable_fpga_smbus_alert_irq(pdev);

    return 0;

err_i2c_group:
    while(--i >= 0) {
        i2c_del_adapter(adap);
    }
    free_irq(pci_irq_vector(pdev, MSI_SMBUS_ALERT_IRQ), pdev);
err_irq:
    kfree(group_priv);

    return err;
}

void drv_i2c_anlogic_exit(void **driver)
{
    struct pci_dev *pdev = pci_get_device(0x16c3, 0xabcd, NULL);
    int i, total_adap;
    struct master_priv_data *priv = group_priv;

    disable_fpga_smbus_alert_irq(pdev);

    total_adap = sizeof(priv_conf)/sizeof(struct master_conf);
    for (i=0; i<total_adap; i++) {
        i2c_del_adapter(&(priv[i].adap));
    }
    free_irq(pci_irq_vector(pdev, MSI_SMBUS_ALERT_IRQ), pdev);
    kfree(group_priv);

    return;
}
