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

#define DEFAULT_RETRY 3
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

// #define XIIC_MSB_OFFSET (0)
// #define XIIC_REG_OFFSET (0x100+XIIC_MSB_OFFSET)
// /*
//  * Register offsets in bytes from RegisterBase. Three is added to the
//  * base offset to access LSB (IBM style) of the word
//  */
// #define XIIC_CR_REG_OFFSET   (0x00+XIIC_REG_OFFSET) /* Control Register   */
// #define XIIC_SR_REG_OFFSET   (0x04+XIIC_REG_OFFSET) /* Status Register    */
// #define XIIC_DTR_REG_OFFSET  (0x08+XIIC_REG_OFFSET) /* Data Tx Register   */
// #define XIIC_DRR_REG_OFFSET  (0x0C+XIIC_REG_OFFSET) /* Data Rx Register   */
// #define XIIC_ADR_REG_OFFSET  (0x10+XIIC_REG_OFFSET) /* Address Register   */
// #define XIIC_TFO_REG_OFFSET  (0x14+XIIC_REG_OFFSET) /* Tx FIFO Occupancy  */
// #define XIIC_RFO_REG_OFFSET  (0x18+XIIC_REG_OFFSET) /* Rx FIFO Occupancy  */
// #define XIIC_TBA_REG_OFFSET  (0x1C+XIIC_REG_OFFSET) /* 10 Bit Address reg */
// #define XIIC_RFD_REG_OFFSET  (0x20+XIIC_REG_OFFSET) /* Rx FIFO Depth reg  */
// #define XIIC_GPO_REG_OFFSET  (0x24+XIIC_REG_OFFSET) /* Output Register    */

// /* Control Register masks */
// #define XIIC_CR_ENABLE_DEVICE_MASK        0x01  /* Device enable = 1      */
// #define XIIC_CR_TX_FIFO_RESET_MASK        0x02  /* Transmit FIFO reset=1  */
// #define XIIC_CR_MSMS_MASK                 0x04  /* Master starts Txing=1  */
// #define XIIC_CR_DIR_IS_TX_MASK            0x08  /* Dir of tx. Txing=1     */
// #define XIIC_CR_NO_ACK_MASK               0x10  /* Tx Ack. NO ack = 1     */
// #define XIIC_CR_REPEATED_START_MASK       0x20  /* Repeated start = 1     */
// #define XIIC_CR_GENERAL_CALL_MASK         0x40  /* Gen Call enabled = 1   */

// /* Status Register masks */
// #define XIIC_SR_GEN_CALL_MASK             0x01  /* 1=a mstr issued a GC   */
// #define XIIC_SR_ADDR_AS_SLAVE_MASK        0x02  /* 1=when addr as slave   */
// #define XIIC_SR_BUS_BUSY_MASK             0x04  /* 1 = bus is busy        */
// #define XIIC_SR_MSTR_RDING_SLAVE_MASK     0x08  /* 1=Dir: mstr <-- slave  */
// #define XIIC_SR_TX_FIFO_FULL_MASK         0x10  /* 1 = Tx FIFO full       */
// #define XIIC_SR_RX_FIFO_FULL_MASK         0x20  /* 1 = Rx FIFO full       */
// #define XIIC_SR_RX_FIFO_EMPTY_MASK        0x40  /* 1 = Rx FIFO empty      */
// #define XIIC_SR_TX_FIFO_EMPTY_MASK        0x80  /* 1 = Tx FIFO empty      */

// /* Interrupt Status Register masks    Interrupt occurs when...       */
// #define XIIC_INTR_ARB_LOST_MASK           0x01  /* 1 = arbitration lost   */
// #define XIIC_INTR_TX_ERROR_MASK           0x02  /* 1=Tx error/msg complete */
// #define XIIC_INTR_TX_EMPTY_MASK           0x04  /* 1 = Tx FIFO/reg empty  */
// #define XIIC_INTR_RX_FULL_MASK            0x08  /* 1=Rx FIFO/reg=OCY level */
// #define XIIC_INTR_BNB_MASK                0x10  /* 1 = Bus not busy       */
// #define XIIC_INTR_AAS_MASK                0x20  /* 1 = when addr as slave */
// #define XIIC_INTR_NAAS_MASK               0x40  /* 1 = not addr as slave  */
// #define XIIC_INTR_TX_HALF_MASK            0x80  /* 1 = TX FIFO half empty */

