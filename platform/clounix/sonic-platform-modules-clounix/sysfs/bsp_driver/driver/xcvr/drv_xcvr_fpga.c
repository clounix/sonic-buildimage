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

#include "drv_xcvr_fpga.h"
#include "clx_driver.h"

//external function declaration
extern void __iomem *clounix_fpga_base;
extern int g_loglevel;

struct drv_xcvr_fpga drv_xcvr;

static uint8_t clx_fpga_sfp_translate_offset(struct clounix_priv_data *sfp,
        int eth_index, loff_t *offset)
{
    unsigned int page = 0;

    /* if SFP style, offset > 255, shift to i2c addr 0x51 */
    if (sfp->chip[eth_index].dev_class == TWO_ADDR) {
        if (*offset > 255) {
            sfp->chip[eth_index].slave_addr = SFP_EEPROM_A2_ADDR;
            *offset -= 256;
        } else {
            sfp->chip[eth_index].slave_addr = SFP_EEPROM_A0_ADDR;
        }
    }

    /*
     * if offset is in the range 0-128...
     * page doesn't matter (using lower half), return 0.
     * offset is already correct (don't add 128 to get to paged area)
     */
    if (*offset < SFP_PAGE_SIZE)
        return page;

    /* note, page will always be positive since *offset >= 128 */
    page = (*offset >> 7)-1;
    /* 0x80 places the offset in the top half, offset is last 7 bits */
    *offset = SFP_PAGE_SIZE + (*offset & 0x7f);

    return page;  /* note also returning cliclx_fpga_sfp_translate_offsetent and offset */
}
/*
 i2c controller    CLX128000    CLX8000
 0                 0-15         0-23
 1                16-34(sfp 0-1)    24-47    
 2                 Not used     48-55
*/

static int i2c_dev_index[XCVR_PLATFORM_TYPE_MAX][XCVR_I2C_DEV_MAX] = {
    {24, 48, 56},
    {16, 34, -1}
};

int drv_xcvr_get_platform_idx(u8 port_max)
{
    if (port_max == 56)
        return XCVR_PLATFORM_TYPEA;
    else if (port_max == 34)
        return XCVR_PLATFORM_TYPEB;
    else
        return DRIVER_ERR;
}

int drv_xcvr_get_i2c_idx(u8 platform, u8 port)
{
    u8 idx;

    for (idx = 0; idx < XCVR_I2C_DEV_MAX; idx++) {
        if (i2c_dev_index[platform][idx] == (-1)) 
            break;
        if (port < i2c_dev_index[platform][idx])
            return idx;
    }
    return DRIVER_ERR;
}

u8 drv_xcvr_i2c_port_data(u8 port, u8 platform_type, u8 i2c_idx)
{
    u8 idx = 0;

    if (i2c_idx)
        idx = i2c_idx - 1;
    if( port >= i2c_dev_index[platform_type][idx])
        return port- i2c_dev_index[platform_type][idx];
    else
        return port;
}
static int xcvr_cpld_index[XCVR_PLATFORM_TYPE_MAX][XCVR_CPLD_GROUP_MAX] = {
    {30, 56},
    {16, 34}
};
int drv_xcvr_get_cpld_idx(u8 platform, u8 port)
{
    u8 idx;

    for (idx = 0; idx < XCVR_CPLD_GROUP_MAX; idx++) {
        if (port < xcvr_cpld_index[platform][idx])
            return idx;
    }
    return DRIVER_ERR;
}
static int fpga_i2c_byte_read(struct clounix_priv_data *sfp,
            int port,
            char *buf, unsigned int offset)
{
    uint32_t data;
    uint32_t idx = 0;
    int i = 0;

    idx = sfp->chip[port].dev_idx;
    data = 0x80000000;
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    data = 0x40000000 | (((sfp->chip[port].slave_addr+1) & 0xFF) << 8) | (sfp->chip[port].slave_addr & 0xFF) | (sfp->chip[port].clk_div << 16);
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    data = drv_xcvr_i2c_port_data(port, sfp->platform_type, idx);
    writel(data, port_mgr_mux_reg(sfp->mmio, idx));

    data = 0x81000000 | ((offset&0xFF) << 16);
    writel(data, port_mgr_ctrl_reg(sfp->mmio, idx));
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "fpga_i2c_byte_read port %d, offset:0x%x, idx:%d,cfg:%p@%x mux:%p@%x ctrl:%p@%x stat:%p@%x\n", port, offset, idx,
                       port_mgr_cfg_reg(sfp->mmio, idx), readl(port_mgr_cfg_reg(sfp->mmio, idx)),
                       port_mgr_mux_reg(sfp->mmio, idx), readl(port_mgr_mux_reg(sfp->mmio, idx)),
                       port_mgr_ctrl_reg(sfp->mmio, idx), readl(port_mgr_ctrl_reg(sfp->mmio, idx)),
                       port_mgr_stat_reg(sfp->mmio, idx), readl(port_mgr_stat_reg(sfp->mmio, idx))
                       );
    do {
        data = readl(port_mgr_stat_reg(sfp->mmio, idx));
        if ((data & 0xC0000000) == 0x80000000) {
            *buf = data & 0xFF;
            return 0;
        }
        usleep_range(100,200);
    }while(i++ < 1000);
    return -ENXIO;
}

static int fpga_i2c_byte_write(struct clounix_priv_data *sfp,
            int port,
            char *buf, unsigned int offset)
{
    uint32_t data;
    uint32_t idx = 0;
    int i = 0;

    idx = sfp->chip[port].dev_idx;
    data = 0x80000000;
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    data = 0x40000000 | (((sfp->chip[port].slave_addr+1) & 0xFF) << 8) | (sfp->chip[port].slave_addr & 0xFF) | (sfp->chip[port].clk_div << 16);
    writel(data, port_mgr_cfg_reg(sfp->mmio, idx));

    data = drv_xcvr_i2c_port_data(port, sfp->platform_type, idx);
    writel(data, port_mgr_mux_reg(sfp->mmio, idx));

    data = 0x84000000 | ((offset & 0xFF) << 16) | (*buf & 0xFF);
    writel(data, port_mgr_ctrl_reg(sfp->mmio, idx));
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "fpga_i2c_byte_write port %d, offset:0x%x, idx:%d,cfg:%p@%x mux:%p@%x ctrl:%p@%x stat:%p@%x\n", port, offset, idx,
                       port_mgr_cfg_reg(sfp->mmio, idx), readl(port_mgr_cfg_reg(sfp->mmio, idx)),
                       port_mgr_mux_reg(sfp->mmio, idx), readl(port_mgr_mux_reg(sfp->mmio, idx)),
                       port_mgr_ctrl_reg(sfp->mmio, idx), readl(port_mgr_ctrl_reg(sfp->mmio, idx)),
                       port_mgr_stat_reg(sfp->mmio, idx), readl(port_mgr_stat_reg(sfp->mmio, idx))
                       );
    do {
        data = readl(port_mgr_stat_reg(sfp->mmio, idx));
        if ((data & 0xC0000000) == 0x80000000) {
            return 0;
        }
    }while(i++ < 1000);
    return -ENXIO;
}

