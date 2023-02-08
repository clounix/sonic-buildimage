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

#define DEFAULT_RETRY 3

#define XIIC_MSB_OFFSET (0)
#define XIIC_REG_OFFSET (0x100+XIIC_MSB_OFFSET)
/*
 * Register offsets in bytes from RegisterBase. Three is added to the
 * base offset to access LSB (IBM style) of the word
 */
#define XIIC_CR_REG_OFFSET   (0x00+XIIC_REG_OFFSET) /* Control Register   */
#define XIIC_SR_REG_OFFSET   (0x04+XIIC_REG_OFFSET) /* Status Register    */
#define XIIC_DTR_REG_OFFSET  (0x08+XIIC_REG_OFFSET) /* Data Tx Register   */
#define XIIC_DRR_REG_OFFSET  (0x0C+XIIC_REG_OFFSET) /* Data Rx Register   */
#define XIIC_ADR_REG_OFFSET  (0x10+XIIC_REG_OFFSET) /* Address Register   */
#define XIIC_TFO_REG_OFFSET  (0x14+XIIC_REG_OFFSET) /* Tx FIFO Occupancy  */
#define XIIC_RFO_REG_OFFSET  (0x18+XIIC_REG_OFFSET) /* Rx FIFO Occupancy  */
#define XIIC_TBA_REG_OFFSET  (0x1C+XIIC_REG_OFFSET) /* 10 Bit Address reg */
#define XIIC_RFD_REG_OFFSET  (0x20+XIIC_REG_OFFSET) /* Rx FIFO Depth reg  */
#define XIIC_GPO_REG_OFFSET  (0x24+XIIC_REG_OFFSET) /* Output Register    */

/* Control Register masks */
#define XIIC_CR_ENABLE_DEVICE_MASK        0x01  /* Device enable = 1      */
#define XIIC_CR_TX_FIFO_RESET_MASK        0x02  /* Transmit FIFO reset=1  */
#define XIIC_CR_MSMS_MASK                 0x04  /* Master starts Txing=1  */
#define XIIC_CR_DIR_IS_TX_MASK            0x08  /* Dir of tx. Txing=1     */
#define XIIC_CR_NO_ACK_MASK               0x10  /* Tx Ack. NO ack = 1     */
#define XIIC_CR_REPEATED_START_MASK       0x20  /* Repeated start = 1     */
#define XIIC_CR_GENERAL_CALL_MASK         0x40  /* Gen Call enabled = 1   */

/* Status Register masks */
#define XIIC_SR_GEN_CALL_MASK             0x01  /* 1=a mstr issued a GC   */
#define XIIC_SR_ADDR_AS_SLAVE_MASK        0x02  /* 1=when addr as slave   */
#define XIIC_SR_BUS_BUSY_MASK             0x04  /* 1 = bus is busy        */
#define XIIC_SR_MSTR_RDING_SLAVE_MASK     0x08  /* 1=Dir: mstr <-- slave  */
#define XIIC_SR_TX_FIFO_FULL_MASK         0x10  /* 1 = Tx FIFO full       */
#define XIIC_SR_RX_FIFO_FULL_MASK         0x20  /* 1 = Rx FIFO full       */
#define XIIC_SR_RX_FIFO_EMPTY_MASK        0x40  /* 1 = Rx FIFO empty      */
#define XIIC_SR_TX_FIFO_EMPTY_MASK        0x80  /* 1 = Tx FIFO empty      */

/* Interrupt Status Register masks    Interrupt occurs when...       */
#define XIIC_INTR_ARB_LOST_MASK           0x01  /* 1 = arbitration lost   */
#define XIIC_INTR_TX_ERROR_MASK           0x02  /* 1=Tx error/msg complete */
#define XIIC_INTR_TX_EMPTY_MASK           0x04  /* 1 = Tx FIFO/reg empty  */
#define XIIC_INTR_RX_FULL_MASK            0x08  /* 1=Rx FIFO/reg=OCY level */
#define XIIC_INTR_BNB_MASK                0x10  /* 1 = Bus not busy       */
#define XIIC_INTR_AAS_MASK                0x20  /* 1 = when addr as slave */
#define XIIC_INTR_NAAS_MASK               0x40  /* 1 = not addr as slave  */
#define XIIC_INTR_TX_HALF_MASK            0x80  /* 1 = TX FIFO half empty */

/* The following constants specify the depth of the FIFOs */
#define IIC_RX_FIFO_DEPTH         16    /* Rx fifo capacity               */
#define IIC_TX_FIFO_DEPTH         16    /* Tx fifo capacity               */

#define XIIC_DGIER_OFFSET    (XIIC_MSB_OFFSET+0x1C) /* Device Global Interrupt Enable Register */
#define XIIC_IISR_OFFSET     (XIIC_MSB_OFFSET+0x20) /* Interrupt Status Register */
#define XIIC_IIER_OFFSET     (XIIC_MSB_OFFSET+0x28) /* Interrupt Enable Register */
#define XIIC_RESETR_OFFSET   (XIIC_MSB_OFFSET+0x40) /* Reset Register */

#define XIIC_RESET_MASK             0xAUL

#define XIIC_PM_TIMEOUT     1000    /* ms */
/* timeout waiting for the controller to respond */
#define XIIC_I2C_TIMEOUT    (msecs_to_jiffies(500))

#define XIIC_TX_DYN_START_MASK            0x0100 /* 1 = Set dynamic start */
#define XIIC_TX_DYN_STOP_MASK             0x0200 /* 1 = Set dynamic stop */

#define XIIC_GINTR_ENABLE_MASK      0x80000000UL

#define CLOUNIX_INIT_TIMEOUT (msecs_to_jiffies(100))