/* The following constants specify the depth of the FIFOs */
// #define IIC_RX_FIFO_DEPTH         16    /* Rx fifo capacity               */
// #define IIC_TX_FIFO_DEPTH         16    /* Tx fifo capacity               */

// #define XIIC_DGIER_OFFSET    (XIIC_MSB_OFFSET+0x1C) /* Device Global Interrupt Enable Register */
// #define XIIC_IISR_OFFSET     (XIIC_MSB_OFFSET+0x20) /* Interrupt Status Register */
// #define XIIC_IIER_OFFSET     (XIIC_MSB_OFFSET+0x28) /* Interrupt Enable Register */
// #define XIIC_RESETR_OFFSET   (XIIC_MSB_OFFSET+0x40) /* Reset Register */

// #define XIIC_RESET_MASK             0xAUL

// #define XIIC_PM_TIMEOUT     1000    /* ms */
// /* timeout waiting for the controller to respond */
// #define XIIC_I2C_TIMEOUT    (msecs_to_jiffies(500))
// #define XIIC_I2C_INIT_TIMEOUT    (msecs_to_jiffies(50))

// #define XIIC_TX_DYN_START_MASK            0x0100 /* 1 = Set dynamic start */
// #define XIIC_TX_DYN_STOP_MASK             0x0200 /* 1 = Set dynamic stop */

// #define XIIC_GINTR_ENABLE_MASK      0x80000000UL

// #define CLOUNIX_INIT_TIMEOUT (msecs_to_jiffies(100))

// struct master_conf {
//     int offset;
//     char *name;
// };


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
#if 0
static int tx_fifo_full(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_TX_FIFO_FULL_MASK;
}

// static int tx_fifo_empty(struct master_priv_data *priv)
// {
//     return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_TX_FIFO_EMPTY_MASK;
// }

/*
static int rx_fifo_full(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_RX_FIFO_FULL_MASK;
}
*/

// static int rx_fifo_empty(struct master_priv_data *priv)
// {
//     return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_RX_FIFO_EMPTY_MASK;
// }

/*
static int tx_fifo_space(struct master_priv_data *priv)
{
    return IIC_TX_FIFO_DEPTH - readb(priv->mmio + XIIC_TFO_REG_OFFSET) - 1;
}
*/

// static int bus_busy(struct master_priv_data *priv)
// {
//     return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK;
// }

// static void dump_reg(struct master_priv_data *priv)
// {
//     int i;

//     pddf_dbg(FPGA, "%s[%d]\n", __func__, __LINE__);
//     pddf_dbg(FPGA, "%x\r\n", readl(priv->mmio + 0x1c));
//     pddf_dbg(FPGA, "%x\r\n", readl(priv->mmio + 0x20));
//     pddf_dbg(FPGA, "%x\r\n", readl(priv->mmio + 0x28));
    
//     i = XIIC_CR_REG_OFFSET;
//     while (i<=XIIC_RFD_REG_OFFSET) {
//         pddf_dbg(FPGA, "off %x: %x\r\n", i, readl(priv->mmio + i));
//         i+=4;
//     }
// }

// static int fpga_i2c_reinit(struct master_priv_data *priv, unsigned long after)
// {
//     unsigned long timeout;
    
//     dump_reg(priv);
//     writeb(XIIC_RESET_MASK, priv->mmio + XIIC_RESETR_OFFSET);
//     timeout = jiffies +  after;
//     while(time_after(jiffies, timeout)) {};