static ssize_t clx_fpga_sfp_eeprom_read_byte_by_byte(struct clounix_priv_data *sfp,
            int port,
            char *buf, unsigned int offset, size_t count)
{
    unsigned long timeout, write_time;
    int ret;
    int i = 0;

    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        write_time = jiffies;
        ret = fpga_i2c_byte_read(sfp, port, buf + i, (offset + i));
        LOG_DBG(CLX_DRIVER_TYPES_XCVR, "read register byte by byte %d, offset:0x%x, data :0x%x ret:%d\n",
                i, offset + i, *(buf + i), ret);
        if (ret == -ENXIO) /* no module present */
            return ret;
    i++;
    } while (time_before(write_time, timeout) && (i < count));

    if (i == count)
        return count;

    return -ETIMEDOUT;
}

static ssize_t clx_fpga_sfp_eeprom_read(struct clounix_priv_data *sfp,
            int port,
            char *buf, unsigned int offset, size_t count)
{
    unsigned long timeout, read_time;
    int status;

    /*smaller eeproms can work given some SMBus extension calls */
    if (count > I2C_SMBUS_BLOCK_MAX)
        count = I2C_SMBUS_BLOCK_MAX;

    /*
     * Reads fail if the previous write didn't complete yet. We may
     * loop a few times until this one succeeds, waiting at least
     * long enough for one entire page write to work.
     */
    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        read_time = jiffies;

        status = clx_fpga_sfp_eeprom_read_byte_by_byte(sfp, port, buf, offset, count);

        LOG_DBG(CLX_DRIVER_TYPES_XCVR, "eeprom read %zu@%d --> %d (%ld)\n",
                count, offset, status, jiffies);

        if (status == count)  /* happy path */
            return count;

        if (status == -ENXIO) /* no module present */
            return status;

    } while (time_before(read_time, timeout));

    return -ETIMEDOUT;
}

static ssize_t clx_fpga_sfp_eeprom_write_byte_by_byte(
            struct clounix_priv_data *sfp,
            int port,
            const char *buf, unsigned int offset, size_t count)
{
    unsigned long timeout, write_time;
    int ret;
    int i = 0;

    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        write_time = jiffies;
        ret = fpga_i2c_byte_write(sfp, port, (char *)buf + i, (offset + i));
        LOG_DBG(CLX_DRIVER_TYPES_XCVR, "Write register byte by byte %zu, offset:0x%x, data :0x%x ret:%d\n",
                       count, offset + i, *(buf + i), ret);
        if (ret == -ENXIO)
            return ret;
    i++;
    usleep_range(10000, 15000);
    } while (time_before(write_time, timeout) && (i < count));

    if (i == count) {
        return count;
    }

    return -ETIMEDOUT;
}


static ssize_t clx_fpga_sfp_eeprom_write(struct clounix_priv_data *sfp,
                int port,
                const char *buf,
                unsigned int offset, size_t count)
{
    ssize_t status;
    unsigned long timeout, write_time;
    unsigned int next_page_start;

    /* write max is at most a page
     * (In this driver, write_max is actually one byte!)
     */
    if (count > sfp->chip[port].write_max)
        count = sfp->chip[port].write_max;

    /* shorten count if necessary to avoid crossing page boundary */
    next_page_start = roundup(offset + 1, SFP_PAGE_SIZE);
    if ((offset + count) > next_page_start)
        count = next_page_start - offset;

    if (count > I2C_SMBUS_BLOCK_MAX)
        count = I2C_SMBUS_BLOCK_MAX;

    /*
     * Reads fail if the previous write didn't complete yet. We may
     * loop a few times until this one succeeds, waiting at least
     * long enough for one entire page write to work.
     */
    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        write_time = jiffies;

        status = clx_fpga_sfp_eeprom_write_byte_by_byte(sfp, port, buf, offset, count);
        if (status == count)  /* happy path */
            return count;

        if (status == -ENXIO) /* no module present */
            return status;

    } while (time_before(write_time, timeout));

    return -ETIMEDOUT;
}

static ssize_t clx_fpga_sfp_eeprom_update_client(struct clounix_priv_data *sfp,
                int eth_index, char *buf, loff_t off, size_t count)
{
    ssize_t retval = 0;
    uint8_t page = 0;
    uint8_t page_check = 0;
    loff_t phy_offset = off;
    int ret = 0;

    page = clx_fpga_sfp_translate_offset(sfp, eth_index, &phy_offset);

    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "off %lld  page:%d phy_offset:%lld, count:%ld\n",
        off, page, phy_offset, (long int) count);
    if (page > 0) {
        ret = clx_fpga_sfp_eeprom_write(sfp, eth_index, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "Write page register for page %d failed ret:%d!\n",page, ret);
            return ret;
        }
    }
    wmb();

    ret =  clx_fpga_sfp_eeprom_read(sfp, eth_index,
            &page_check , SFP_PAGE_SELECT_REG, 1);
    if (ret < 0) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "Read page register for page %d failed ret:%d!\n",page, ret);
            return ret;
    }
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "read page register %d checked %d, ret:%d\n",page, page_check, ret);

    while (count) {
        ssize_t status;

        status =  clx_fpga_sfp_eeprom_read(sfp, eth_index,
            buf, phy_offset, count);

        if (status <= 0) {
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        phy_offset += status;
        count -= status;
        retval += status;
    }


    if (page > 0) {
        /* return the page register to page 0 (why?) */
        page = 0;
        ret = clx_fpga_sfp_eeprom_write(sfp, eth_index, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "Restore page register to 0 failed:%d!\n", ret);
            /* error only if nothing has been transferred */
            if (retval == 0)
                retval = ret;
        }
    }
    return retval;
}

static ssize_t clx_fpga_sfp_eeprom_write_client(struct clounix_priv_data *sfp,
                int eth_index, char *buf, loff_t off, size_t count)
{
    ssize_t retval = 0;
    uint8_t page = 0;
    uint8_t page_check = 0;
    loff_t phy_offset = off;
    int ret = 0;

    page = clx_fpga_sfp_translate_offset(sfp, eth_index, &phy_offset);

    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "off %lld  page:%d phy_offset:%lld, count:%ld\n",
        off, page, phy_offset, (long int) count);
    if (page > 0) {
        ret = clx_fpga_sfp_eeprom_write(sfp, eth_index, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "Write page register for page %d failed ret:%d!\n",page, ret);
            return ret;
        }
    }
    wmb();

    ret =  clx_fpga_sfp_eeprom_read(sfp, eth_index,
            &page_check , SFP_PAGE_SELECT_REG, 1);
    if (ret < 0) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "Read page register for page %d failed ret:%d!\n",page, ret);
            return ret;
    }
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "read page register %d checked %d, ret:%d\n",page, page_check, ret);

    ret =  clx_fpga_sfp_eeprom_write(sfp, eth_index,
           buf, phy_offset, count);

    if (ret < 0) {
        retval = ret;
        LOG_ERR(CLX_DRIVER_TYPES_XCVR, "write eeprom failed:offset:0x%llx bytes:%d ret:%d!\n", phy_offset, eth_index, ret);
    }

    if (page > 0) {
        /* return the page register to page 0 (why?) */
        page = 0;
        ret = clx_fpga_sfp_eeprom_write(sfp, eth_index, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "Restore page register to 0 failed:%d!\n", ret);
            /* error only if nothing has been transferred */
            if (retval == 0)
                retval = ret;
        }
    }
    return retval;
}

