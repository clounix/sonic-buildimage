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

#include "clounix/clounix_fpga.h"
#include "device_driver_common.h"
#include "clx_driver.h"

extern void __iomem *clounix_fpga_base;

#define FPGA_I2C_DEFAULT_RETRY 3
#define FPGA_I2C_TIMEOUT (msecs_to_jiffies(250))
#define FPGA_I2C_MASTER_MGR_RST (1 << 31)
#define FPGA_I2C_MASTER_MGR_ENABLE ((1 << 30) | (1 << 29) | (0x64 << 16))
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
#define FPGA_I2C_MASTER_TX_BUFFER_ADDR (0x20)
#define FPGA_I2C_MASTER_TX_BUFFER_MAX (32)
#define FPGA_I2C_MASTER_RX_BUFFER_ADDR (0x60)
#define FPGA_I2C_MASTER_RX_BUFFER_MAX (32)

struct master_conf
{
    int iomem_offset;
    int reg_base_addr;
    int i2c_channel_sel;
    char *name;
};

static const struct master_conf priv_conf[] = {
    {0x000000, 0x1100, 0x01, "fpga-psu1"},      /*58*/
    {0x000000, 0x1100, 0x02, "fpga-adm"},       /*34*/
    {0x000000, 0x1100, 0x04, "fpga-pmbus"},     /*20*/
    {0x000000, 0x1100, 0x08, "fpga-pll"},       /*64*/
    {0x000000, 0x1100, 0x06, "fpga-tmp"},       /*48*/
    {0x000000, 0x1100, 0x07, "fpga-fan"},       /*60*/
    {0x000000, 0x1100, 0x00, "fpga-psu0"},      /*5a*/
    {0x000000, 0x1100, 0x03, "fpga-adm1"},      /*36*/
    {0x000000, 0x1100, 0x05, "fpga-pmbus1"},    /*21*/
    {0x000000, 0x1100, 0x09, "fpga-rebootrom"}, /*50*/
};

struct master_priv_data
{
    struct i2c_adapter adap;
    struct mutex lock;
    void __iomem *mmio;
    int reg_base_addr;
    int i2c_channel_sel;
};

static struct master_priv_data *group_priv;

static u32 clounix_i2c_master_func(struct i2c_adapter *a)
{
    return ((I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) & (~I2C_FUNC_SMBUS_QUICK)) | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static void clounix_i2c_master_dump_reg(struct master_priv_data *priv)
{
    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "FPGA_I2C_MASTER_CFG_ADDR 0x%x\r\n", readl(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR));
    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "FPGA_I2C_MASTER_CTRL_ADDR 0x%x\r\n", readl(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR));
    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "FPGA_I2C_MASTER_STATUS_ADDR 0x%x\r\n", readl(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_STATUS_ADDR));
    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "FPGA_I2C_MASTER_16BIT_ADDR 0x%x\r\n", readl(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_16BIT_ADDR));
    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "FPGA_I2C_MASTER_CHANNEL_SEL_ADDR 0x%x\r\n", readl(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CHANNEL_SEL_ADDR));
}

static int clounix_i2c_wait_bus_tx_done(struct master_priv_data *priv)
{
    unsigned int data;
    unsigned long timeout = jiffies + FPGA_I2C_TIMEOUT;

    do
    {
        data = readl(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_STATUS_ADDR);

        if (data & FPGA_I2C_MASTER_TX_FINISH_MASK)
        {
            if (data & FPGA_I2C_MASTER_TX_ERROR_MASK)
            {
                clounix_i2c_master_dump_reg(priv);

                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "clounix_i2c_wait_bus_tx_done data ECOMM error\r\n");

                return -ECOMM;
            }

            return 0;
        }

    } while (time_before(jiffies, timeout));

    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "clounix_i2c_wait_bus_tx_done data ETIMEDOUT error\r\n");

    return -ETIMEDOUT;
}