//     writeb(IIC_RX_FIFO_DEPTH - 1, priv->mmio + XIIC_RFD_REG_OFFSET);
    
//     writeb(XIIC_CR_ENABLE_DEVICE_MASK , priv->mmio + XIIC_CR_REG_OFFSET);
//     writeb(XIIC_CR_ENABLE_DEVICE_MASK | XIIC_CR_TX_FIFO_RESET_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
//     timeout = jiffies +  after;
//     while(time_after(jiffies, timeout)) {};
//     writeb(0, priv->mmio + XIIC_CR_REG_OFFSET);

//     timeout = jiffies + after;
//     while (rx_fifo_empty(priv) == 0) {
//         if (time_after(jiffies, timeout)) {
//             pddf_err(FPGA, "%s reinit timeout\r\n", priv->adap->name);
//             return -ETIMEDOUT;
//         }
//     }
    
    dump_reg(priv);
    return 0;
}
#endif
static u32 clounix_i2c_func(struct i2c_adapter *a)
{
    //return ((I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) & (~I2C_FUNC_SMBUS_QUICK)) | I2C_FUNC_SMBUS_BLOCK_DATA;
    return (I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) | I2C_FUNC_SMBUS_BLOCK_DATA;
}

#if 0
static int wait_bus_busy_status(struct master_priv_data *priv, unsigned int status)
{
    unsigned long timeout;
    
//     timeout = jiffies + XIIC_I2C_TIMEOUT; 
//     while (bus_busy(priv) != status) {
//         if (time_after(jiffies, timeout)) {
//             pddf_info(FPGA, "bus status err %x\r\n", status);
//             return 0;
//         }
//     }

//     return 1;
// }

// static int wait_bus_tx_done(struct master_priv_data *priv)
// {
//     unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

//     while (tx_fifo_empty(priv) == 0) {
//         if (time_after(jiffies, timeout)) {
//             pddf_info(FPGA, "tx fifo not empty\r\n");
//             return 0;
//         }
//     }

//     return 1;
// }

// static int wait_bus_can_tx(struct master_priv_data *priv)
// {
//     unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

//     while (tx_fifo_full(priv) != 0) {
//         if (time_after(jiffies, timeout)) {
//             pddf_info(FPGA, "tx fifo full\r\n");
//             return 0;
//         }
//     }

//     return 1;
// }

// static int wait_bus_can_rx(struct master_priv_data *priv)
// {
//     unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT;

//     while (rx_fifo_empty(priv) != 0) {
//         if (time_after(jiffies, timeout))
//             return 0;
//     }

//     return 1;
// }

static void clounix_i2c_master_dump_reg(struct master_priv_data *priv)
{
    pddf_dbg(FPGA, "FPGA_I2C_MASTER_CFG_ADDR 0x%x\r\n", readl(priv->mmio + FPGA_I2C_MASTER_CFG_ADDR));
    pddf_dbg(FPGA, "FPGA_I2C_MASTER_CTRL_ADDR 0x%x\r\n", readl(priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR));
    pddf_dbg(FPGA, "FPGA_I2C_MASTER_STATUS_ADDR 0x%x\r\n", readl(priv->mmio + FPGA_I2C_MASTER_STATUS_ADDR));
    // pddf_dbg(FPGA, "FPGA_I2C_MASTER_16BIT_ADDR 0x%x\r\n", readl(priv->mmio + FPGA_I2C_MASTER_16BIT_ADDR));
    // pddf_dbg(FPGA, "FPGA_I2C_MASTER_CHANNEL_SEL_ADDR 0x%x\r\n", readl(priv->mmio + FPGA_I2C_MASTER_CHANNEL_SEL_ADDR));
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

                pddf_dbg(FPGA, "clounix_i2c_wait_bus_tx_done data ECOMM error\r\n");

                return -ECOMM;
            }

            return 0;
        }

    } while (time_before(jiffies, timeout));

    pddf_dbg(FPGA, "clounix_i2c_wait_bus_tx_done data ETIMEDOUT error\r\n");

    return -ETIMEDOUT;
}

