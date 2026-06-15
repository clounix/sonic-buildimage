#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include "pddf_client_defs.h"
#include "pddf_i2c_algo.h"

static int *log_level = &fpgapci_log_level;

extern void __iomem * fpga_ctl_addr;
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);
extern int (*pddf_i2c_pci_add_numbered_bus)(struct i2c_adapter *, int);

#define FPGA_I2C_DEFAULT_RETRY 3
#define FPGA_I2C_TIMEOUT (msecs_to_jiffies(250))
#define FPGA_I2C_MASTER_MGR_RST (1 << 31)
#define FPGA_I2C_MASTER_MGR_ENABLE ((1 << 30) | (1 << 29) | (0x54 << 16))
#define FPGA_I2C_MASTER_TX_FINISH_MASK (0x80000000UL)
#define FPGA_I2C_MASTER_TX_ERROR_MASK (0x40000000UL)
#define FPGA_I2C_MASTER_MGR_RD_BYTE (0x81 << 24)
#define FPGA_I2C_MASTER_MGR_WT_BYTE (0x84 << 24)
#define FPGA_I2C_MASTER_MGR_RD_WORD (0x82 << 24)
#define FPGA_I2C_MASTER_MGR_WT_WORD (0x88 << 24)
#define FPGA_I2C_MASTER_MGR_WT_NONE (0x85 << 24)

#define FPGA_I2C_MASTER_CFG_ADDR (0x00)
#define FPGA_I2C_MASTER_CTRL_ADDR (0x04)
#define FPGA_I2C_MASTER_STATUS_ADDR (0x08)
#define FPGA_I2C_MASTER_DBG_ADDR (0x0c)
#define FPGA_I2C_MASTER_16BIT_ADDR (0x10)
#define FPGA_I2C_MASTER_CHANNEL_SEL_ADDR (0x14)

#define XFPGA_RAM_BASE_ADDR  (0x130000)
#define XFPGA_RAM_SIZE       (0x10000)


struct master_priv_data {
    struct i2c_adapter *adap;
    struct mutex lock;
    void __iomem *mmio;
    int ram_base_addr;
};

static struct master_priv_data group_priv[I2C_PCI_MAX_BUS];

static void clounix_i2c_master_dump_reg(struct master_priv_data *priv)
{
    pddf_dbg(FPGA, "FPGA_I2C_MASTER_CFG_ADDR 0x%x\n", readl(priv->mmio + FPGA_I2C_MASTER_CFG_ADDR));
    pddf_dbg(FPGA, "FPGA_I2C_MASTER_CTRL_ADDR 0x%x\n", readl(priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR));
    pddf_dbg(FPGA, "FPGA_I2C_MASTER_STATUS_ADDR 0x%x\n", readl(priv->mmio + FPGA_I2C_MASTER_STATUS_ADDR));
    pddf_dbg(FPGA, "FPGA_I2C_MASTER_16BIT_ADDR 0x%x\n", readl(priv->mmio + FPGA_I2C_MASTER_16BIT_ADDR));
    pddf_dbg(FPGA, "FPGA_I2C_MASTER_CHANNEL_SEL_ADDR 0x%x\n", readl(priv->mmio + FPGA_I2C_MASTER_CHANNEL_SEL_ADDR));
}

static int clounix_i2c_wait_bus_tx_done(struct master_priv_data *priv)
{
    unsigned int data;
    unsigned long timeout = jiffies + FPGA_I2C_TIMEOUT;

    do
    {
        data = readl(priv->mmio + FPGA_I2C_MASTER_STATUS_ADDR);

        if (data & FPGA_I2C_MASTER_TX_FINISH_MASK)
        {
            if (data & FPGA_I2C_MASTER_TX_ERROR_MASK)
            {
                clounix_i2c_master_dump_reg(priv);

                pddf_dbg(FPGA, "clounix_i2c_wait_bus_tx_done data ECOMM error\n");

                return -ECOMM;
            }

            return 0;
        }

    } while (time_before(jiffies, timeout));

    pddf_dbg(FPGA, "clounix_i2c_wait_bus_tx_done data ETIMEDOUT error\r\n");

    return -ETIMEDOUT;
}