#define FPGA_PSU_MGR_RST (1 << 31)
#define FPGA_PSU_MGR_ENABLE ((1 << 30) | (0x64 << 16))
#define FPGA_PSU_TX_FINISH_MASK (0x80000000UL)
#define FPGA_PSU_TX_ERROR_MASK (0x40000000UL)

#define FPGA_PSU_MGR_RD_BYTE (0x81 << 24)
#define FPGA_PSU_MGR_WT_BYTE (0x84 << 24)
#define FPGA_PSU_MGR_RD_WORD (0x82 << 24)
#define FPGA_PSU_MGR_WT_WORD (0x88 << 24)
#define FPGA_PSU_MGR_WT_NONE (0x85 << 24)

#define FPGA_PSU_BASE (0x1100)
#define FPGA_PSU_CFG (FPGA_PSU_BASE + 0xe0)
#define FPGA_PSU_CTRL (FPGA_PSU_BASE + 0xe4)
#define FPGA_PSU_STAT (FPGA_PSU_BASE + 0xe8)
#define FPGA_PSU_DBG (FPGA_PSU_BASE + 0xec)

#define FPGA_PSU1_RAM_ADDR_OFFSET (0x1d0000)

#define FPGA_PSU0_BASE (0x1200)
#define FPGA_PSU0_CFG (FPGA_PSU0_BASE + 0xe0)
#define FPGA_PSU0_CTRL (FPGA_PSU0_BASE + 0xe4)
#define FPGA_PSU0_STAT (FPGA_PSU0_BASE + 0xe8)
#define FPGA_PSU0_DBG (FPGA_PSU0_BASE + 0xec)

#define FPGA_PSU0_RAM_ADDR_OFFSET (0x1e0000)

#define FPGA_EEPROM_MGR_RST (1 << 31)
#define FPGA_EEPROM_MGR_ENABLE ((1 << 30) | (1 << 29) | (0x64 << 16))
#define FPGA_EEPROM_TX_FINISH_MASK (0x80000000UL)
#define FPGA_EEPROM_TX_ERROR_MASK (0x40000000UL)

#define FPGA_EEPROM_MGR_RD_BYTE (0x81 << 24)
#define FPGA_EEPROM_MGR_WT_BYTE (0x84 << 24)
#define FPGA_EEPROM_MGR_RD_WORD (0x82 << 24)
#define FPGA_EEPROM_MGR_WT_WORD (0x88 << 24)
#define FPGA_EEPROM_MGR_WT_NONE (0x85 << 24)

#define FPGA_EEPROM_BASE (0x2000)
#define FPGA_EEPROM_CFG (FPGA_EEPROM_BASE + 0x00)
#define FPGA_EEPROM_CTRL (FPGA_EEPROM_BASE + 0x04)
#define FPGA_EEPROM_STAT (FPGA_EEPROM_BASE + 0x08)
#define FPGA_EEPROM_DBG (FPGA_EEPROM_BASE + 0x0c)

#define FPGA_EEPROM_RAM_ADDR_OFFSET (0x1f0000)

struct master_conf
{
    int offset;
    char *name;
};

static const struct master_conf priv_conf[] = {
    {0x000000, "fpga-psu1"}, /*58*/
    {0x210000, "fpga-adm"},
    {0x220000, "fpga-pmbus"},
    {0x230000, "fpga-pll"},
    {0x240000, "fpga-tmp"},
    {0x250000, "fpga-fan"},
    {0x000000, "fpga-psu0"}, /*5a*/
    {0x270000, "fpga-adm1"},
    {0x280000, "fpga-pmbus1"},
    {0x000000, "fpga-rebootrom"},
};

struct master_priv_data {
    struct i2c_adapter adap;
    struct mutex lock;
    void __iomem *mmio;
};

static struct master_priv_data *group_priv;

static int tx_fifo_full(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_TX_FIFO_FULL_MASK;
}

static int tx_fifo_empty(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_TX_FIFO_EMPTY_MASK;
}

static int rx_fifo_full(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_RX_FIFO_FULL_MASK;
}

static int rx_fifo_empty(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_RX_FIFO_EMPTY_MASK;
}

static int tx_fifo_space(struct master_priv_data *priv)
{
    return IIC_TX_FIFO_DEPTH - readb(priv->mmio + XIIC_TFO_REG_OFFSET) - 1;
}

static int bus_busy(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK;
}

static void dump_reg(struct master_priv_data *priv)
{
    int i;

    LOG_INFO(CLX_DRIVER_TYPES_I2C_MASTER, "%x\r\n", readl(priv->mmio + 0x1c));
    LOG_INFO(CLX_DRIVER_TYPES_I2C_MASTER, "%x\r\n", readl(priv->mmio + 0x20));
    LOG_INFO(CLX_DRIVER_TYPES_I2C_MASTER, "%x\r\n", readl(priv->mmio + 0x28));
    
    i = XIIC_CR_REG_OFFSET;
    while (i<=XIIC_RFD_REG_OFFSET) {
        LOG_INFO(CLX_DRIVER_TYPES_I2C_MASTER, "off %x: %x\r\n", i, readl(priv->mmio + i));
        i+=4;
    }
}