/*
 * Figure out if this access is within the range of supported pages.
 * Note this is called on every access because we don't know if the
 * module has been replaced since the last call.
 * If/when modules support more pages, this is the routine to update
 * to validate and allow access to additional pages.
 *
 * Returns updated len for this access:
 *     - entire access is legal, original len is returned.
 *     - access begins legal but is too long, len is truncated to fit.
 *     - initial offset exceeds supported pages, return OPTOE_EOF (zero)
 */
static ssize_t clx_fpga_sfp_page_legal(struct clounix_priv_data *sfp,
        int eth_index, loff_t off, size_t len)
{
    u8 regval;
    int not_pageable;
    int status;
    size_t maxlen;

    if (off < 0)
        return -EINVAL;

    if (sfp->chip[eth_index].dev_class == TWO_ADDR) {
        /* SFP case */
        /* if only using addr 0x50 (first 256 bytes) we're good */
        if ((off + len) <= TWO_ADDR_NO_0X51_SIZE)
            return len;
        /* if offset exceeds possible pages, we're not good */
        if (off >= TWO_ADDR_EEPROM_SIZE)
            return SFP_EOF;
        /* in between, are pages supported? */
        status = clx_fpga_sfp_eeprom_read(sfp, eth_index, &regval,
                TWO_ADDR_PAGEABLE_REG, 1);
        if (status < 0)
            return status;  /* error out (no module?) */
        if (regval & TWO_ADDR_PAGEABLE) {
            /* Pages supported, trim len to the end of pages */
            maxlen = TWO_ADDR_EEPROM_SIZE - off;
        } else {
            /* pages not supported, trim len to unpaged size */
            if (off >= TWO_ADDR_EEPROM_UNPAGED_SIZE)
                return SFP_EOF;

            /* will be accessing addr 0x51, is that supported? */
            /* byte 92, bit 6 implies DDM support, 0x51 support */
            status = clx_fpga_sfp_eeprom_read(sfp, eth_index, &regval,
                        TWO_ADDR_0X51_REG, 1);
            if (status < 0)
                return status;
            if (regval & TWO_ADDR_0X51_SUPP) {
                /* addr 0x51 is OK */
                maxlen = TWO_ADDR_EEPROM_UNPAGED_SIZE - off;
            } else {
                /* addr 0x51 NOT supported, trim to 256 max */
                if (off >= TWO_ADDR_NO_0X51_SIZE)
                    return SFP_EOF;
                maxlen = TWO_ADDR_NO_0X51_SIZE - off;
            }
        }
        len = (len > maxlen) ? maxlen : len;
        LOG_DBG(CLX_DRIVER_TYPES_XCVR, "page_legal, SFP, off %lld len %ld\n",
            off, (long int) len);
    } else {
        /* QSFP case, CMIS case */
        /* if no pages needed, we're good */
        if ((off + len) <= ONE_ADDR_EEPROM_UNPAGED_SIZE)
            return len;
        /* if offset exceeds possible pages, we're not good */
        if (off >= ONE_ADDR_EEPROM_SIZE)
            return SFP_EOF;
        /* in between, are pages supported? */
        status = clx_fpga_sfp_eeprom_read(sfp, eth_index, &regval,
                ONE_ADDR_PAGEABLE_REG, 1);
        if (status < 0)
            return status;  /* error out (no module?) */

        if (sfp->chip[eth_index].dev_class == ONE_ADDR) {
            not_pageable = QSFP_NOT_PAGEABLE;
        } else {
            not_pageable = CMIS_NOT_PAGEABLE;
        }
        LOG_DBG(CLX_DRIVER_TYPES_XCVR, "Paging Register: 0x%x; not_pageable mask: 0x%x\n",
            regval, not_pageable);

        if (regval & not_pageable) {
            /* pages not supported, trim len to unpaged size */
            if (off >= ONE_ADDR_EEPROM_UNPAGED_SIZE)
                return SFP_EOF;
            maxlen = ONE_ADDR_EEPROM_UNPAGED_SIZE - off;
        } else {
            /* Pages supported, trim len to the end of pages */
            maxlen = ONE_ADDR_EEPROM_SIZE - off;
        }
        len = (len > maxlen) ? maxlen : len;
        LOG_DBG(CLX_DRIVER_TYPES_XCVR, "page_legal, QSFP, off %lld len %ld\n",
            off, (long int) len);
    }
    return len;
}