static int clounix_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr = 0, r_addr = 0, w_addr = 0, reg_addr = 0;
    unsigned char i2c_16bit_reg_flag = 0, reg_16bit_high_addr = 0;
    unsigned int *tmp_addr = NULL;
    unsigned int tmp_value = 0, i = 0, j = 0;

    mutex_lock(&priv->lock);

    writel(priv->i2c_channel_sel, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CHANNEL_SEL_ADDR);

    tmp_value = 0x00;

    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_16BIT_ADDR);

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
            tmp_value = readl(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            reg_addr = ((tmp_value >> 16) & 0xFF);

            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read reg_addr: 0x%x \r\n", reg_addr);

            tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

            if (p->len == 1)
            {
                tmp_value = 0;

                tmp_value = (FPGA_I2C_MASTER_MGR_RD_BYTE | (reg_addr & 0xFF) << 16 | (p->len << 8));

                writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                {
                    goto out;
                }
                else
                {
                    p->buf[0] = readb(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_STATUS_ADDR);

                    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth: %d,data : 0x%x \r\n", p->len, p->buf[0]);
                }
            }
            else
            {
                if (p->len > FPGA_I2C_MASTER_RX_BUFFER_MAX)
                {
                    goto out;
                }

                tmp_value = 0;

                tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((reg_addr & 0xFF) << 16) | ((p->len) << 8));

                writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                {
                    goto out;
                }
                else
                {
                    tmp_addr = (unsigned int *)(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_RX_BUFFER_ADDR);

                    for (j = 0; j < p->len; j += 4)
                    {
                        tmp_value = readl(tmp_addr);

                        p->buf[j] = (tmp_value & 0xFF);

                        LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth :%d,num :%d ,data:0x%x \r\n", p->len, j, p->buf[j]);

                        if ((j + 1) >= p->len)
                        {
                            break;
                        }

                        p->buf[j + 1] = ((tmp_value >> 8) & 0xFF);

                        LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth :%d,num :%d ,data:0x%x \r\n", p->len, j + 1, p->buf[j + 1]);

                        if ((j + 2) >= p->len)
                        {
                            break;
                        }

                        p->buf[j + 2] = ((tmp_value >> 16) & 0xFF);

                        LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth :%d,num :%d ,data:0x%x \r\n", p->len, j + 2, p->buf[j + 2]);

                        if ((j + 3) >= p->len)
                        {
                            break;
                        }

                        p->buf[j + 3] = ((tmp_value >> 24) & 0xFF);

                        LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth :%d,num :%d ,data:0x%x \r\n", p->len, j + 3, p->buf[j + 3]);

                        tmp_addr++;
                    }
                }
            }
        }
        else
        {
            tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

            if (p->len < 2)
            {
                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "write addr : 0x%x, len : %d, reg_addr:0x%x\r\n", addr, p->len, p->buf[0]);

                tmp_value = 0;

                tmp_value = (FPGA_I2C_MASTER_MGR_WT_NONE | ((p->buf[0] & 0xFF) << 16) | (0x01 << 8));

                writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                {
                    goto out;
                }
            }
            else if (p->len == 2)
            {
                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "write addr : 0x%x, len : %d, reg_addr:0x%x, data:0x%x\r\n", addr, p->len, p->buf[0], p->buf[1]);

                tmp_value = 0;

                tmp_value = (FPGA_I2C_MASTER_MGR_WT_BYTE | ((p->buf[0] & 0xFF) << 16) | ((p->len - 1) << 8) | p->buf[1]);

                writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                {
                    goto out;
                }
            }
            else
            {
                if (p->len > FPGA_I2C_MASTER_TX_BUFFER_MAX)
                {
                    goto out;
                }

                tmp_addr = (unsigned int *)(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_TX_BUFFER_ADDR);

                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "write addr : 0x%x, len : %d, reg_addr:0x%x, data:0x%x\r\n", addr, p->len, p->buf[0], p->buf[1]);

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

                tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((p->buf[0] & 0xFF) << 16) | ((p->len - 1) << 8));

                writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

                if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                {
                    goto out;
                }
            }
        }
    }

    mutex_unlock(&priv->lock);

    usleep_range(50, 100);

    return num;