static int fpga_i2c_reinit(struct master_priv_data *priv, unsigned long after)
{
    unsigned long timeout;
    
    dump_reg(priv);
    writeb(0, priv->mmio + XIIC_CR_REG_OFFSET);
    writeb(XIIC_RESET_MASK, priv->mmio + XIIC_RESETR_OFFSET);
    timeout = jiffies +  after;
    while(time_after(jiffies, timeout)) {};

    writeb(IIC_RX_FIFO_DEPTH - 1, priv->mmio + XIIC_RFD_REG_OFFSET);
    
    //writeb(XIIC_CR_ENABLE_DEVICE_MASK , priv->mmio + XIIC_CR_REG_OFFSET);
    //writeb(XIIC_CR_ENABLE_DEVICE_MASK | XIIC_CR_TX_FIFO_RESET_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
    writeb(XIIC_CR_TX_FIFO_RESET_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
    timeout = jiffies +  after;
    while(time_after(jiffies, timeout)) {};
    writeb(0, priv->mmio + XIIC_CR_REG_OFFSET);

    timeout = jiffies + after;
    while (rx_fifo_empty(priv) == 0) {
        if (time_after(jiffies, timeout)) {
            LOG_ERR(CLX_DRIVER_TYPES_I2C_MASTER, "%s reinit timeout\r\n", priv->adap.name);
            return -ETIMEDOUT;
        }
    }
    
    dump_reg(priv);
    return 0;
}

static u32 clounix_i2c_func(struct i2c_adapter *a)
{
    return ((I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) & (~I2C_FUNC_SMBUS_QUICK)) | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static int wait_bus_busy_status(struct master_priv_data *priv, unsigned int status)
{
    unsigned long timeout;
    
    timeout = jiffies + XIIC_I2C_TIMEOUT; 
    while (bus_busy(priv) != status) {
        if (time_after(jiffies, timeout)) {
            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "bus status err %x\r\n", status);
            return 0;
        }
    }

    return 1;
}

static int wait_bus_tx_done(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

    while (tx_fifo_empty(priv) == 0) {
        if (time_after(jiffies, timeout)) {
            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "tx fifo not empty\r\n");
            return 0;
        }
    }

    return 1;
}

static int wait_bus_can_tx(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

    while (tx_fifo_full(priv) != 0) {
        if (time_after(jiffies, timeout)) {
            LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER,  "tx fifo full\r\n");
            return 0;
        }
    }

    return 1;
}

static int wait_bus_can_rx(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT;

    while (rx_fifo_empty(priv) != 0) {
        if (time_after(jiffies, timeout))
            return 0;
    }

    return 1;
}

static int clounix_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr;
    int i, j;
    unsigned long timeout;

    p = &msgs[0];
    if((0x50 <= p->addr) && (p->addr <= 0x57) && ((p->flags & I2C_M_RD)==0))
    {
        usleep_range(6000, 10000);
    }

    mutex_lock(&priv->lock);
    writeb(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
    if (wait_bus_busy_status(priv, 0) == 0)
       goto out;  
    if (wait_bus_tx_done(priv) == 0)
        goto out;

    for (i=0; i<num; i++) {
        p = &msgs[i];
        if (p->flags & I2C_M_TEN)
            goto out;
        addr = i2c_8bit_addr_from_msg(p);
        
        writew(addr | XIIC_TX_DYN_START_MASK, priv->mmio + XIIC_DTR_REG_OFFSET);
        if (p->flags & I2C_M_RD) {
            writew((p->len | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
        }
        j = 0;
        if (p->flags & I2C_M_RD) {
            if (wait_bus_tx_done(priv) == 0)
                goto out;
            timeout = jiffies + XIIC_I2C_TIMEOUT;
            while (j < p->len) {
                if (rx_fifo_empty(priv) != 0) {
                    if (time_after(jiffies, timeout)) {
                        dump_reg(priv);
                        LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER,  "rx timeout\r\n");
                        goto out;
                    }
                    continue;
                }
                timeout = jiffies + XIIC_I2C_TIMEOUT;
                p->buf[j] = readb(priv->mmio + XIIC_DRR_REG_OFFSET);
                j++;
            }
        } else {
            timeout = jiffies + XIIC_I2C_TIMEOUT; 
            while (j < p->len) {
                if (tx_fifo_full(priv) != 0) {
                    if (time_after(jiffies, timeout)) {
                        LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER,  "tx fifo full\r\n");
                        goto out;
                    }

                    continue;
                }
               
                timeout = jiffies + XIIC_I2C_TIMEOUT; 
                if ((j == p->len - 1)&&(i == num-1)) {
                    writew(p->buf[j] | XIIC_TX_DYN_STOP_MASK, priv->mmio + XIIC_DTR_REG_OFFSET);
                } else {
                    writew(p->buf[j], priv->mmio + XIIC_DTR_REG_OFFSET);
                }

                j++;
            }
        
        }
    }
    if (wait_bus_busy_status(priv, 0) == 0)
        goto out;

    writeb((XIIC_CR_TX_FIFO_RESET_MASK | XIIC_CR_ENABLE_DEVICE_MASK), priv->mmio + XIIC_CR_REG_OFFSET);
    writeb(0, priv->mmio + XIIC_CR_REG_OFFSET);
    mutex_unlock(&priv->lock);

    return num;

out:
    fpga_i2c_reinit(priv, CLOUNIX_INIT_TIMEOUT);
    mutex_unlock(&priv->lock);
    
    return -ETIMEDOUT;
}

static int repeated_start_status(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_CR_REG_OFFSET) & XIIC_CR_REPEATED_START_MASK;
}

int wait_repeated_start_done(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

    while (repeated_start_status(priv) != 0) {
        if (time_after(jiffies, timeout))
            return 0;
    }
    
    return 1;
}

#define DO_RX_B(priv, data) \
    if (wait_bus_can_rx(priv) == 0) { \
        dump_reg(priv); \
        goto out; \
    } \
    data = readb(priv->mmio + XIIC_DRR_REG_OFFSET)