static int clounix_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr = 0, r_addr = 0, w_addr = 0, reg_addr = 0;
    unsigned int *tmp_addr = NULL;
    unsigned int tmp_value = 0, i = 0, j = 0;
    printk("clounix_i2c_xfer start\r\n");
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

        if (p->len == 1)
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_BYTE | ((p->buf[0] & 0xFF) << 16) | (p->len << 8) | p->buf[1]);

            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        else
        {
            tmp_addr = (unsigned int *)(priv->fpga_ram_mmio);

            for (j = 1; j <= p->len; j += 4)
            {
                tmp_value = p->buf[j];

                if ((j + 1) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 1] << 8);

                if ((j + 2) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 2] << 16);

                if ((j + 3) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 3] << 24);

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((p->buf[0] & 0xFF) << 16) | (p->len << 8));

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
            {
                goto out;
            }

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
                    {
                        goto out;
                    }
                    else
                    {
                        p->buf[0] = readb(priv->mmio + FPGA_I2C_MASTER_STATUS_ADDR);
                    }
                }
                else
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((reg_addr & 0xFF) << 16) | ((p->len) << 8));

                    writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                    if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        tmp_addr = (unsigned int *)(priv->fpga_ram_mmio);

                        for (j = 0; j < p->len; j += 4)
                        {
                            tmp_value = readl(tmp_addr);

                            p->buf[j] = (tmp_value & 0xFF);

                            if ((j + 1) >= p->len)
                            {
                                break;
                            }

                            p->buf[j + 1] = ((tmp_value >> 8) & 0xFF);

                            if ((j + 2) >= p->len)
                            {
                                break;
                            }

                            p->buf[j + 2] = ((tmp_value >> 16) & 0xFF);

                            if ((j + 3) >= p->len)
                            {
                                break;
                            }

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
#else
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

        if (p->len == 1)
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_BYTE | ((p->buf[0] & 0xFF) << 16) | (p->len << 8) | p->buf[1]);

            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        else
        {
            tmp_addr = (unsigned int *)(priv->mmio + priv->ram_base_addr);

            for (j = 1; j <= p->len; j += 4)
            {
                tmp_value = p->buf[j];

                if ((j + 1) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 1] << 8);

                if ((j + 2) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 2] << 16);

                if ((j + 3) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 3] << 24);

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((p->buf[0] & 0xFF) << 16) | (p->len << 8));

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
            {
                goto out;
            }

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
                    {
                        goto out;
                    }
                    else
                    {
                        p->buf[0] = readb(priv->mmio + FPGA_I2C_MASTER_STATUS_ADDR);
                    }
                }
                else
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((reg_addr & 0xFF) << 16) | ((p->len) << 8));

                    writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

                    if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        tmp_addr = (unsigned int *)(priv->mmio + priv->ram_base_addr);

                        for (j = 0; j < p->len; j += 4)
                        {
                            tmp_value = readl(tmp_addr);

                            p->buf[j] = (tmp_value & 0xFF);

                            if ((j + 1) >= p->len)
                            {
                                break;
                            }

                            p->buf[j + 1] = ((tmp_value >> 8) & 0xFF);

                            if ((j + 2) >= p->len)
                            {
                                break;
                            }

                            p->buf[j + 2] = ((tmp_value >> 16) & 0xFF);

                            if ((j + 3) >= p->len)
                            {
                                break;
                            }

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
#endif
#if 0
static int repeated_start_status(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_CR_REG_OFFSET) & XIIC_CR_REPEATED_START_MASK;
}

// int wait_repeated_start_done(struct master_priv_data *priv)
// {
//     unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

//     while (repeated_start_status(priv) != 0) {
//         if (time_after(jiffies, timeout))
//             return 0;
//     }
    
//     return 1;
// }

// #define DO_RX_B(priv, data) \
//     if (wait_bus_can_rx(priv) == 0) { \
//         goto out; \
//     } \
//     data = readb(priv->mmio + XIIC_DRR_REG_OFFSET)