static u32 clounix_i2c_func(struct i2c_adapter *a)
{
    //return ((I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) & (~I2C_FUNC_SMBUS_QUICK)) | I2C_FUNC_SMBUS_BLOCK_DATA;
    return (I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static int clounix_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr = 0, r_addr = 0, w_addr = 0, reg_addr = 0;
    unsigned int *tmp_addr = NULL;
    unsigned int tmp_value = 0, i = 0, j = 0;

    mutex_lock(&priv->lock);

    if (num == 1) // write
    {
        p = &msgs[0];

        if (p->flags & I2C_M_TEN)
            goto out;

        addr = i2c_8bit_addr_from_msg(p);
        w_addr = (addr & (~(0x01)));
        r_addr = (addr | 0x01);

        tmp_value = 0;
        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);
        writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);

        if (p->len == 2)
        {
            tmp_value = 0;
            tmp_value = (FPGA_I2C_MASTER_MGR_WT_BYTE | ((p->buf[0] & 0xFF) << 16) | (1 << 8) | p->buf[1]);
            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        else if (p->len > 2)
        {
            tmp_addr = (unsigned int *)(priv->mmio + priv->ram_base_addr);

            for (j = 1; j < p->len; j += 4)
            {
                tmp_value = p->buf[j];
                if ((j + 1) >= p->len)
                {
                    writel(tmp_value, tmp_addr);
                    break;
                }
                tmp_value |= (p->buf[j + 1] << 8);
                if ((j + 2) > p->len)
                {
                    writel(tmp_value, tmp_addr);
                    break;
                }
                tmp_value |= (p->buf[j + 2] << 16);
                if ((j + 3) > p->len)
                {
                    writel(tmp_value, tmp_addr);
                    break;
                }
                tmp_value |= (p->buf[j + 3] << 24);
                writel(tmp_value, tmp_addr);
                tmp_addr++;
            }

            tmp_value = 0;
            tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((p->buf[0] & 0xFF) << 16) | ((p->len - 1) << 8));
            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
    }
    else // read
    {
        for (i = 0; i < num; i++)
        {
            p = &msgs[i];

            if (p->flags & I2C_M_TEN)
                goto out;

            addr = i2c_8bit_addr_from_msg(p);
            w_addr = (addr & (~(0x01)));
            r_addr = (addr | 0x01);

            if (p->flags & I2C_M_RD)
            {
                tmp_value = 0;
                tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);
                writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);

                if (p->len == 1)
                {
                    tmp_value = 0;
                    tmp_value = (FPGA_I2C_MASTER_MGR_RD_BYTE | (reg_addr & 0xFF) << 16 | (p->len << 8));
                    writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                    if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                        goto out;
                    else
                        p->buf[0] = readb(priv->mmio + FPGA_I2C_MASTER_STATUS_ADDR);
                }
                else
                {
                    tmp_value = 0;
                    tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((reg_addr & 0xFF) << 16) | ((p->len) << 8));
                    writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                    if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                        goto out;
                    else
                    {
                        tmp_addr = (unsigned int *)(priv->mmio + priv->ram_base_addr);
                        for (j = 0; j < p->len; j += 4)
                        {
                            tmp_value = readl(tmp_addr);
                            p->buf[j] = (tmp_value & 0xFF);

                            if ((j + 1) >= p->len)
                                break;
                            p->buf[j + 1] = ((tmp_value >> 8) & 0xFF);

                            if ((j + 2) >= p->len)
                                break;
                            p->buf[j + 2] = ((tmp_value >> 16) & 0xFF);

                            if ((j + 3) >= p->len)
                                break;
                            p->buf[j + 3] = ((tmp_value >> 24) & 0xFF);

                            tmp_addr++;
                        }
                    }
                }
            }
            else
            {
                reg_addr = p->buf[0];
            }
        }
    }

    mutex_unlock(&priv->lock);

    return num;

out:
    tmp_value = 0;
    tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE);
    writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);
    mutex_unlock(&priv->lock);
    return -ETIMEDOUT;
}

static int clounix_i2c_smbus_xfer(struct i2c_adapter *adap, unsigned short addr, unsigned short flags, 
                       char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    unsigned int tmp_value = 0;
    unsigned char r_addr = 0, w_addr = 0, i = 0, data_size = 0;
    unsigned int *tmp_addr = NULL;

    mutex_lock(&priv->lock);
    //pddf_dbg(FPGA, "addr: %#x, read_write: %d, command : %#x, size: %d\n", addr, read_write, command, size);
    addr = (addr & 0x7f) << 1;
    w_addr = addr;
    r_addr = (addr | 0x01);

    switch (size) {
        case I2C_SMBUS_BYTE:
            tmp_value = 0;
            tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);
            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);
            if (read_write == I2C_SMBUS_READ) {
#if 0
                tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);
                writel((FPGA_I2C_MASTER_MGR_RD_BYTE | ((command & 0xFF) << 16) | (data & 0xFF)), priv->mmio + FPGA_I2C_MASTER_CTRL);
                if (clounix_i2c_wait_bus_tx_done(priv) != 0) {
                    goto out;
                } else {
                    data->byte = readb(priv->mmio + FPGA_I2C_MASTER_STAT);
                }