static int clounix_i2c_smbus_xfer(struct i2c_adapter *adap, unsigned short addr, unsigned short flags, 
                       char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    unsigned char tmp;
    unsigned char i;
    
    if (wait_bus_busy_status(priv, 0) == 0)
        return -EBUSY;

    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER,  "size: %x command %x rw %x\r\n", size, command, read_write);

    mutex_lock(&priv->lock); 
    switch (size) {
        case I2C_SMBUS_BYTE:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | read_write), priv->mmio + XIIC_DTR_REG_OFFSET);
            if (read_write == I2C_SMBUS_READ) {
                writew((0x1 | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);

                if (wait_bus_tx_done(priv) == 0)
                    goto out;
                
                DO_RX_B(priv, data->byte);
            } else {
                writew((command | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            }
            break;

        case I2C_SMBUS_BYTE_DATA:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_WRITE), priv->mmio + XIIC_DTR_REG_OFFSET);
            writew((command), priv->mmio + XIIC_DTR_REG_OFFSET);
            if (read_write == I2C_SMBUS_READ) {
                writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_READ), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew((0x1 | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
                
                if (wait_bus_tx_done(priv) == 0)
                    goto out;
                
                DO_RX_B(priv, data->byte);
            } else {
                writew((data->byte | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            }
            break;

        case I2C_SMBUS_WORD_DATA:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_WRITE), priv->mmio + XIIC_DTR_REG_OFFSET);
            writew((command), priv->mmio + XIIC_DTR_REG_OFFSET);
            if (read_write == I2C_SMBUS_READ) {
                writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_READ), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew((0x2 | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
                if (wait_bus_tx_done(priv) == 0)
                    goto out;

                DO_RX_B(priv, data->word);
                
                DO_RX_B(priv, tmp);
                data->word += tmp*256;
            } else {
                writew((data->word & 0xff), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew((((data->word & 0xff00) >> 8) | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            }
            break;

        case I2C_SMBUS_BLOCK_DATA:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_WRITE), priv->mmio + XIIC_DTR_REG_OFFSET);
            writew(command, priv->mmio + XIIC_DTR_REG_OFFSET);
            
            if (read_write == I2C_SMBUS_READ) {
                writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_READ), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(((I2C_SMBUS_BLOCK_MAX+1) | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            
                if (wait_bus_tx_done(priv) == 0)
                    goto out;

                DO_RX_B(priv, data->block[0]);
                if (data->block[0] > I2C_SMBUS_BLOCK_MAX)
                    goto out;

                for (i=1; i<=data->block[0]; i++) {
                    DO_RX_B(priv, data->block[i]);
                }
                
                if (data->block[0] < I2C_SMBUS_BLOCK_MAX) {
                    for (i=i; i<=I2C_SMBUS_BLOCK_MAX; i++) {
                        DO_RX_B(priv, tmp);
                    }
                }
            } else {
                writew(data->block[0], priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
                for (i=1; i<=data->block[0]; i++) {
                    if (wait_bus_can_tx(priv) == 0)
                        goto out;
                   
                    if (i == data->block[0])
                        writew(data->block[i] | XIIC_TX_DYN_STOP_MASK, priv->mmio + XIIC_DTR_REG_OFFSET);
                    else
                        writew(data->block[i], priv->mmio + XIIC_DTR_REG_OFFSET);
                }
            }
            break;

        default:
            break;
    }

    if (wait_bus_busy_status(priv, 0) == 0)
        goto out; 

    writeb((XIIC_CR_TX_FIFO_RESET_MASK | XIIC_CR_ENABLE_DEVICE_MASK), priv->mmio + XIIC_CR_REG_OFFSET);
    writew(0, priv->mmio + XIIC_CR_REG_OFFSET);
    mutex_unlock(&priv->lock);
    return 0;

out:
    fpga_i2c_reinit(priv, CLOUNIX_INIT_TIMEOUT);
    mutex_unlock(&priv->lock);
    return -ETIMEDOUT;
}

static int psu_wait_bus_tx_done(struct master_priv_data *priv)
{
    unsigned int data;
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT;

    do
    {
        data = readl(priv->mmio + FPGA_PSU_STAT);

        if (data & FPGA_PSU_TX_FINISH_MASK)
        {
            if (data & FPGA_PSU_TX_ERROR_MASK)
            {
                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "psu_wait_bus_tx_done data ECOMM error\r\n");

                return -ECOMM;
            }

            return 0;
        }

    } while (time_before(jiffies, timeout));

    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "psu_wait_bus_tx_done data ETIMEDOUT error\r\n");

    return -ETIMEDOUT;
}

static int clounix_i2c_smbus_xfer_psu1(struct i2c_adapter *adap, unsigned short addr, unsigned short flags,
                                       char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);

    unsigned int tmp_value = 0;

    unsigned char r_addr = 0, w_addr = 0, i = 0, data_size = 0;

    unsigned int *tmp_addr = NULL;

    // LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "clounix_i2c_smbus_xfer_psu1 addr : %x size: %x command %x rw %x\r\n", addr, size, command, read_write);

    mutex_lock(&priv->lock);

    addr = (addr & 0x7f) << 1;

    w_addr = addr;

    r_addr = (addr | 0x01);

    switch (size)
    {

    case I2C_SMBUS_BYTE:

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
#if 0
            tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

            writel((FPGA_PSU_MGR_RD_BYTE | ((command & 0xFF) << 16) | (data & 0xFF)), priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + FPGA_PSU_STAT);
            }
#endif
        }
        else
        {
            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_WT_NONE | ((command & 0xFF) << 16) | (0x01 << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BYTE_DATA:

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = (FPGA_PSU_MGR_RD_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + FPGA_PSU_STAT);
            }
        }
        else
        {
            tmp_value = (FPGA_PSU_MGR_WT_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_WORD_DATA:

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_RD_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->word = readw((priv->mmio + FPGA_PSU1_RAM_ADDR_OFFSET));
            }
        }
        else
        {
            writew(data->word, (priv->mmio + FPGA_PSU1_RAM_ADDR_OFFSET));

            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_WT_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, (priv->mmio + FPGA_PSU_CTRL));

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BLOCK_DATA:

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_RD_WORD | ((command & 0xFF) << 16) | ((I2C_SMBUS_BLOCK_MAX + 1) << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                tmp_value = readl(priv->mmio + FPGA_PSU1_RAM_ADDR_OFFSET);

                data_size = (tmp_value & 0xFF);

                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "I2C_SMBUS_BLOCK_DATA I2C_SMBUS_READ data_size %d\r\n", data_size);

                if (data_size > I2C_SMBUS_BLOCK_MAX)
                {
                    goto out;
                }

                tmp_addr = (unsigned int *)(priv->mmio + FPGA_PSU1_RAM_ADDR_OFFSET);

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

            tmp_addr = (unsigned int *)(priv->mmio + FPGA_PSU1_RAM_ADDR_OFFSET);

            for (i = 0; i <= data_size; i += 4)
            {
                tmp_value = (data->block[i] + (data->block[i + 1] << 8) + (data->block[i + 2] << 16) + (data->block[i + 3] << 24));

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_WT_WORD | ((command & 0xFF) << 16) | ((data_size + 1) << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        break;

    default:
        break;
    }

    mutex_unlock(&priv->lock);
    usleep_range(6000, 10000);
    return 0;

out:
    mutex_unlock(&priv->lock);
    return -ETIMEDOUT;
}

static int clounix_i2c_xfer_psu1(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr = 0, r_addr = 0, w_addr = 0, reg_addr = 0;
    unsigned int *tmp_addr = NULL;
    unsigned int tmp_value = 0, i = 0, j = 0;

    mutex_lock(&priv->lock);

    if (num == 1)//write
    {
        p = &msgs[0];

        if (p->flags & I2C_M_TEN)
            goto out;

        addr = i2c_8bit_addr_from_msg(p);

        w_addr = (addr & (~(0x01)));

        r_addr = (addr | 0x01);

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU_CFG);

        if (p->len == 1)
        {
            tmp_value = (FPGA_PSU_MGR_WT_BYTE | ((p->buf[0] & 0xFF) << 16) | (p->len << 8) | p->buf[1]);

            writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        else
        {
            tmp_addr = (unsigned int *)(priv->mmio + FPGA_PSU1_RAM_ADDR_OFFSET);

            for (j = 1; j <= p->len; j += 4)
            {
                tmp_value = p->buf[j];

                if((j + 1) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 1] << 8);

                if((j + 2) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 2] << 16);

                if((j + 3) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 3] << 24);

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_WT_WORD | ((p->buf[0] & 0xFF) << 16) | (p->len << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
    }
    else//read
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

                tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

                writel(tmp_value, priv->mmio + FPGA_PSU_CFG);

                if (p->len == 1)
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_PSU_MGR_RD_BYTE | (reg_addr & 0xFF) << 16 | (p->len << 8));

                    writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

                    if (psu_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        p->buf[0] = readb(priv->mmio + FPGA_PSU_STAT);
                    }
                }
                else
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_PSU_MGR_RD_WORD | ((reg_addr & 0xFF) << 16) | ((p->len) << 8));

                    writel(tmp_value, priv->mmio + FPGA_PSU_CTRL);

                    if (psu_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        tmp_addr = (unsigned int *)(priv->mmio + FPGA_PSU1_RAM_ADDR_OFFSET);

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
    mutex_unlock(&priv->lock);

    return -ETIMEDOUT;
}