out:

    tmp_value = 0;

    tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE);

    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

    mutex_unlock(&priv->lock);

    return -ETIMEDOUT;
}

#if 0
static int clounix_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr = 0, r_addr = 0, w_addr = 0, reg_addr = 0;
    unsigned char i2c_16bit_reg_flag = 0, reg_16bit_high_addr = 0;
    unsigned int *tmp_addr = NULL;
    unsigned int tmp_value = 0, i = 0, j = 0;

    mutex_lock(&priv->lock);

    writel(priv->i2c_channel_sel, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CHANNEL_SEL_ADDR);

    tmp_value = 0x00;

    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_16BIT_ADDR);

    if (num == 1) // write
    {
        p = &msgs[0];

        if (p->flags & I2C_M_TEN)
        {
            goto out;
        }

        addr = i2c_8bit_addr_from_msg(p);

        w_addr = (addr & (~(0x01)));

        r_addr = (addr | 0x01);

        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

        if (p->len < 2)
        {
            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "write addr : 0x%x, len : %d, reg_addr:0x%x\r\n", addr, p->len, p->buf[0]);

            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_NONE | ((p->buf[0] & 0xFF) << 16) | (0x01 << 8));

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        else if (p->len == 2)
        {
            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "write addr : 0x%x, len : %d, reg_addr:0x%x, data:0x%x\r\n", addr, p->len, p->buf[0], p->buf[1]);

            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_BYTE | ((p->buf[0] & 0xFF) << 16) | ((p->len - 1) << 8) | p->buf[1]);

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        else
        {
            if (p->len > FPGA_I2C_MASTER_TX_BUFFER_MAX)
            {
                goto out;
            }

            tmp_addr = (unsigned int *)(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_TX_BUFFER_ADDR);

            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "write addr : 0x%x, len : %d, reg_addr:0x%x, data:0x%x\r\n", addr, p->len, p->buf[0], p->buf[1]);

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

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((p->buf[0] & 0xFF) << 16) | ((p->len - 1) << 8));

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

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
                tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

                writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

                if (i2c_16bit_reg_flag == 1)
                {
                    tmp_value = 0x00;

                    tmp_value = ((1 << 31) | reg_16bit_high_addr);

                    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_16BIT_ADDR);
                }
                else
                {
                    tmp_value = 0x00;

                    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_16BIT_ADDR);
                }

                if (p->len == 1)
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_I2C_MASTER_MGR_RD_BYTE | (reg_addr & 0xFF) << 16 | (p->len << 8));

                    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

                    if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        p->buf[0] = readb(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_STATUS_ADDR);

                        LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth: %d,data : 0x%x \r\n", p->len, p->buf[0]);
                    }
                }
                else
                {
                    if (p->len > FPGA_I2C_MASTER_RX_BUFFER_MAX)
                    {
                        goto out;
                    }

                    tmp_value = 0;

                    tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((reg_addr & 0xFF) << 16) | ((p->len) << 8));

                    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

                    if (clounix_i2c_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        tmp_addr = (unsigned int *)(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_RX_BUFFER_ADDR);

                        for (j = 0; j < p->len; j += 4)
                        {
                            tmp_value = readl(tmp_addr);

                            p->buf[j] = (tmp_value & 0xFF);

                            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth :%d,num :%d ,data:0x%x \r\n", p->len, j, p->buf[j]);

                            if ((j + 1) >= p->len)
                            {
                                break;
                            }

                            p->buf[j + 1] = ((tmp_value >> 8) & 0xFF);

                            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth :%d,num :%d ,data:0x%x \r\n", p->len, j + 1, p->buf[j + 1]);

                            if ((j + 2) >= p->len)
                            {
                                break;
                            }

                            p->buf[j + 2] = ((tmp_value >> 16) & 0xFF);

                            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth :%d,num :%d ,data:0x%x \r\n", p->len, j + 2, p->buf[j + 2]);

                            if ((j + 3) >= p->len)
                            {
                                break;
                            }

                            p->buf[j + 3] = ((tmp_value >> 24) & 0xFF);

                            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read lenth :%d,num :%d ,data:0x%x \r\n", p->len, j + 3, p->buf[j + 3]);

                            tmp_addr++;
                        }
                    }
                }
            }
            else
            {
                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read reg lenth :%d\r\n", p->len);

                if (p->len == 1)
                {
                    reg_addr = p->buf[0];

                    i2c_16bit_reg_flag = 0;

                    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read reg lenth :%d,reg_addr:0x%x \r\n", p->len, reg_addr);
                }
                if (p->len == 2)
                {
                    reg_16bit_high_addr = p->buf[0];

                    reg_addr = p->buf[1];

                    i2c_16bit_reg_flag = 1;

                    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "read 16_bit_reg lenth :%d,reg_addr1:0x%x ,reg_addr1:0x%x\r\n", p->len, reg_addr, reg_16bit_high_addr);
                }
            }
        }
    }

    mutex_unlock(&priv->lock);

    usleep_range(50, 100);

    return num;