static ssize_t clx_fpga_sfp_read(struct clounix_priv_data *sfp,
        int eth_index, char *buf, loff_t off, size_t len)
{
    int chunk;
    int status = 0;
    ssize_t retval;
    size_t pending_len = 0, chunk_len = 0;
    loff_t chunk_offset = 0, chunk_start_offset = 0;
    loff_t chunk_end_offset = 0;

    if (unlikely(!len))
        return len;
    
    /*
     * Read data from chip, protecting against concurrent updates
     * from this host, but not from other I2C masters.
     */
    mutex_lock(&sfp->lock);

    /*
     * Confirm this access fits within the device suppored addr range
     */
    status = clx_fpga_sfp_page_legal(sfp, eth_index, off, len);
    if ((status == SFP_EOF) || (status < 0)) {
        mutex_unlock(&sfp->lock);
        return status;
    }
    len = status;

    /*
     * For each (128 byte) chunk involved in this request, issue a
     * separate call to sff_eeprom_update_client(), to
     * ensure that each access recalculates the client/page
     * and writes the page register as needed.
     * Note that chunk to page mapping is confusing, is different for
     * QSFP and SFP, and never needs to be done.  Don't try!
     */
    pending_len = len; /* amount remaining to transfer */
    retval = 0;  /* amount transferred */
    for (chunk = off >> 7; chunk <= (off + len - 1) >> 7; chunk++) {

        /*
         * Compute the offset and number of bytes to be read/write
         *
         * 1. start at an offset not equal to 0 (within the chunk)
         *    and read/write less than the rest of the chunk
         * 2. start at an offset not equal to 0 and read/write the rest
         *    of the chunk
         * 3. start at offset 0 (within the chunk) and read/write less
         *    than entire chunk
         * 4. start at offset 0 (within the chunk), and read/write
         *    the entire chunk
         */
        chunk_start_offset = chunk * SFP_PAGE_SIZE;
        chunk_end_offset = chunk_start_offset + SFP_PAGE_SIZE;

        if (chunk_start_offset < off) {
            chunk_offset = off;
            if ((off + pending_len) < chunk_end_offset)
                chunk_len = pending_len;
            else
                chunk_len = chunk_end_offset - off;
        } else {
            chunk_offset = chunk_start_offset;
            if (pending_len < SFP_PAGE_SIZE)
                chunk_len = pending_len;
            else
                chunk_len = SFP_PAGE_SIZE;
        }

        /*
         * note: chunk_offset is from the start of the EEPROM,
         * not the start of the chunk
         */
        status = clx_fpga_sfp_eeprom_update_client(sfp, eth_index, buf,
                chunk_offset, chunk_len);
        if (status != chunk_len) {
            /* This is another 'no device present' path */
            if (status > 0)
                retval += status;
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        pending_len -= status;
        retval += status;
    }
    mutex_unlock(&sfp->lock);

    return retval;
}

static ssize_t clx_fpga_sfp_write(struct clounix_priv_data *sfp,
        int eth_index, char *buf, loff_t off, size_t len)
{
    int chunk;
    int status = 0;
    ssize_t retval;
    size_t pending_len = 0, chunk_len = 0;
    loff_t chunk_offset = 0, chunk_start_offset = 0;
    loff_t chunk_end_offset = 0;

    if (unlikely(!len))
        return len;

    /*
     * Read data from chip, protecting against concurrent updates
     * from this host, but not from other I2C masters.
     */
    mutex_lock(&sfp->lock);

    /*
     * Confirm this access fits within the device suppored addr range
     */
    status = clx_fpga_sfp_page_legal(sfp, eth_index, off, len);
    if ((status == SFP_EOF) || (status < 0)) {
        mutex_unlock(&sfp->lock);
        return status;
    }
    len = status;

    /*
     * For each (128 byte) chunk involved in this request, issue a
     * separate call to sff_eeprom_update_client(), to
     * ensure that each access recalculates the client/page
     * and writes the page register as needed.
     * Note that chunk to page mapping is confusing, is different for
     * QSFP and SFP, and never needs to be done.  Don't try!
     */
    pending_len = len; /* amount remaining to transfer */
    retval = 0;  /* amount transferred */
    for (chunk = off >> 7; chunk <= (off + len - 1) >> 7; chunk++) {

        /*
         * Compute the offset and number of bytes to be read/write
         *
         * 1. start at an offset not equal to 0 (within the chunk)
         *    and read/write less than the rest of the chunk
         * 2. start at an offset not equal to 0 and read/write the rest
         *    of the chunk
         * 3. start at offset 0 (within the chunk) and read/write less
         *    than entire chunk
         * 4. start at offset 0 (within the chunk), and read/write
         *    the entire chunk
         */
        chunk_start_offset = chunk * SFP_PAGE_SIZE;
        chunk_end_offset = chunk_start_offset + SFP_PAGE_SIZE;

        if (chunk_start_offset < off) {
            chunk_offset = off;
            if ((off + pending_len) < chunk_end_offset)
                chunk_len = pending_len;
            else
                chunk_len = chunk_end_offset - off;
        } else {
            chunk_offset = chunk_start_offset;
            if (pending_len < SFP_PAGE_SIZE)
                chunk_len = pending_len;
            else
                chunk_len = SFP_PAGE_SIZE;
        }

        /*
         * note: chunk_offset is from the start of the EEPROM,
         * not the start of the chunk
         */
        status = clx_fpga_sfp_eeprom_write_client(sfp, eth_index, buf,
                chunk_offset, chunk_len);
        if (status != chunk_len) {
            /* This is another 'no device present' path */
            if (status > 0)
                retval += status;
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        pending_len -= status;
        retval += status;
    }
    mutex_unlock(&sfp->lock);

    return retval;
}

static int drv_xcvr_get_eth_number(void *xcvr)
{
    struct drv_xcvr_fpga *driver = (struct drv_xcvr_fpga *)xcvr;
    return (driver->xcvr_if.port_max);
}

/*
 * drv_xcvr_get_transceiver_power_on_status - Used to get the whole machine port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t platforma_xcvr_get_transceiver_power_on_status(struct clounix_priv_data *sfp, char *buf, size_t count)
{
    /*spf 0 -47  power on */
    size_t poweron_bit_map = 0xffffffffffffUL;
    size_t data = 0;
    uint32_t reg = QSFP_CONFIG_ADDRESS_BASE;
  

    /*sfp 48-56*/
    data = fpga_reg_read(sfp, reg);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data: %lx\r\n", reg, data);

    poweron_bit_map |=  ((data >> QSFP_CONFIG_POWER_EN_OFFSET) & 0xffUL) << QSFP_START_PORT;
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "poweron_bit_map:%lx\r\n", poweron_bit_map);

    return sprintf(buf, "0x%lx\n", poweron_bit_map);
}
static ssize_t platformb_xcvr_get_transceiver_power_on_status(struct clounix_priv_data *sfp, char *buf, size_t count)
{
    /*Qspf 1 -34  power on */
    size_t poweron_bit_map = 0x3ffffffffUL;

    return sprintf(buf, "0x%lx\n", poweron_bit_map);
}
/*
 * drv_xcvr_get_transceiver_power_on_status - Used to get the whole machine port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_transceiver_power_on_status(void *xcvr, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    switch(sfp->platform_type)
    {
    case XCVR_PLATFORM_TYPEA:
        return platforma_xcvr_get_transceiver_power_on_status(sfp,buf,count);
    case XCVR_PLATFORM_TYPEB:
        return platformb_xcvr_get_transceiver_power_on_status(sfp,buf,count);
    default:
        return -ENOSYS;
    }
}
/*
 * drv_xcvr_set_transceiver_power_on_status - Used to set the whole machine port power on status,
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_xcvr_set_transceiver_power_on_status(void *xcvr, int status)
{
    /* it does not need to supported?*/
    return -ENOSYS;
}
static ssize_t platforma_xcvr_get_transceiver_presence_status(struct clounix_priv_data *sfp, char *buf, size_t count)
{
    ssize_t data1 = 0, data2 = 0,data3 = 0;
    ssize_t present_bit_map = 0;

    /*sfp 0-29*/
    data1 = fpga_reg_read(sfp, DSFP_PRESENT_ADDRESS_BASE);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data1: %lx\r\n", DSFP_PRESENT_ADDRESS_BASE, data1);
    /*sfp 30-47*/
    data2 = fpga_reg_read(sfp, (DSFP_PRESENT_ADDRESS_BASE+0x10));
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data2: %lx\r\n", DSFP_PRESENT_ADDRESS_BASE+0x10, data2);
     /*sfp 48-57*/
    data3 = fpga_reg_read(sfp, QSFP_STATUS_ADDRESS_BASE);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data3: %lx\r\n", QSFP_STATUS_ADDRESS_BASE, data3);

    present_bit_map |= (data1 & 0x3FFFFFFF) | ((data2 & 0x3FFFF) << 30)| ((data3 & 0xffUL) << QSFP_START_PORT);

    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "present_bit_map:0x%lx\r\n", present_bit_map);


    return sprintf(buf, "0x%lx\n", present_bit_map);
}
static ssize_t platformb_xcvr_get_transceiver_presence_status(struct clounix_priv_data *sfp, char *buf, size_t count)
{
    size_t data1 = 0, data2 = 0,data3 = 0;
    ssize_t present_bit_map = 0;
    
    /*sfp 0-15*/
    data1 = fpga_reg_read(sfp, DSFP_PRESENT_ADDRESS_BASE);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data1: %lx\r\n", DSFP_PRESENT_ADDRESS_BASE, data1);
    /*sfp 16-31*/
    data2 = fpga_reg_read(sfp, (DSFP_PRESENT_ADDRESS_BASE+0x10));
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data2: %lx\r\n", DSFP_PRESENT_ADDRESS_BASE+0x10, data2);
     /*sfp 32-33*/
    data3 = fpga_reg_read(sfp, SFP_STATUS_ADDRESS_BASE);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data3: %lx\r\n", QSFP_STATUS_ADDRESS_BASE, data3);

    present_bit_map |= (data1 & 0xFFFF) | ((data2 & 0xFFFF) << 16)| (((data3 >>SFP_STATUS_PRESENT_OFFSET) & 0x3UL) << SFP_START_PORT);

    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "present_bit_map:0x%lx\r\n", present_bit_map);

    return sprintf(buf, "0x%lx\n", present_bit_map);
}
/*
 * drv_xcvr_get_transceiver_presence_status - Used to get the whole machine port power on status,
 * filled the value to buf, 0: No presence, 1: Presence
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_transceiver_presence_status(void *xcvr, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);

    switch(sfp->platform_type)
    {
    case XCVR_PLATFORM_TYPEA:
        return platforma_xcvr_get_transceiver_presence_status(sfp,buf,count);
    case XCVR_PLATFORM_TYPEB:
        return platformb_xcvr_get_transceiver_presence_status(sfp,buf,count);
    default:
        return -ENOSYS;
    }
}
/*
 * drv_xcvr_get_eth_power_on_status - Used to get single port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_eth_power_on_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    /*sfp 0 -47  power on bit is not supported default setting power on*/
    size_t val = 0x1;
    uint32_t data = 0;
    uint32_t reg = QSFP_CONFIG_ADDRESS_BASE;
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    /*sfp 48-55*/
    if (eth_index >= QSFP_START_PORT)
    {
        data = fpga_reg_read(sfp, reg);
        LOG_DBG(CLX_DRIVER_TYPES_XCVR, " eth：%d reg: %x, data: %x\r\n", eth_index, reg, data);
        GET_BIT((data >> QSFP_CONFIG_POWER_EN_OFFSET), (eth_index - QSFP_START_PORT), val);
    }

    return sprintf(buf, "%ld\n", val);
}