static int psu0_wait_bus_tx_done(struct master_priv_data *priv)
{
    unsigned int data;
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT;

    do
    {
        data = readl(priv->mmio + FPGA_PSU0_STAT);

        if (data & FPGA_PSU_TX_FINISH_MASK)
        {
            if (data & FPGA_PSU_TX_ERROR_MASK)
            {
                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "psu_wait_bus_tx_done data ECOMM error\r\n");

                return -ECOMM;
            }

            return 0;
        }

    } while (time_before(jiffies, timeout));

    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "psu_wait_bus_tx_done data ETIMEDOUT error\r\n");

    return -ETIMEDOUT;
}

static int clounix_i2c_smbus_xfer_psu0(struct i2c_adapter *adap, unsigned short addr, unsigned short flags,
                                       char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);

    unsigned int tmp_value = 0;

    unsigned char r_addr = 0, w_addr = 0, i = 0, data_size = 0;

    unsigned int *tmp_addr = NULL;

    // LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "clounix_i2c_smbus_xfer_psu0 addr : %x size: %x command %x rw %x\r\n", addr, size, command, read_write);

    mutex_lock(&priv->lock);

    addr = (addr & 0x7f) << 1;

    w_addr = addr;

    r_addr = (addr | 0x01);

    switch (size)
    {

    case I2C_SMBUS_BYTE:

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU0_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
#if 0
            tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

            writel((FPGA_PSU_MGR_RD_BYTE | ((command & 0xFF) << 16) | (data & 0xFF)), priv->mmio + FPGA_PSU_CTRL);

            if (psu_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + FPGA_PSU_STAT);
            }