out:
    tmp_value = 0;

    tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE);

    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);
    
    mutex_unlock(&priv->lock);

    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "go out ETIMEDOUT ETIMEDOUT ETIMEDOUTE TIMEDOUT\r\n");

    return -ETIMEDOUT;
}
#endif

static int clounix_i2c_master_smbus_xfer(struct i2c_adapter *adap, unsigned short addr, unsigned short flags,
                                         char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);

    unsigned int tmp_value = 0;

    unsigned char r_addr = 0, w_addr = 0, i = 0, data_size = 0;

    unsigned int *tmp_addr = NULL;

    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "clounix_i2c_master_smbus_xfer addr : %x size: %x command %x rw %x\r\n", addr, size, command, read_write);

    mutex_lock(&priv->lock);

    writel(priv->i2c_channel_sel, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CHANNEL_SEL_ADDR);

    tmp_value = 0x00;

    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_16BIT_ADDR);

    w_addr = (addr & 0x7f) << 1;

    r_addr = (((addr & 0x7f) << 1) | 0x01);

    switch (size)
    {

    case I2C_SMBUS_BYTE:

        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

        if (read_write == I2C_SMBUS_READ)
        {
#if 0
            tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

            writel((FPGA_I2C_MASTER_MGR_RD_BYTE | ((command & 0xFF) << 16) | (data & 0xFF)), priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_STATUS_ADDR);
            }
#endif
        }
        else
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_NONE | ((command & 0xFF) << 16) | (0x01 << 8));

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BYTE_DATA:

        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = (FPGA_I2C_MASTER_MGR_RD_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_STATUS_ADDR);
            }
        }
        else
        {
            tmp_value = (FPGA_I2C_MASTER_MGR_WT_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_WORD_DATA:

        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->word = readw((priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_RX_BUFFER_ADDR));
            }
        }
        else
        {
            writew(data->word, (priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_TX_BUFFER_ADDR));

            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, (priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR));

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BLOCK_DATA:

        tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_RD_WORD | ((command & 0xFF) << 16) | ((I2C_SMBUS_BLOCK_MAX) << 8));

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

            if (clounix_i2c_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                tmp_value = readl(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_RX_BUFFER_ADDR);

                data_size = (tmp_value & 0xFF);

                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "I2C_SMBUS_BLOCK_DATA I2C_SMBUS_READ data_size %d\r\n", data_size);

                if (data_size >= I2C_SMBUS_BLOCK_MAX)
                {
                    goto out;
                }

                tmp_addr = (unsigned int *)(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_RX_BUFFER_ADDR);

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
            }
        }
        else
        {
            data_size = data->block[0];

            tmp_addr = (unsigned int *)(priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_TX_BUFFER_ADDR);

            for (i = 0; i <= data_size; i += 4)
            {
                tmp_value = (data->block[i] + (data->block[i + 1] << 8) + (data->block[i + 2] << 16) + (data->block[i + 3] << 24));

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_I2C_MASTER_MGR_WT_WORD | ((command & 0xFF) << 16) | ((data_size + 1) << 8));

            writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CTRL_ADDR);

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
    usleep_range(50, 100);
    return 0;