/*
 * drv_xcvr_set_eth_power_on_status - Used to set single port power on status,
 * @eth_index: start with 0
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_xcvr_set_eth_power_on_status(void *xcvr, unsigned int eth_index, int status)
{
    uint32_t data = 0;
    uint32_t reg = QSFP_CONFIG_ADDRESS_BASE;
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);

    if (eth_index < QSFP_START_PORT)
        return DRIVER_OK;

    data = fpga_reg_read(sfp, reg);
    if(status)
        SET_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_POWER_EN_OFFSET));
    else
        CLEAR_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_POWER_EN_OFFSET));

    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " set power on:eth%d reg: %x, data: %x\r\n", eth_index, reg, data);
    fpga_reg_write(sfp, reg, data);

    return DRIVER_OK;
}
static ssize_t platformb_xcvr_get_eth_tx_fault_status(struct clounix_priv_data *sfp, unsigned int eth_index, char *buf, size_t count)
{
    size_t val = 0x1;
    uint32_t data = 0;
    uint32_t reg = SFP_STATUS_ADDRESS_BASE;

    if (eth_index < SFP_START_PORT)
        return -ENOSYS;

    data = fpga_reg_read(sfp, reg);
    GET_BIT((data >> SFP_STATUS_TXTAULT_OFFSET), (eth_index - SFP_START_PORT), val);
    return sprintf(buf, "%ld\n", val);
}

/*
 * drv_xcvr_get_eth_tx_fault_status - Used to get port tx_fault status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_eth_tx_fault_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    switch(sfp->platform_type)
    {
    case XCVR_PLATFORM_TYPEB:
        return platformb_xcvr_get_eth_tx_fault_status(sfp,eth_index,buf,count);
    case XCVR_PLATFORM_TYPEA:
    default:
             return -ENOSYS;
    }
}
static ssize_t platformb_xcvr_get_eth_tx_disable_status(struct clounix_priv_data *sfp , unsigned int eth_index, char *buf, size_t count)
{
    size_t val = 0x1;
    uint32_t data = 0;
    uint32_t reg = SFP_CONFIG_ADDRESS_BASE;

    if (eth_index < SFP_START_PORT)
        return -ENOSYS;

    data = fpga_reg_read(sfp, reg);
    GET_BIT((data >> SFP_CONFIG_TX_DIS_OFFSET), (eth_index - SFP_START_PORT), val);
    return sprintf(buf, "%ld\n", val);
}
/*
 * drv_xcvr_get_eth_tx_disable_status - Used to get port tx_disable status,
 * filled the value to buf, 0: tx_enable, 1: tx_disable
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_eth_tx_disable_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    switch(sfp->platform_type)
    {
    case XCVR_PLATFORM_TYPEA:
        return -ENOSYS;
    case XCVR_PLATFORM_TYPEB:
        return platformb_xcvr_get_eth_tx_disable_status(sfp,eth_index,buf,count);
    default:
        return -ENOSYS;
    }
}
static int platformb_xcvr_set_eth_tx_disable_status(struct clounix_priv_data *sfp, unsigned int eth_index, int status)
{
    uint32_t data = 0;
    uint32_t reg = SFP_CONFIG_ADDRESS_BASE;

    if (eth_index < SFP_START_PORT)
        return DRIVER_OK;

    data = fpga_reg_read(sfp, reg);
    if(status)
        SET_BIT(data, (eth_index - SFP_START_PORT + SFP_CONFIG_TX_DIS_OFFSET));
    else
        CLEAR_BIT(data, (eth_index - SFP_START_PORT + SFP_CONFIG_TX_DIS_OFFSET));

    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " set tx disable:eth%d reg: %x, data: %x\r\n", eth_index, reg, data);
    fpga_reg_write(sfp, reg, data);

    return DRIVER_OK;
}
/*
 * drv_xcvr_set_eth_tx_disable_status - Used to set port tx_disable status,
 * @eth_index: start with 0
 * @status: tx_disable status, 0: tx_enable, 1: tx_disable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_xcvr_set_eth_tx_disable_status(void *xcvr, unsigned int eth_index, int status)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    switch(sfp->platform_type)
    {
        case XCVR_PLATFORM_TYPEA:
            return -ENOSYS;

        case XCVR_PLATFORM_TYPEB:
            return platformb_xcvr_set_eth_tx_disable_status(sfp,eth_index,status);
        default:
             return -ENOSYS;
    }
}
static ssize_t platformb_xcvr_get_eth_rx_los_status(struct clounix_priv_data *sfp , unsigned int eth_index, char *buf, size_t count)
{
    size_t val = 0x1;
    uint32_t data = 0;
    uint32_t reg = SFP_STATUS_ADDRESS_BASE;

    if (eth_index < SFP_START_PORT)
        return -ENOSYS;

    data = fpga_reg_read(sfp, reg);
    GET_BIT((data >> SFP_STATUS_RXLOS_OFFSET), (eth_index - SFP_START_PORT), val);
    return sprintf(buf, "%ld\n", val);
}
/*
 * drv_xcvr_get_eth_rx_los_status - Used to get port rx_los status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_eth_rx_los_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    switch(sfp->platform_type)
    {
        case XCVR_PLATFORM_TYPEA:
            return -ENOSYS;
        case XCVR_PLATFORM_TYPEB:
            return platformb_xcvr_get_eth_rx_los_status(sfp,eth_index,buf,count);
        default:
             return -ENOSYS;
    }
}
#define PORT_DSFP 1
#define PORT_QSFP 2
#define PORT_SFP 3

static int get_sfp_porttype(unsigned int eth_index,u8 platform_type)
{
    if (XCVR_PLATFORM_TYPEA == platform_type) {
        if(eth_index >= QSFP_START_PORT)
            return PORT_QSFP;
        else
            return PORT_DSFP; 
    }
    if (XCVR_PLATFORM_TYPEB == platform_type) {
        if(eth_index >= SFP_START_PORT)
            return PORT_SFP;
        else
            return PORT_DSFP; 
    }
    return DRIVER_ERR;
}
static ssize_t get_dsfp_present(struct clounix_priv_data *sfp,
                unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0, idx = 0,reg;

    idx = sfp->chip[eth_index].cpld_idx;
    GET_DSFP_PRESENT_ADDRESS(idx, reg);
    data = fpga_reg_read(sfp, reg);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "eth_index:%d, reg: %x, data: %x\r\n",eth_index, reg, data);
    if(eth_index >= xcvr_cpld_index[sfp->platform_type][0])
        GET_BIT(data, (eth_index - xcvr_cpld_index[sfp->platform_type][0]), val);
    else
        GET_BIT(data, eth_index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_qsfp_present(struct clounix_priv_data *sfp,
                unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0;

    data = fpga_reg_read(sfp, QSFP_STATUS_ADDRESS_BASE);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data: %x\r\n", QSFP_STATUS_ADDRESS_BASE, data);
    GET_BIT((data >> QSFP_STATUS_PRESENT_OFFSET), (eth_index - QSFP_START_PORT), val);

    return sprintf(buf, "%d\n", val);
}
static ssize_t get_sfp_present(struct clounix_priv_data *sfp,
                unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0;

    data = fpga_reg_read(sfp, SFP_STATUS_ADDRESS_BASE);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data: %x\r\n", SFP_STATUS_ADDRESS_BASE, data);
    GET_BIT((data >> SFP_STATUS_PRESENT_OFFSET), (eth_index - SFP_START_PORT), val);

    return sprintf(buf, "%d\n", val);
}
/*
 * drv_xcvr_get_eth_present_status - Used to get port present status,
 * filled the value to buf, 1: present, 0: absent
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_eth_present_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    switch (get_sfp_porttype(eth_index,sfp->platform_type))
    {
    case PORT_DSFP:
        return get_dsfp_present(sfp, eth_index, buf, count);
    case PORT_QSFP:
        return get_qsfp_present(sfp, eth_index, buf, count);
    case PORT_SFP:
        return get_sfp_present(sfp, eth_index, buf, count);       
    default:
        return -ENOSYS;
    }
}


static ssize_t get_dsfp_reset(struct clounix_priv_data *sfp, unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0, reg;
   
    GET_DSFP_RST_ADDRESS(sfp->chip[eth_index].cpld_idx, reg);
    data = fpga_reg_read(sfp, reg);

    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data: %x\r\n",  reg, data);
    if(eth_index >= xcvr_cpld_index[sfp->platform_type][0])
        GET_BIT(data, (eth_index - xcvr_cpld_index[sfp->platform_type][0]), val);
    else
        GET_BIT(data, eth_index, val);
    return sprintf(buf, "%d\n", !val);
}

/*QSFP CPLD 0:reset  1:not reset. clounix 0:not reset 1:reset*/
static ssize_t get_qsfp_reset(struct clounix_priv_data *sfp, unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0;
    uint32_t reg = QSFP_CONFIG_ADDRESS_BASE;

    data = fpga_reg_read(sfp, reg);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data: %x\r\n", reg, data);
    GET_BIT((data >> QSFP_CONFIG_RESET_OFFSET), (eth_index - QSFP_START_PORT), val);

    return sprintf(buf, "%d\n", !val);
}
/*
 * drv_xcvr_get_eth_reset_status - Used to get port reset status,
 * filled the value to buf, 0: unreset, 1: reset
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_eth_reset_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);

    switch (get_sfp_porttype(eth_index,sfp->platform_type))
    {
    case PORT_DSFP:
        return get_dsfp_reset(sfp, eth_index, buf, count);
    case PORT_QSFP:
        return get_qsfp_reset(sfp, eth_index, buf, count);
    case PORT_SFP:
        return count;

    default:
        return -ENOSYS;
    }
}

/*DSFP CPLD 0:reset  1:not reset. KWAI 0:not reset 1:reset*/
static ssize_t set_dsfp_reset(struct clounix_priv_data *sfp, unsigned int eth_index, int status)
{
    uint32_t data = 0, reg;

    GET_DSFP_RST_ADDRESS(sfp->chip[eth_index].cpld_idx, reg);
    data = fpga_reg_read(sfp, reg);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data: %x\r\n", reg, data);
    if(0x1 == status)
    {
        if(eth_index >= xcvr_cpld_index[sfp->platform_type][0])
            CLEAR_BIT(data, (eth_index - xcvr_cpld_index[sfp->platform_type][0]));
        else
            CLEAR_BIT(data, eth_index );
    }else
    {
        if(eth_index >= xcvr_cpld_index[sfp->platform_type][0])
            SET_BIT(data, (eth_index - xcvr_cpld_index[sfp->platform_type][0]));
        else
            SET_BIT(data, eth_index);
    }
    fpga_reg_write(sfp, reg, data);
    return 0;
}