#endif
        }
        else
        {
            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_WT_NONE | ((command & 0xFF) << 16) | (0x01 << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BYTE_DATA:

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU0_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = (FPGA_PSU_MGR_RD_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + FPGA_PSU0_STAT);
            }
        }
        else
        {
            tmp_value = (FPGA_PSU_MGR_WT_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_WORD_DATA:

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU0_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_RD_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->word = readw((priv->mmio + FPGA_PSU0_RAM_ADDR_OFFSET));
            }
        }
        else
        {
            writew(data->word, (priv->mmio + FPGA_PSU0_RAM_ADDR_OFFSET));

            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_WT_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, (priv->mmio + FPGA_PSU0_CTRL));

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BLOCK_DATA:

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU0_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_RD_WORD | ((command & 0xFF) << 16) | ((I2C_SMBUS_BLOCK_MAX + 1) << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                tmp_value = readl(priv->mmio + FPGA_PSU0_RAM_ADDR_OFFSET);

                data_size = (tmp_value & 0xFF);

                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "I2C_SMBUS_BLOCK_DATA I2C_SMBUS_READ data_size %d\r\n", data_size);

                if (data_size > I2C_SMBUS_BLOCK_MAX)
                {
                    goto out;
                }

                tmp_addr = (unsigned int *)(priv->mmio + FPGA_PSU0_RAM_ADDR_OFFSET);

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

            tmp_addr = (unsigned int *)(priv->mmio + FPGA_PSU0_RAM_ADDR_OFFSET);

            for (i = 0; i <= data_size; i += 4)
            {
                tmp_value = (data->block[i] + (data->block[i + 1] << 8) + (data->block[i + 2] << 16) + (data->block[i + 3] << 24));

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_WT_WORD | ((command & 0xFF) << 16) | ((data_size + 1) << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        break;

    default:
        break;
    }

    mutex_unlock(&priv->lock);
    usleep_range(6000, 10000);
    return 0;

out:
    mutex_unlock(&priv->lock);
    return -ETIMEDOUT;
}

static int clounix_i2c_xfer_psu0(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr = 0, r_addr = 0, w_addr = 0, reg_addr = 0;
    unsigned int *tmp_addr = NULL;
    unsigned int tmp_value = 0, i = 0, j = 0;

    mutex_lock(&priv->lock);

    if (num == 1)//write
    {
        p = &msgs[0];

        if (p->flags & I2C_M_TEN)
            goto out;

        addr = i2c_8bit_addr_from_msg(p);

        w_addr = (addr & (~(0x01)));

        r_addr = (addr | 0x01);

        tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_PSU0_CFG);

        if (p->len == 1)
        {
            tmp_value = (FPGA_PSU_MGR_WT_BYTE | ((p->buf[0] & 0xFF) << 16) | (p->len << 8) | p->buf[1]);

            writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        else
        {
            tmp_addr = (unsigned int *)(priv->mmio + FPGA_PSU0_RAM_ADDR_OFFSET);

            for (j = 1; j <= p->len; j += 4)
            {
                tmp_value = p->buf[j];

                if((j + 1) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 1] << 8);

                if((j + 2) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 2] << 16);

                if((j + 3) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 3] << 24);

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_PSU_MGR_WT_WORD | ((p->buf[0] & 0xFF) << 16) | (p->len << 8));

            writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

            if (psu0_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
    }
    else//read
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
                tmp_value = (FPGA_PSU_MGR_RST | FPGA_PSU_MGR_ENABLE | (r_addr << 8) | w_addr);

                writel(tmp_value, priv->mmio + FPGA_PSU0_CFG);

                if (p->len == 1)
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_PSU_MGR_RD_BYTE | (reg_addr & 0xFF) << 16 | (p->len << 8));

                    writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

                    if (psu0_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        p->buf[0] = readb(priv->mmio + FPGA_PSU0_STAT);
                    }
                }
                else
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_PSU_MGR_RD_WORD | ((reg_addr & 0xFF) << 16) | ((p->len) << 8));

                    writel(tmp_value, priv->mmio + FPGA_PSU0_CTRL);

                    if (psu0_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        tmp_addr = (unsigned int *)(priv->mmio + FPGA_PSU0_RAM_ADDR_OFFSET);

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
    mutex_unlock(&priv->lock);

    return -ETIMEDOUT;
}


static int eeprom_wait_bus_tx_done(struct master_priv_data *priv)
{
    unsigned int data;
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT;

    do
    {
        data = readl(priv->mmio + FPGA_EEPROM_STAT);

        if (data & FPGA_EEPROM_TX_FINISH_MASK)
        {
            if (data & FPGA_EEPROM_TX_ERROR_MASK)
            {
                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "eeprom_wait_bus_tx_done data ECOMM error\r\n");

                return -ECOMM;
            }

            return 0;
        }

    } while (time_before(jiffies, timeout));

    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "eeprom_wait_bus_tx_done data ETIMEDOUT error\r\n");

    return -ETIMEDOUT;
}