out:
    tmp_value = 0;
    tmp_value = (FPGA_I2C_MASTER_MGR_RST | FPGA_I2C_MASTER_MGR_ENABLE);
    writel(tmp_value, priv->mmio + priv->reg_base_addr + FPGA_I2C_MASTER_CFG_ADDR);
    mutex_unlock(&priv->lock);
    return -ETIMEDOUT;
}

static struct i2c_algorithm clounix_i2c_master_algo = {
    .smbus_xfer = clounix_i2c_master_smbus_xfer,
    .master_xfer = clounix_i2c_master_xfer,
    .functionality = clounix_i2c_master_func,
};

int drv_i2c_master_xilinx_init(void **driver)
{
    struct pci_dev *pdev = pci_get_device(0x10ee, 0x7021, NULL);
    struct master_priv_data *priv;
    void __iomem *base;
    struct i2c_adapter *adap;
    int total_adap;
    int i, err;

    if (pdev == NULL)
    {
        LOG_ERR(CLX_DRIVER_TYPES_I2C_MASTER, "dev not found\r\n");
        return -ENXIO;
    }

    if (clounix_fpga_base == NULL)
    {
        LOG_ERR(CLX_DRIVER_TYPES_I2C_MASTER, "resource not iomap\r\n");
        return -ENXIO;
    }

    base = pci_get_drvdata(pdev);

    total_adap = sizeof(priv_conf) / sizeof(struct master_conf);

    group_priv = kzalloc(sizeof(struct master_priv_data) * total_adap, GFP_KERNEL);
    if (group_priv == NULL)
    {
        LOG_ERR(CLX_DRIVER_TYPES_I2C_MASTER, "no memory\r\n");
        return -ENOMEM;
    }

    priv = group_priv;

    for (i = 0; i < total_adap; i++)
    {
        adap = &priv[i].adap;
        adap->owner = THIS_MODULE;
        adap->algo = &clounix_i2c_master_algo;
        adap->retries = FPGA_I2C_DEFAULT_RETRY;
        adap->dev.parent = &pdev->dev;
        adap->dev.of_node = pdev->dev.of_node;
        strlcpy(adap->name, priv_conf[i].name, sizeof(adap->name));
        i2c_set_adapdata(adap, &priv[i]);
        priv[i].mmio = base + priv_conf[i].iomem_offset;
        priv[i].reg_base_addr = priv_conf[i].reg_base_addr;
        priv[i].i2c_channel_sel = priv_conf[i].i2c_channel_sel;
        mutex_init(&(priv[i].lock));

        err = i2c_add_adapter(adap);
        if (err)
            goto err_i2c_group;
    }

    return 0;

err_i2c_group:
    while (--i >= 0)
    {
        i2c_del_adapter(adap);
    }

    kfree(group_priv);

    return err;
}

void drv_i2c_master_xilinx_exit(void **driver)
{
    int i, total_adap;
    struct master_priv_data *priv = group_priv;

    total_adap = sizeof(priv_conf) / sizeof(struct master_conf);

    for (i = 0; i < total_adap; i++)
    {
        i2c_del_adapter(&(priv[i].adap));
    }

    kfree(group_priv);

    return;
}