#endif
            } else {
                tmp_value = 0;
                tmp_value = (FPGA_I2C_MASTER_MGR_WT_NONE | ((command & 0xFF) << 16) | (0x01 << 8));
                writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0) {
                    goto out;
                }
            }

            break;
        
        case I2C_SMBUS_BYTE_DATA:
            tmp_value = 0;
            tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);
            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);

            if (read_write == I2C_SMBUS_READ) {
                tmp_value = 0;
                tmp_value = (FPGA_I2C_MASTER_MGR_RD_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));
                writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0) {
                    goto out;
                } else {
                    data->byte = readb(priv->mmio + FPGA_I2C_MASTER_STATUS_ADDR);
                }
            } else {
                tmp_value = 0;
                tmp_value = (FPGA_I2C_MASTER_MGR_WT_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));
                writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0) {
                    goto out;
                }
            }

            break;
        
        case I2C_SMBUS_WORD_DATA:
            tmp_value = 0;
            tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);
            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);

            if (read_write == I2C_SMBUS_READ)
            {
                tmp_value = 0;
                tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((command & 0xFF) << 16) | (0x02 << 8));
                writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0) {
                    goto out;
                } else {
                    data->word = readw((priv->mmio + priv->ram_base_addr));
                }
            } else {
                writew(data->word, (priv->mmio + priv->ram_base_addr));
                tmp_value = 0;
                tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((command & 0xFF) << 16) | (0x02 << 8));
                writel(tmp_value, (priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR));

                if (clounix_i2c_wait_bus_tx_done(priv) != 0) {
                    goto out;
                }
            }

            break;
        case I2C_SMBUS_BLOCK_DATA:
            tmp_value = 0;
            tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);
            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);

            if (read_write == I2C_SMBUS_READ) {
                tmp_value = 0;
                tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((command & 0xFF) << 16) | ((I2C_SMBUS_BLOCK_MAX + 1) << 8));
                writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0) {
                    goto out;
                } else {
                    tmp_value = 0;
                    tmp_value = readl(priv->mmio + priv->ram_base_addr);
                    data_size = (tmp_value & 0xFF);

                    // LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "I2C_SMBUS_BLOCK_DATA I2C_SMBUS_READ data_size %d\r\n", data_size);
                    if (data_size > I2C_SMBUS_BLOCK_MAX) {
                        goto out;
                    }

                    tmp_addr = (unsigned int *)(priv->mmio + priv->ram_base_addr);
                    for (i = 0; i <= data_size; i += 4) {
                        tmp_value = readl(tmp_addr);
                        data->block[i] = (tmp_value & 0xFF);

                        if ((i + 1) > data_size) {
                            break;
                        }
                        data->block[i + 1] = ((tmp_value >> 8) & 0xFF);

                        if ((i + 2) > data_size) {
                            break;
                        }
                        data->block[i + 2] = ((tmp_value >> 16) & 0xFF);

                        if ((i + 3) > data_size) {
                            break;
                        }
                        data->block[i + 3] = ((tmp_value >> 24) & 0xFF);

                        tmp_addr++;
                    }
                }
            } else {
                data_size = data->block[0];
                tmp_addr = (unsigned int *)(priv->mmio + priv->ram_base_addr);
                for (i = 0; i <= data_size; i += 4) {
                    tmp_value = (data->block[i] + (data->block[i + 1] << 8) + (data->block[i + 2] << 16) + (data->block[i + 3] << 24));
                    writel(tmp_value, tmp_addr);
                    tmp_addr++;
                }

                tmp_value = 0;
                tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((command & 0xFF) << 16) | ((data_size + 1) << 8));
                writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);
                if (clounix_i2c_wait_bus_tx_done(priv) != 0) {
                    goto out;
                }
            }

            break;

        default:
            break;
    }
    mutex_unlock(&priv->lock);
    return 0;

out:
    tmp_value = 0;
    tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE);
    writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);
    mutex_unlock(&priv->lock);
    return -ETIMEDOUT;
}

static struct i2c_algorithm clounix_i2c_algo = {
    .smbus_xfer = clounix_i2c_smbus_xfer,
    .master_xfer = clounix_i2c_xfer,
    .functionality = clounix_i2c_func,
};