static int clounix_i2c_xfer_reboot_eeprom(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr = 0, r_addr = 0, w_addr = 0, reg_addr = 0;
    unsigned int *tmp_addr = NULL;
    unsigned int tmp_value = 0, i = 0, j = 0;

    mutex_lock(&priv->lock);

    if (num == 1)//write
    {
        p = &msgs[0];

        if (p->flags & I2C_M_TEN)
            goto out;

        addr = i2c_8bit_addr_from_msg(p);

        w_addr = (addr & (~(0x01)));

        r_addr = (addr | 0x01);

        tmp_value = (FPGA_EEPROM_MGR_RST | FPGA_EEPROM_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_EEPROM_CFG);
        
        if(p->len > 8)
        {
            p->len = 8;
        }

        if (p->len == 1)
        {
            tmp_value = (FPGA_EEPROM_MGR_WT_BYTE | ((p->buf[0] & 0xFF) << 16) | (p->len << 8) | p->buf[1]);

            writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        else
        {
            tmp_addr = (unsigned int *)(priv->mmio + FPGA_EEPROM_RAM_ADDR_OFFSET);

            for (j = 1; j <= p->len; j += 4)
            {
                tmp_value = p->buf[j];

                if((j + 1) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }

                tmp_value += (p->buf[j + 1] << 8);

                if((j + 2) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                } 

                tmp_value += (p->buf[j + 2] << 16);

                if((j + 3) > p->len)
                {
                    writel(tmp_value, tmp_addr);

                    break;
                }              

                tmp_value += (p->buf[j + 3] << 24);

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_EEPROM_MGR_WT_WORD | ((p->buf[0] & 0xFF) << 16) | (p->len << 8));

            writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
    }
    else//read
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
                tmp_value = (FPGA_EEPROM_MGR_RST | FPGA_EEPROM_MGR_ENABLE | (r_addr << 8) | w_addr);

                writel(tmp_value, priv->mmio + FPGA_EEPROM_CFG);

                if(p->len > 8)
                {
                    p->len = 8;
                }

                if (p->len == 1)
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_EEPROM_MGR_RD_BYTE | (reg_addr & 0xFF) << 16 | (p->len << 8));

                    writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

                    if (eeprom_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        p->buf[0] = readb(priv->mmio + FPGA_EEPROM_STAT);
                    }
                }
                else
                {
                    tmp_value = 0;

                    tmp_value = (FPGA_EEPROM_MGR_RD_WORD | ((reg_addr & 0xFF) << 16) | ((p->len) << 8));

                    writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

                    if (eeprom_wait_bus_tx_done(priv) != 0)
                    {
                        goto out;
                    }
                    else
                    {
                        tmp_addr = (unsigned int *)(priv->mmio + FPGA_EEPROM_RAM_ADDR_OFFSET);

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
    mutex_unlock(&priv->lock);

    return -ETIMEDOUT;
}


static int clounix_i2c_smbus_xfer_reboot_eeprom(struct i2c_adapter *adap, unsigned short addr, unsigned short flags,
                                       char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);

    unsigned int tmp_value = 0;

    unsigned char r_addr = 0, w_addr = 0, i = 0, data_size = 0;

    unsigned int *tmp_addr = NULL;

    LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "clounix_i2c_smbus_xfer_reboot_eeprom addr : %x size: %x command %x rw %x\r\n", addr, size, command, read_write);

    mutex_lock(&priv->lock);

    addr = (addr & 0x7f) << 1;

    w_addr = addr;

    r_addr = (addr | 0x01);

    switch (size)
    {

    case I2C_SMBUS_BYTE:

        tmp_value = (FPGA_EEPROM_MGR_RST | FPGA_EEPROM_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_EEPROM_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
#if 0
            tmp_value = (FPGA_EEPROM_MGR_RST | FPGA_EEPROM_MGR_ENABLE | (r_addr << 8) | w_addr);

            writel((FPGA_EEPROM_MGR_RD_BYTE | ((command & 0xFF) << 16) | (data & 0xFF)), priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + FPGA_EEPROM_STAT);
            }
#endif
        }
        else
        {
            tmp_value = 0;

            tmp_value = (FPGA_EEPROM_MGR_WT_NONE | ((command & 0xFF) << 16) | (0x01 << 8));

            writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BYTE_DATA:

        tmp_value = (FPGA_EEPROM_MGR_RST | FPGA_EEPROM_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_EEPROM_CFG);

        if (read_write == I2C_SMBUS_READ)
        { 
            tmp_value = 0;

            tmp_value = (FPGA_EEPROM_MGR_RD_BYTE | ((command & 0xFF) << 16) | (0x01 << 8));

            writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->byte = readb(priv->mmio + FPGA_EEPROM_STAT);
            }
        }
        else
        {
            tmp_value = (FPGA_EEPROM_MGR_WT_BYTE | ((command & 0xFF) << 16) | (0x01 << 8) | (data->byte));

            writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }

        }

        break;

    case I2C_SMBUS_WORD_DATA:

        tmp_value = (FPGA_EEPROM_MGR_RST | FPGA_EEPROM_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_EEPROM_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_EEPROM_MGR_RD_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                data->word = readw((priv->mmio + FPGA_EEPROM_RAM_ADDR_OFFSET));
            }
        }
        else
        {
            writew(data->word, (priv->mmio + FPGA_EEPROM_RAM_ADDR_OFFSET));

            tmp_value = 0;

            tmp_value = (FPGA_EEPROM_MGR_WT_WORD | ((command & 0xFF) << 16) | (0x02 << 8));

            writel(tmp_value, (priv->mmio + FPGA_EEPROM_CTRL));

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }

        break;

    case I2C_SMBUS_BLOCK_DATA:

        tmp_value = (FPGA_EEPROM_MGR_RST | FPGA_EEPROM_MGR_ENABLE | (r_addr << 8) | w_addr);

        writel(tmp_value, priv->mmio + FPGA_EEPROM_CFG);

        if (read_write == I2C_SMBUS_READ)
        {
            tmp_value = 0;

            tmp_value = (FPGA_EEPROM_MGR_RD_WORD | ((command & 0xFF) << 16) | ((I2C_SMBUS_BLOCK_MAX + 1) << 8));

            writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
            else
            {
                tmp_value = readl(priv->mmio + FPGA_EEPROM_RAM_ADDR_OFFSET);

                data_size = (tmp_value & 0xFF);

                LOG_DBG(CLX_DRIVER_TYPES_I2C_MASTER, "I2C_SMBUS_BLOCK_DATA I2C_SMBUS_READ data_size %d\r\n", data_size);

                if (data_size > I2C_SMBUS_BLOCK_MAX)
                {
                    goto out;
                }

                tmp_addr = (unsigned int *)(priv->mmio + FPGA_EEPROM_RAM_ADDR_OFFSET);

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

            tmp_addr = (unsigned int *)(priv->mmio + FPGA_EEPROM_RAM_ADDR_OFFSET);

            for (i = 0; i <= data_size; i += 4)
            {
                tmp_value = (data->block[i] + (data->block[i + 1] << 8) + (data->block[i + 2] << 16) + (data->block[i + 3] << 24));

                writel(tmp_value, tmp_addr);

                tmp_addr++;
            }

            tmp_value = 0;

            tmp_value = (FPGA_EEPROM_MGR_WT_WORD | ((command & 0xFF) << 16) | ((data_size + 1) << 8));

            writel(tmp_value, priv->mmio + FPGA_EEPROM_CTRL);

            if (eeprom_wait_bus_tx_done(priv) != 0)
            {
                goto out;
            }
        }
        break;

    default:
        break;
    }

    mutex_unlock(&priv->lock);
    usleep_range(6000, 10000);
    return 0;