/*QSFP CPLD 0:reset  1:not reset. clounix 0:not reset 1:reset*/
static ssize_t set_qsfp_reset(struct clounix_priv_data *sfp, unsigned int eth_index, int status)
{
    uint32_t data = 0;
    uint32_t reg = QSFP_CONFIG_ADDRESS_BASE;

    data = fpga_reg_read(sfp, reg);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, " reg: %x, data: %x\r\n", reg, data);
    if(status)
        CLEAR_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_RESET_OFFSET));
    else
        SET_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_RESET_OFFSET));
    fpga_reg_write(sfp, reg, data);
    return 0;
}

/*
 * drv_xcvr_set_eth_reset_status - Used to set port reset status,
 * @eth_index: start with 0
 * @status: reset status, 0: unreset, 1: reset
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_xcvr_set_eth_reset_status(void *xcvr, unsigned int eth_index, int status)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    switch (get_sfp_porttype(eth_index,sfp->platform_type))
    {
    case PORT_DSFP:
        return set_dsfp_reset(sfp, eth_index, status);
    case PORT_QSFP:
        return set_qsfp_reset(sfp, eth_index, status);
    case PORT_SFP:
        return 0;
    default:
        return -ENOSYS;
    }
}


/*DSFP CPLD 0:Low  1:High. clounix 0:High 1:Low*/
static ssize_t get_dsfp_lowpower(struct clounix_priv_data *sfp,
                                unsigned int eth_index,  char *buf, int count)
{
    uint32_t data = 0, val = 0, reg;

    GET_DSFP_LOWPOWER_ADDRESS(sfp->chip[eth_index].cpld_idx, reg);
    data = fpga_reg_read(sfp, reg);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "low power mode eth%d reg: %x, data: %x\r\n", eth_index, reg, data);
    if(eth_index >= xcvr_cpld_index[sfp->platform_type][0])
        GET_BIT(data, (eth_index - xcvr_cpld_index[sfp->platform_type][0]), val);
    else
        GET_BIT(data, eth_index, val);
    return sprintf(buf, "0x%02x\n", !val);//convert val for clounix requirement
}