static int adap_data_init(struct i2c_adapter *adap, int i2c_ch_index)
{
    struct fpgapci_devdata *pci_privdata = 0;
    //int err = 0;
    
    pci_privdata = (struct fpgapci_devdata*) dev_get_drvdata(adap->dev.parent);
    if (pci_privdata == 0) {
        pddf_err(FPGA, "[%s]: ERROR pci_privdata is 0\n", __FUNCTION__);
        return -1;
    }

    pddf_dbg(FPGA, "[%s] index: [%d] fpga_data__base_addr:0x%p"
        " fpgapci_bar_len:0x%08lx fpga_i2c_ch_base_addr:0x%p ch_size=0x%x supported_i2c_ch=%d",
             __FUNCTION__, i2c_ch_index, pci_privdata->fpga_data_base_addr,
            pci_privdata->bar_length, pci_privdata->fpga_i2c_ch_base_addr,
            pci_privdata->fpga_i2c_ch_size, pci_privdata->max_fpga_i2c_ch);

    if (i2c_ch_index >= pci_privdata->max_fpga_i2c_ch || pci_privdata->max_fpga_i2c_ch > I2C_PCI_MAX_BUS) {
        pddf_err(FPGA, "[%s]: ERROR i2c_ch_index=%d max_ch_index=%d out of range: %d\n",
             __FUNCTION__, i2c_ch_index, pci_privdata->max_fpga_i2c_ch, I2C_PCI_MAX_BUS);
        return -1;
    }
    memset(&group_priv[i2c_ch_index], 0, sizeof(group_priv[0]));
    group_priv[i2c_ch_index].adap = adap;
    adap->owner = THIS_MODULE;
    adap->algo = &clounix_i2c_algo;
    adap->retries = FPGA_I2C_DEFAULT_RETRY;
    group_priv[i2c_ch_index].mmio =  pci_privdata->fpga_i2c_ch_base_addr +
                          i2c_ch_index* pci_privdata->fpga_i2c_ch_size;
    group_priv[i2c_ch_index].ram_base_addr = ((0x1300 + i2c_ch_index* pci_privdata->fpga_i2c_ch_size) << 8) - (0x1200 + i2c_ch_index* pci_privdata->fpga_i2c_ch_size);
    pddf_dbg(FPGA, "index: %d, before : %#x, after: %#x \n", i2c_ch_index, ((0x1300 + i2c_ch_index* pci_privdata->fpga_i2c_ch_size) << 8), (0x1200 * i2c_ch_index* pci_privdata->fpga_i2c_ch_size));
    pddf_dbg(FPGA, "index: %d, ram_base_addr : %#x \n", i2c_ch_index, group_priv[i2c_ch_index].ram_base_addr);
    mutex_init(&group_priv[i2c_ch_index].lock);
    i2c_set_adapdata(adap, &group_priv[i2c_ch_index]);
#if 0
    err = fpga_i2c_reinit(&group_priv[i2c_ch_index], XIIC_I2C_INIT_TIMEOUT);
    if (err != 0)
    {
        return err;
    }
#endif
    return 0;
}
static int pddf_i2c_pci_add_numbered_bus_default (struct i2c_adapter *adap, int i2c_ch_index)
{
    int ret = 0;

    if (fpga_ctl_addr == NULL) {
        pddf_info(FPGA, "resource not iomap\n");
        return -ENXIO;
    }
    adap_data_init(adap, i2c_ch_index);
    adap->algo = &clounix_i2c_algo;

    ret = i2c_add_numbered_adapter(adap);
    return ret;
}
/*
 * FPGAPCI APIs
 */
int board_i2c_fpgapci_read(uint32_t offset)
{
	int data = 0;

    if(NULL != fpga_ctl_addr){
	    data=ioread32(fpga_ctl_addr+offset);
    }
	return data;
}


int board_i2c_fpgapci_write(uint32_t offset, uint32_t value)
{
    if(NULL != fpga_ctl_addr){
	    iowrite32(value, fpga_ctl_addr+offset);
    }    
	return (0);
}
static int __init pddf_xilinx_device_7021_algo_init(void)
{
    pddf_i2c_pci_add_numbered_bus = pddf_i2c_pci_add_numbered_bus_default;
    ptr_fpgapci_read = board_i2c_fpgapci_read;
    ptr_fpgapci_write = board_i2c_fpgapci_write;
    pddf_info(FPGA, "[%s--%d]\n", __FUNCTION__, __LINE__);
    return 0;
}

static void __exit pddf_xilinx_device_7021_algo_exit(void)
{
    pddf_info(FPGA, "[%s]\n", __FUNCTION__);
    pddf_i2c_pci_add_numbered_bus = NULL;
    ptr_fpgapci_read = NULL;
    ptr_fpgapci_write = NULL;
    return;
}

module_init (pddf_xilinx_device_7021_algo_init);
module_exit (pddf_xilinx_device_7021_algo_exit);

MODULE_DESCRIPTION("Xilinx Corporation Device 7021 FPGAPCIe I2C-Bus algorithm");
MODULE_LICENSE("GPL");