out:
    mutex_unlock(&priv->lock);
    return -ETIMEDOUT;
}

static struct i2c_algorithm clounix_i2c_algo = {
    .smbus_xfer = clounix_i2c_smbus_xfer,
    .master_xfer = clounix_i2c_xfer,
    .functionality = clounix_i2c_func,
};

static struct i2c_algorithm clounix_i2c_master1_algo = {
    .smbus_xfer = clounix_i2c_smbus_xfer_psu1,
    .master_xfer = clounix_i2c_xfer_psu1,
    .functionality = clounix_i2c_func,
};

static struct i2c_algorithm clounix_i2c_master0_algo = {
    .smbus_xfer = clounix_i2c_smbus_xfer_psu0,
    .master_xfer = clounix_i2c_xfer_psu0,
    .functionality = clounix_i2c_func,
};
static struct i2c_algorithm clounix_i2c_master2_algo = {

    .smbus_xfer = clounix_i2c_smbus_xfer_reboot_eeprom,
    .master_xfer = clounix_i2c_xfer_reboot_eeprom,
    .functionality = clounix_i2c_func,

};



int drv_i2c_xilinx_init(void **driver)
{
    struct pci_dev *pdev = pci_get_device(0x10ee, 0x7021, NULL);
    struct master_priv_data *priv;
    void __iomem *base;
    struct i2c_adapter *adap;
    int total_adap;
    int i, err;

    if (pdev == NULL) {
        LOG_ERR(CLX_DRIVER_TYPES_I2C_MASTER,  "dev not found\r\n");
        return -ENXIO;
    }

    if (clounix_fpga_base == NULL) {
        LOG_ERR(CLX_DRIVER_TYPES_I2C_MASTER,  "resource not iomap\r\n");
        return -ENXIO;
    }
    base = pci_get_drvdata(pdev);

    total_adap = sizeof(priv_conf)/sizeof(struct master_conf);
    group_priv = kzalloc(sizeof(struct master_priv_data) * total_adap, GFP_KERNEL);
    if (group_priv == NULL) {
        LOG_ERR(CLX_DRIVER_TYPES_I2C_MASTER,  "no memory\r\n");
        return -ENOMEM;
    }

    priv = group_priv;
    for (i=0; i<total_adap; i++) {
        adap = &priv[i].adap;
        adap->owner = THIS_MODULE;

        if (strncmp(priv_conf[i].name, "fpga-psu1", strlen("fpga-psu1")) == 0)
        {
            adap->algo = &clounix_i2c_master1_algo;
        }
        else if (strncmp(priv_conf[i].name, "fpga-psu0", strlen("fpga-psu0")) == 0)
        {
            adap->algo = &clounix_i2c_master0_algo;
        }
        else if (strncmp(priv_conf[i].name, "fpga-rebootrom", strlen("fpga-rebootrom")) == 0)
        {
            adap->algo = &clounix_i2c_master2_algo;
        }
        else
        {
            adap->algo = &clounix_i2c_algo;
        }
        adap->retries = DEFAULT_RETRY;
        adap->dev.parent = &pdev->dev;
        adap->dev.of_node = pdev->dev.of_node;
        strlcpy(adap->name, priv_conf[i].name, sizeof(adap->name));
        i2c_set_adapdata(adap, &priv[i]);
        priv[i].mmio = base + priv_conf[i].offset;
        mutex_init(&(priv[i].lock));

        if((strncmp(priv_conf[i].name, "fpga-rebootrom", strlen("fpga-rebootrom")) == 0)\
        || (strncmp(priv_conf[i].name, "fpga-psu0", strlen("fpga-psu0")) == 0)\
        || (strncmp(priv_conf[i].name, "fpga-psu1", strlen("fpga-psu1")) == 0))
        {

        }
        else
        {
            if (fpga_i2c_reinit(&priv[i], XIIC_I2C_TIMEOUT) != 0)
                goto err_i2c_group;
        }

        err = i2c_add_adapter(adap);
        if (err)
            goto err_i2c_group;
    }
    return 0;

err_i2c_group:
    while(--i >= 0) {
        i2c_del_adapter(adap);
    }
    kfree(group_priv);
    return err;
}

void drv_i2c_xilinx_exit(void **driver)
{
    int i, total_adap;
    struct master_priv_data *priv = group_priv;

    total_adap = sizeof(priv_conf)/sizeof(struct master_conf);
    for (i=0; i<total_adap; i++) {
        i2c_del_adapter(&(priv[i].adap));
        writeb(XIIC_RESET_MASK, priv[i].mmio + XIIC_RESETR_OFFSET);
    }

    kfree(group_priv);
    return;
}