static ssize_t set_dsfp_lowpower(struct clounix_priv_data *sfp,
                                unsigned int eth_index, int status)
{
    uint32_t data = 0, reg;

    GET_DSFP_LOWPOWER_ADDRESS(sfp->chip[eth_index].cpld_idx, reg);
    data = fpga_reg_read(sfp, reg);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "set low power mode eth%d reg: %x, data: %x\r\n",eth_index, reg, data);

    if(status) {//convert val for clounix requirement
       if(eth_index >= xcvr_cpld_index[sfp->platform_type][0])
            CLEAR_BIT(data, (eth_index - xcvr_cpld_index[sfp->platform_type][0]));
        else
            CLEAR_BIT(data, eth_index);
    }else{
       if(eth_index >= xcvr_cpld_index[sfp->platform_type][0])
            SET_BIT(data, (eth_index - xcvr_cpld_index[sfp->platform_type][0]));
        else
            SET_BIT(data, eth_index);
    }
    fpga_reg_write(sfp, reg, data);

    return 0;
}

/*QSFP-DD CPLD 1:High  0:Low. clounix 0:High 1:Low*/
static ssize_t get_qsfp_lowpower(struct clounix_priv_data *sfp,
                                unsigned int eth_index, char *buf, int count)
{
    uint32_t data = 0, val = 0;

    data = fpga_reg_read(sfp, QSFP_CONFIG_ADDRESS_BASE);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "reg: %x, data: %x\r\n", QSFP_CONFIG_ADDRESS_BASE, data);
    GET_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_POWER_MODE_OFFSET), val);
    return sprintf(buf, "0x%02x\n", !val);
}

/*QSFP CPLD 0:reset  1:not reset. clounix 0:not reset 1:reset*/
static ssize_t set_qsfp_lowpower(struct clounix_priv_data *sfp,
                                unsigned int eth_index, int status)
{
    uint32_t data = 0;

    data = fpga_reg_read(sfp, QSFP_CONFIG_ADDRESS_BASE);
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "reg: %x, data: %x\r\n", QSFP_STATUS_ADDRESS_BASE, data);
    if(status)
        CLEAR_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_POWER_MODE_OFFSET));
    else
        SET_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_POWER_MODE_OFFSET));
    fpga_reg_write(sfp, QSFP_CONFIG_ADDRESS_BASE, data);
    return 0;
}

/*
 * drv_xcvr_get_eth_low_power_mode_status - Used to get port low power mode status,
 * filled the value to buf, 0: high power mode, 1: low power mode
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_eth_low_power_mode_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);

    switch (get_sfp_porttype(eth_index,sfp->platform_type))
    {
    case PORT_DSFP:
        return get_dsfp_lowpower(sfp, eth_index,  buf, count);
    case PORT_QSFP:
        return get_qsfp_lowpower(sfp, eth_index, buf, count);
    case PORT_SFP:  
        return sprintf(buf, "0x%02x\n", 1);;    
    default:
        return -ENOSYS;
    }
}

/*
 * drv_xcvr_set_eth_low_power_mode_status - Used to set port low power mode status,
 * filled the status, 0: high power mode, 1: low power mode
 * @eth_index: start with 0
 * @status: low power mode
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static int drv_xcvr_set_eth_low_power_mode_status(void *xcvr, unsigned int eth_index, int status)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);
    
    switch (get_sfp_porttype(eth_index,sfp->platform_type))
    {
    case PORT_DSFP:
        return set_dsfp_lowpower(sfp, eth_index, status);
    case PORT_QSFP:
        return set_qsfp_lowpower(sfp, eth_index, status);
    case PORT_SFP:
        return 0;
    default:
        return -ENOSYS;
    }
}
/*
 * drv_xcvr_get_eth_interrupt_status - Used to get port interruption status,
 * filled the value to buf, 0: no interruption, 1: interruption
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_get_eth_interrupt_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    /* it is not supported */
    return -ENOSYS;
}

/*
 * drv_xcvr_get_eth_eeprom_size - Used to get port eeprom size
 *
 * This function returns the size of port eeprom,
 * otherwise it returns a negative value on failed.
 */
static int drv_xcvr_get_eth_eeprom_size(void *xcvr, unsigned int eth_index)
{
    return 0x8180;
}