static int clounix_i2c_smbus_xfer(struct i2c_adapter *adap, unsigned short addr, unsigned short flags, 
                       char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);

    unsigned int tmp_value = 0;

    unsigned char r_addr = 0, w_addr = 0, i = 0, data_size = 0;

    unsigned int *tmp_addr = NULL;
    printk("func:%s line:%d addr:0x%x flags:%d read_write:%d command:0x%x size:%d\n", __func__, __LINE__, 
                                        addr, flags, read_write, command, size);
    mutex_lock(&priv->lock);

    addr = (addr & 0x7f) << 1;

    w_addr = addr;

    r_addr = (addr | 0x01);

    switch (size)
    {

    case I2C_SMBUS_BYTE:

        tmp_value = 0;

        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);

        if (read_write == I2C_SMBUS_READ)
        {
#if 0
            tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

            writel((FPGA_I2C_MASTER_MGR_RD_BYTE | ((command & 0xFF) << 16) | (data & 0xFF)), priv->mmio + FPGA_I2C_MASTER_CTRL);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + FPGA_I2C_MASTER_STAT);
            }
#endif
        }
        else
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_NONE | ((command & 0xFF) << 16) | (0x01 << 8));

            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BYTE_DATA:

        tmp_value = 0;

        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_RD_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + FPGA_I2C_MASTER_STATUS_ADDR);
            }
        }
        else
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
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

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->word = readw((priv->fpga_ram_mmio));
            }
        }
        else
        {
            writew(data->word, (priv->fpga_ram_mmio));

            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, (priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR));

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BLOCK_DATA:

        tmp_value = 0;

        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CFG_ADDR);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((command & 0xFF) << 16) | ((I2C_SMBUS_BLOCK_MAX + 1) << 8));

            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);
            pddf_err(FPGA, "tmp_value %d\r\n", tmp_value);
            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                tmp_value = 0;

                tmp_value = readl(priv->fpga_ram_mmio);

                data_size = (tmp_value & 0xFF);

                pddf_err(FPGA, "I2C_SMBUS_BLOCK_DATA I2C_SMBUS_READ data_size %d\r\n", data_size);

                if (data_size > I2C_SMBUS_BLOCK_MAX)
                {
                    goto out;
                }

                tmp_addr = (unsigned int *)(priv->fpga_ram_mmio);
                pddf_err(FPGA, "priv->mmio:%p   priv->fpga_ram_mmio:%p\r\n",priv->mmio, priv->fpga_ram_mmio);

                for (i = 0; i <= data_size; i += 4)
                {
                    tmp_value = readl(tmp_addr);

                    data->block[i] = (tmp_value & 0xFF);

                    if ((i + 1) > data_size)
                    {
                        break;
                    }

                    data->block[i + 1] = ((tmp_value >> 8) & 0xFF);

                    if ((i + 2) > data_size)
                    {
                        break;
                    }

                    data->block[i + 2] = ((tmp_value >> 16) & 0xFF);

                    if ((i + 3) > data_size)
                    {
                        break;
                    }

                    data->block[i + 3] = ((tmp_value >> 24) & 0xFF);

                    tmp_addr++;
                }
                pddf_err(FPGA, "data->block[0]: %d\r\n", data->block[0]);
            }
        }
        else
        {
            data_size = data->block[0];

            tmp_addr = (unsigned int *)(priv->fpga_ram_mmio);

            for (i = 0; i <= data_size; i += 4)
            {
                tmp_value = (data->block[i] + (data->block[i + 1] << 8) + (data->block[i + 2] << 16) + (data->block[i + 3] << 24));

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((command & 0xFF) << 16) | ((data_size + 1) << 8));

            writel(tmp_value, priv->mmio + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
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
#else
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
    r_addr = addr | 0x01;

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
#endif
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
    adap->retries = DEFAULT_RETRY;    
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