/*
 * drv_xcvr_read_eth_eeprom_data - Used to read port eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read port eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_read_eth_eeprom_data(void *xcvr, unsigned int eth_index, char *buf, loff_t offset,
                   size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);

   return clx_fpga_sfp_read(sfp, eth_index, buf, offset, count);
}

/*
 * drv_xcvr_write_eth_eeprom_data - Used to write port eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write port eeprom data
 * @count: length of buf
 *
 * This function returns the written length of port eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_xcvr_write_eth_eeprom_data(void *xcvr, unsigned int eth_index, char *buf, loff_t offset,
                   size_t count)
{
    struct clounix_priv_data *sfp = &(((struct drv_xcvr_fpga *)xcvr)->dev);

    return clx_fpga_sfp_write(sfp, eth_index, buf, offset, count);
}

static void drv_xcvr_fpga_init_port(struct clounix_priv_data *priv)
{
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "fpga_init_port:%p .\r\n", priv->mmio);
#if 0
    fpga_reg_write(priv, FPGA_MGR_RST, FPGA_PORT_MGR0_CFG);
    fpga_reg_write(priv, FPGA_MGR_RST, FPGA_PORT_MGR1_CFG);
    fpga_reg_write(priv, FPGA_MGR_RST, FPGA_PORT_MGR2_CFG);
#endif
    return;
}
static int drv_xcvr_dev_init(void *xcvr)
{
    int eth_idx;
    struct drv_xcvr_fpga *driver = (struct drv_xcvr_fpga *)xcvr;
    u8 i2c_dev_idx,cpld_idx;
    u8 platform_type;

    if (clounix_fpga_base == NULL) {
        LOG_ERR(CLX_DRIVER_TYPES_XCVR, "xcvr resource is not available.\r\n");
        return DRIVER_ERR;
    }
    mutex_init(&(driver->dev.lock));
    driver->xcvr_base = clounix_fpga_base + XCVR_BASE_ADDRESS;
    driver->dev.mmio = driver->xcvr_base;
    LOG_ERR(CLX_DRIVER_TYPES_XCVR, "clx_driver_xcvr_dev_init:%p :base:%p.\r\n", driver->dev.mmio, driver->xcvr_base);
    drv_xcvr_fpga_init_port(&driver->dev);
    platform_type = drv_xcvr_get_platform_idx(driver->xcvr_if.port_max);
    if (platform_type == DRIVER_ERR) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "faild to get platform type ethernet eth:%d.\r\n", driver->xcvr_if.port_max);
            return DRIVER_ERR;
    }
    LOG_DBG(CLX_DRIVER_TYPES_XCVR, "dev index ethernet max:%d, platform:%d.\r\n", driver->xcvr_if.port_max, platform_type);
    driver->dev.platform_type = platform_type;

    for (eth_idx = 0; eth_idx < driver->xcvr_if.port_max; eth_idx++) {
        if(PORT_SFP == get_sfp_porttype(eth_idx, platform_type)) {
            driver->dev.chip[eth_idx].dev_class  = TWO_ADDR;
            driver->dev.chip[eth_idx].byte_len   = TWO_ADDR_EEPROM_SIZE; 
        }
        else {
            driver->dev.chip[eth_idx].dev_class  = CMIS_ADDR;
            driver->dev.chip[eth_idx].byte_len   = ONE_ADDR_EEPROM_SIZE;
        }
        driver->dev.chip[eth_idx].write_max  = I2C_SMBUS_BLOCK_MAX;
        driver->dev.chip[eth_idx].slave_addr = SFP_EEPROM_A0_ADDR;
        driver->dev.chip[eth_idx].clk_div = driver->xcvr_if.clk_div;
        drv_xcvr_set_eth_power_on_status(driver, eth_idx, XCVRD_POWR_ON);
        
        i2c_dev_idx = drv_xcvr_get_i2c_idx(platform_type, eth_idx);
        if (i2c_dev_idx == DRIVER_ERR) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "faild to get i2c dev index ethernet max:%d eth:%d.\r\n", driver->xcvr_if.port_max, eth_idx);
            return DRIVER_ERR;
        }
        cpld_idx = drv_xcvr_get_cpld_idx(platform_type, eth_idx);
        if (cpld_idx == DRIVER_ERR) {
            LOG_ERR(CLX_DRIVER_TYPES_XCVR, "faild to get cpld  index ethernet max:%d eth:%d.\r\n", driver->xcvr_if.port_max, eth_idx);
            return DRIVER_ERR;
        }
       
        driver->dev.chip[eth_idx].dev_idx = i2c_dev_idx;
        driver->dev.chip[eth_idx].cpld_idx = cpld_idx;
    }

    return DRIVER_OK;
}

int drv_xcvr_fpga_init(void **xcvr_driver)
{
     struct drv_xcvr_fpga *xcvr = &drv_xcvr;

     xcvr->xcvr_if.dev_init = drv_xcvr_dev_init;
     xcvr->xcvr_if.get_eth_number = drv_xcvr_get_eth_number;
     xcvr->xcvr_if.get_transceiver_power_on_status = drv_xcvr_get_transceiver_power_on_status;
     xcvr->xcvr_if.set_transceiver_power_on_status = drv_xcvr_set_transceiver_power_on_status;
     xcvr->xcvr_if.get_transceiver_presence_status = drv_xcvr_get_transceiver_presence_status;
     xcvr->xcvr_if.get_eth_power_on_status = drv_xcvr_get_eth_power_on_status;
     xcvr->xcvr_if.set_eth_power_on_status = drv_xcvr_set_eth_power_on_status;
     xcvr->xcvr_if.get_eth_tx_fault_status = drv_xcvr_get_eth_tx_fault_status;
     xcvr->xcvr_if.get_eth_tx_disable_status = drv_xcvr_get_eth_tx_disable_status;
     xcvr->xcvr_if.set_eth_tx_disable_status = drv_xcvr_set_eth_tx_disable_status;
     xcvr->xcvr_if.get_eth_present_status = drv_xcvr_get_eth_present_status;
     xcvr->xcvr_if.get_eth_rx_los_status = drv_xcvr_get_eth_rx_los_status;
     xcvr->xcvr_if.get_eth_reset_status = drv_xcvr_get_eth_reset_status;
     xcvr->xcvr_if.set_eth_reset_status = drv_xcvr_set_eth_reset_status;
     xcvr->xcvr_if.get_eth_low_power_mode_status = drv_xcvr_get_eth_low_power_mode_status;
     xcvr->xcvr_if.set_eth_low_power_mode_status = drv_xcvr_set_eth_low_power_mode_status;
     xcvr->xcvr_if.get_eth_interrupt_status = drv_xcvr_get_eth_interrupt_status;
     xcvr->xcvr_if.get_eth_eeprom_size = drv_xcvr_get_eth_eeprom_size;
     xcvr->xcvr_if.read_eth_eeprom_data = drv_xcvr_read_eth_eeprom_data;
     xcvr->xcvr_if.write_eth_eeprom_data = drv_xcvr_write_eth_eeprom_data;
     *xcvr_driver = xcvr;
     LOG_INFO(CLX_DRIVER_TYPES_XCVR, "XCVR driver initialization done.\r\n");

    return DRIVER_OK;
}
//drv_xcvr_driver_define_initcall(drv_drv_xcvr_fpga_xcvr_init);

