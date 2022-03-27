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

#include "xcvr_driver_clx8000.h"
#include "clx_driver.h"

//external function declaration
extern void __iomem *clounix_fpga_base;

//internal function declaration
struct xcvr_driver_clx8000 driver_xcvr_clx8000;
static unsigned int loglevel = LOG_INFO | LOG_WARNING | LOG_ERR ;
module_param(loglevel, uint, 0664);

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

static int fpga_i2c_byte_read(struct clounix_priv_data *sfp,
            int port,
            char *buf, unsigned int offset)
{
    uint32_t data;
    int i = 0;
    int cfg, ctrl, stat, mux;

    if (port >= PORT_REG_2) {
        cfg = FPGA_PORT_MGR2_CFG;
        ctrl = FPGA_PORT_MGR2_CTRL;
        stat = FPGA_PORT_MGR2_STAT;
        mux = FPGA_PORT_MGR2_MUX;
        port -= PORT_REG_2;
    } else if (port >= PORT_REG_1) {
        cfg = FPGA_PORT_MGR1_CFG;
        ctrl = FPGA_PORT_MGR1_CTRL;
        stat = FPGA_PORT_MGR1_STAT;
        mux = FPGA_PORT_MGR1_MUX;
        port -= PORT_REG_1;
    } else {
        cfg = FPGA_PORT_MGR0_CFG;
        ctrl = FPGA_PORT_MGR0_CTRL;
        stat = FPGA_PORT_MGR0_STAT;
        mux = FPGA_PORT_MGR0_MUX;
    }

    data = FPGA_MGR_RST;
    writel(data, sfp->mmio + cfg);

    data = FPGA_MGR_ENABLE;
    writel(data, sfp->mmio + cfg);

    data = port;
    writel(data, sfp->mmio + mux);

    data = FPGA_MGR_RD | ((offset & 0xFF) << 16);
    writel(data, sfp->mmio + ctrl);
    do {
        data = readl(sfp->mmio + stat);
        if (data & 0x80000000) {
            *buf = data & 0xFF;
            return 0;
        }
    }while(i++ < 1000);

    return -ENXIO;
}

static int fpga_i2c_byte_write(struct clounix_priv_data *sfp,
            int port,
            char *buf, unsigned int offset)
{
    uint32_t data;
    int i = 0;
    int cfg, ctrl, stat, mux;

    if (port >= PORT_REG_2) {
        cfg = FPGA_PORT_MGR2_CFG;
        ctrl = FPGA_PORT_MGR2_CTRL;
        stat = FPGA_PORT_MGR2_STAT;
        mux = FPGA_PORT_MGR2_MUX;
        port -= PORT_REG_2;
    } else if (port >= PORT_REG_1) {
        cfg = FPGA_PORT_MGR1_CFG;
        ctrl = FPGA_PORT_MGR1_CTRL;
        stat = FPGA_PORT_MGR1_STAT;
        mux = FPGA_PORT_MGR1_MUX;
        port -= PORT_REG_1;
    } else {
        cfg = FPGA_PORT_MGR0_CFG;
        ctrl = FPGA_PORT_MGR0_CTRL;
        stat = FPGA_PORT_MGR0_STAT;
        mux = FPGA_PORT_MGR0_MUX;
    }

    data = FPGA_MGR_RST;
    writel(data, sfp->mmio + cfg);

    data = FPGA_MGR_ENABLE;
    writel(data, sfp->mmio + cfg);

    data = port;
    writel(data, sfp->mmio + mux);

    data = FPGA_MGR_WT | ((offset & 0xFF) << 16) | (*buf & 0xFF);
    writel(data, sfp->mmio + ctrl);
    do {
        data = readl(sfp->mmio + stat);
        if (data & 0x80000000) {
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
        pega_print(DEBUG, "read register byte by byte %d, offset:0x%x, data :0x%x ret:%d\n",
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

        pega_print(DEBUG, "eeprom read %zu@%d --> %d (%ld)\n",
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
        pega_print(DEBUG, "Write register byte by byte %zu, offset:0x%x, data :0x%x ret:%d\n",
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

    pega_print(DEBUG,"off %lld  page:%d phy_offset:%lld, count:%ld\n",
        off, page, phy_offset, (long int) count);
    if (page > 0) {
        ret = clx_fpga_sfp_eeprom_write(sfp, eth_index, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            pega_print(WARNING,"Write page register for page %d failed ret:%d!\n",page, ret);
            return ret;
        }
    }
    wmb();

    ret =  clx_fpga_sfp_eeprom_read(sfp, eth_index,
            &page_check , SFP_PAGE_SELECT_REG, 1);
    if (ret < 0) {
            pega_print(WARNING,"Read page register for page %d failed ret:%d!\n",page, ret);
            return ret;
    }
    pega_print(DEBUG,"read page register %d checked %d, ret:%d\n",page, page_check, ret);

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
            pega_print(ERR,"Restore page register to 0 failed:%d!\n", ret);
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

    pega_print(DEBUG,"off %lld  page:%d phy_offset:%lld, count:%ld\n",
        off, page, phy_offset, (long int) count);
    if (page > 0) {
        ret = clx_fpga_sfp_eeprom_write(sfp, eth_index, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            pega_print(WARNING,"Write page register for page %d failed ret:%d!\n",page, ret);
            return ret;
        }
    }
    wmb();

    ret =  clx_fpga_sfp_eeprom_read(sfp, eth_index,
            &page_check , SFP_PAGE_SELECT_REG, 1);
    if (ret < 0) {
            pega_print(WARNING,"Read page register for page %d failed ret:%d!\n",page, ret);
            return ret;
    }
    pega_print(DEBUG,"read page register %d checked %d, ret:%d\n",page, page_check, ret);

    ret =  clx_fpga_sfp_eeprom_write(sfp, eth_index,
           buf, phy_offset, count);

    if (ret < 0) {
        retval = ret;
        pega_print(ERR,"write eeprom failed:offset:0x%llx bytes:%d ret:%d!\n", phy_offset, eth_index, ret);
    }

    if (page > 0) {
        /* return the page register to page 0 (why?) */
        page = 0;
        ret = clx_fpga_sfp_eeprom_write(sfp, eth_index, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            pega_print(ERR,"Restore page register to 0 failed:%d!\n", ret);
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
        pega_print(DEBUG,"page_legal, SFP, off %lld len %ld\n",
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
        pega_print(DEBUG,"Paging Register: 0x%x; not_pageable mask: 0x%x\n",
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
        pega_print(DEBUG,"page_legal, QSFP, off %lld len %ld\n",
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

static int clx_driver_clx8000_get_eth_number(void *xcvr)
{
    return XCVR_PORT_MAX;
}

/*
 * clx_driver_clx8000_get_transceiver_power_on_status - Used to get the whole machine port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_transceiver_power_on_status(void *xcvr, char *buf, size_t count)
{
    /*spf 0 -47  power on */
    size_t poweron_bit_map = 0xffffffffffffUL;
    uint32_t data = 0;
    uint32_t reg = QSFP_CONFIG_ADDRESS_BASE;
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

    /*sfp 48-56*/
    data = fpga_reg_read(sfp, reg);
    XCVR_DBG(" reg: %x, data: %x\r\n", reg, data);

    poweron_bit_map |=  ((data >> QSFP_CONFIG_POWER_EN_OFFSET) & 0xffUL) << QSFP_START_PORT;
    XCVR_DBG("poweron_bit_map:%lx\r\n", poweron_bit_map);

    return sprintf(buf, "0x%lx\n", poweron_bit_map);
}

/*
 * clx_driver_clx8000_set_transceiver_power_on_status - Used to set the whole machine port power on status,
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_transceiver_power_on_status(void *xcvr, int status)
{
    /* it does not need to supported?*/
    return -ENOSYS;
}

/*
 * clx_driver_clx8000_get_transceiver_presence_status - Used to get the whole machine port power on status,
 * filled the value to buf, 0: No presence, 1: Presence
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_transceiver_presence_status(void *xcvr, char *buf, size_t count)
{
    uint32_t data1 = 0, data2 = 0,data3 = 0;
    ssize_t present_bit_map = 0;
    uint32_t reg1 = DSFP_PRESENT_ADDRESS_BASE;
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

    /*sfp 0-29*/
    data1 = fpga_reg_read(sfp, DSFP_PRESENT_ADDRESS_BASE);
    XCVR_DBG(" reg: %x, data1: %x\r\n", DSFP_PRESENT_ADDRESS_BASE, data1);
    /*sfp 30-47*/
    data2 = fpga_reg_read(sfp, (DSFP_PRESENT_ADDRESS_BASE+0x10));
    XCVR_DBG(" reg: %x, data2: %x\r\n", DSFP_PRESENT_ADDRESS_BASE, data3);
     /*sfp 48-57*/
    data3 = fpga_reg_read(sfp, QSFP_STATUS_ADDRESS_BASE);
    XCVR_DBG(" reg: %x, data3: %x\r\n", QSFP_STATUS_ADDRESS_BASE, data3);

    present_bit_map |= (data1 & 0x3FFFFFFF) | ((data2 & 0x3FFFF) << 30)| ((data3 & 0xffUL) << QSFP_START_PORT);

    XCVR_DBG("present_bit_map:0x%lx\r\n", present_bit_map);

    return sprintf(buf, "0x%lx\n", present_bit_map);
}

/*
 * clx_driver_clx8000_get_eth_power_on_status - Used to get single port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_eth_power_on_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    /*sfp 0 -47  power on bit is not supported default setting power on*/
    size_t val = 0x1;
    uint32_t data = 0;
    uint32_t reg = QSFP_CONFIG_ADDRESS_BASE;
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);
    /*sfp 48-55*/
    if (eth_index >= QSFP_START_PORT)
    {
        data = fpga_reg_read(sfp, reg);
        XCVR_DBG(" eth：%d reg: %x, data: %x\r\n", eth_index, reg, data);
        GET_BIT((data >> QSFP_CONFIG_POWER_EN_OFFSET), (eth_index - QSFP_START_PORT), val);
    }

    return sprintf(buf, "%d\n", val);
}

/*
 * clx_driver_clx8000_set_eth_power_on_status - Used to set single port power on status,
 * @eth_index: start with 0
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_eth_power_on_status(void *xcvr, unsigned int eth_index, int status)
{
    uint32_t data = 0, val = 0;
    uint32_t reg = QSFP_CONFIG_ADDRESS_BASE;
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

    if (eth_index < QSFP_START_PORT)
        return DRIVER_OK;

    data = fpga_reg_read(sfp, reg);
    XCVR_DBG(" reg: %x, data: %x\r\n", reg, data);
    if(val)
        SET_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_POWER_EN_OFFSET));
    else
        CLEAR_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_POWER_EN_OFFSET));

    fpga_reg_write(sfp, reg, data);

    return DRIVER_OK;
}

/*
 * clx_driver_clx8000_get_eth_tx_fault_status - Used to get port tx_fault status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_eth_tx_fault_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    /* it is not supported */
    return -ENOSYS;
}

/*
 * clx_driver_clx8000_get_eth_tx_disable_status - Used to get port tx_disable status,
 * filled the value to buf, 0: tx_enable, 1: tx_disable
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_eth_tx_disable_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    /* it is not supported */
    return -ENOSYS;
}

/*
 * clx_driver_clx8000_set_eth_tx_disable_status - Used to set port tx_disable status,
 * @eth_index: start with 0
 * @status: tx_disable status, 0: tx_enable, 1: tx_disable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_eth_tx_disable_status(void *xcvr, unsigned int eth_index, int status)
{
    /* it is not supported */
    return -ENOSYS;
}

static int check_dsfp_port(unsigned int eth_index)
{
    if (eth_index < 48) {
        return true;
    }
    else {
        return false;
    }
}

static ssize_t get_dsfp_present(struct clounix_priv_data *sfp,
                unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0, reg;

    GET_DSFP_PRESENT_ADDRESS(eth_index, reg);
    data = fpga_reg_read(sfp, reg);
    XCVR_DBG("eth_index:%d, reg: %x, data: %x\r\n",eth_index, reg, data);
    if(eth_index >= 30)
        GET_BIT(data, (eth_index - 30), val);
    else
        GET_BIT(data, eth_index, val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_qsfp_present(struct clounix_priv_data *sfp,
                unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0;

    data = fpga_reg_read(sfp, QSFP_STATUS_ADDRESS_BASE);
    XCVR_DBG(" reg: %x, data: %x\r\n", QSFP_STATUS_ADDRESS_BASE, data);
    GET_BIT((data >> QSFP_STATUS_PRESENT_OFFSET), (eth_index - QSFP_START_PORT), val);

    return sprintf(buf, "%d\n", val);
}

/*
 * clx_driver_clx8000_get_eth_present_status - Used to get port present status,
 * filled the value to buf, 1: present, 0: absent
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_eth_present_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

    if (check_dsfp_port(eth_index)) {
        return get_dsfp_present(sfp, eth_index, buf, count);
    }
    else {
        return get_qsfp_present(sfp, eth_index, buf, count);
    }
}

/*
 * clx_driver_clx8000_get_eth_rx_los_status - Used to get port rx_los status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_eth_rx_los_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    /* it is not supported*/
    return -ENOSYS;
}
static ssize_t get_dsfp_reset(struct clounix_priv_data *sfp, unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0, reg;

    GET_DSFP_RST_ADDRESS(eth_index, reg);
    data = fpga_reg_read(sfp, reg);

    XCVR_DBG(" reg: %x, data: %x\r\n",  reg, data);
    if(eth_index >= 30)
        GET_BIT(data, (eth_index - 30), val);
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
    XCVR_DBG(" reg: %x, data: %x\r\n", reg, data);
    GET_BIT((data >> QSFP_CONFIG_RESET_OFFSET), (eth_index - QSFP_START_PORT), val);

    return sprintf(buf, "%d\n", !val);
}
/*
 * clx_driver_clx8000_get_eth_reset_status - Used to get port reset status,
 * filled the value to buf, 0: unreset, 1: reset
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_eth_reset_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

    if (check_dsfp_port(eth_index)) {
        return get_dsfp_reset(sfp, eth_index, buf, count);
    }
    else {
        return get_qsfp_reset(sfp, eth_index, buf, count);
    }
}

/*DSFP CPLD 0:reset  1:not reset. KWAI 0:not reset 1:reset*/
static ssize_t set_dsfp_reset(struct clounix_priv_data *sfp, unsigned int eth_index, int status)
{
    uint32_t data = 0, reg;

    GET_DSFP_RST_ADDRESS(eth_index, reg);
    data = fpga_reg_read(sfp, reg);
    XCVR_DBG(" reg: %x, data: %x\r\n", reg, data);
    if(0x1 == status)
    {
        if(eth_index >= 30)
            CLEAR_BIT(data, (eth_index - 30));
        else
            CLEAR_BIT(data, eth_index );
    }else
    {
        if(eth_index >= 30)
            SET_BIT(data, (eth_index - 30));
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
    pega_print(DEBUG, " reg: %x, data: %x\r\n", reg, data);
    if(status)
        CLEAR_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_RESET_OFFSET));
    else
        SET_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_RESET_OFFSET));
    fpga_reg_write(sfp, reg, data);
    return 0;
}
/*
 * clx_driver_clx8000_set_eth_reset_status - Used to set port reset status,
 * @eth_index: start with 0
 * @status: reset status, 0: unreset, 1: reset
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_eth_reset_status(void *xcvr, unsigned int eth_index, int status)
{
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

    if (check_dsfp_port(eth_index)) {
        return set_dsfp_reset(sfp, eth_index, status);
    }
    else {
        return set_qsfp_reset(sfp, eth_index, status);
    }
}

/*
 * clx_driver_clx8000_get_eth_low_power_mode_status - Used to get port low power mode status,
 * filled the value to buf, 0: high power mode, 1: low power mode
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_eth_low_power_mode_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    uint32_t data = 0, val = 0;
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

    data = fpga_reg_read(sfp, QSFP_CONFIG_ADDRESS_BASE);
    //pega_print(DEBUG, "reg: %x, data: %x\r\n", QSFP_CONFIG_ADDRESS_BASE, data);
    GET_BIT(data, (eth_index - QSFP_START_PORT + QSFP_CONFIG_POWER_MODE_OFFSET), val);
    return sprintf(buf, "0x%02x\n", !val);
}

/*
 * clx_driver_clx8000_get_eth_interrupt_status - Used to get port interruption status,
 * filled the value to buf, 0: no interruption, 1: interruption
 * @eth_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_eth_interrupt_status(void *xcvr, unsigned int eth_index, char *buf, size_t count)
{
    /* it is not supported */
    return -ENOSYS;
}

/*
 * clx_driver_clx8000_get_eth_eeprom_size - Used to get port eeprom size
 *
 * This function returns the size of port eeprom,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_get_eth_eeprom_size(void *xcvr, unsigned int eth_index)
{
    return 0x8180;
}

/*
 * clx_driver_clx8000_read_eth_eeprom_data - Used to read port eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read port eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_read_eth_eeprom_data(void *xcvr, unsigned int eth_index, char *buf, loff_t offset,
                   size_t count)
{
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

   return clx_fpga_sfp_read(sfp, eth_index, buf, offset, count);
}

/*
 * clx_driver_clx8000_write_eth_eeprom_data - Used to write port eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write port eeprom data
 * @count: length of buf
 *
 * This function returns the written length of port eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_write_eth_eeprom_data(void *xcvr, unsigned int eth_index, char *buf, loff_t offset,
                   size_t count)
{
    struct clounix_priv_data *sfp = &(((struct xcvr_driver_clx8000 *)xcvr)->dev);

    return clx_fpga_sfp_write(sfp, eth_index, buf, offset, count);
}

static void fpga_init_port(struct clounix_priv_data *priv)
{
    XCVR_DBG("fpga_init_port:%x .\r\n", priv->mmio);
#if 0
    fpga_reg_write(priv, FPGA_MGR_RST, FPGA_PORT_MGR0_CFG);
    fpga_reg_write(priv, FPGA_MGR_RST, FPGA_PORT_MGR1_CFG);
    fpga_reg_write(priv, FPGA_MGR_RST, FPGA_PORT_MGR2_CFG);
#endif
    return;
}
static int clx_driver_clx8000_xcvr_dev_init(struct xcvr_driver_clx8000 *xcvr)
{
    int i;

    if (clounix_fpga_base == NULL) {
        XCVR_ERR("xcvr resource is not available.\r\n");
        return -ENXIO;
    }
    mutex_init(&(xcvr->dev.lock));
    xcvr->xcvr_base = clounix_fpga_base + XCVR_BASE_ADDRESS;
    xcvr->dev.mmio = xcvr->xcvr_base;
    XCVR_ERR("clx_driver_clx8000_xcvr_dev_init:%x :base:%x.\r\n", xcvr->dev.mmio, xcvr->xcvr_base);
    fpga_init_port(&xcvr->dev);

    for (i = 0; i < XCVR_PORT_MAX; i++) {
        xcvr->dev.chip[i].dev_class  = CMIS_ADDR;
        xcvr->dev.chip[i].byte_len   = ONE_ADDR_EEPROM_SIZE;
        xcvr->dev.chip[i].write_max  = I2C_SMBUS_BLOCK_MAX;
        xcvr->dev.chip[i].slave_addr = SFP_EEPROM_A0_ADDR;
    }

    return DRIVER_OK;
}

//void clx_driver_clx8000_xcvr_init(struct xcvr_driver_clx8000 **xcvr_driver)
void clx_driver_clx8000_xcvr_init(void **xcvr_driver)
{
     struct xcvr_driver_clx8000 *xcvr = &driver_xcvr_clx8000;

     clx_driver_clx8000_xcvr_dev_init(xcvr);
     xcvr->xcvr_if.get_eth_number = clx_driver_clx8000_get_eth_number;
     xcvr->xcvr_if.get_transceiver_power_on_status = clx_driver_clx8000_get_transceiver_power_on_status;
     xcvr->xcvr_if.set_transceiver_power_on_status = clx_driver_clx8000_set_transceiver_power_on_status;
     xcvr->xcvr_if.get_transceiver_presence_status = clx_driver_clx8000_get_transceiver_presence_status;
     xcvr->xcvr_if.get_eth_power_on_status = clx_driver_clx8000_get_eth_power_on_status;
     xcvr->xcvr_if.set_eth_power_on_status = clx_driver_clx8000_set_eth_power_on_status;
     xcvr->xcvr_if.get_eth_tx_fault_status = clx_driver_clx8000_get_eth_tx_fault_status;
     xcvr->xcvr_if.get_eth_tx_disable_status = clx_driver_clx8000_get_eth_tx_disable_status;
     xcvr->xcvr_if.set_eth_tx_disable_status = clx_driver_clx8000_set_eth_tx_disable_status;
     xcvr->xcvr_if.get_eth_present_status = clx_driver_clx8000_get_eth_present_status;
     xcvr->xcvr_if.get_eth_rx_los_status = clx_driver_clx8000_get_eth_rx_los_status;
     xcvr->xcvr_if.get_eth_reset_status = clx_driver_clx8000_get_eth_reset_status;
     xcvr->xcvr_if.set_eth_reset_status = clx_driver_clx8000_set_eth_reset_status;
     xcvr->xcvr_if.get_eth_low_power_mode_status = clx_driver_clx8000_get_eth_low_power_mode_status;
     xcvr->xcvr_if.get_eth_interrupt_status = clx_driver_clx8000_get_eth_interrupt_status;
     xcvr->xcvr_if.get_eth_eeprom_size = clx_driver_clx8000_get_eth_eeprom_size;
     xcvr->xcvr_if.read_eth_eeprom_data = clx_driver_clx8000_read_eth_eeprom_data;
     xcvr->xcvr_if.write_eth_eeprom_data = clx_driver_clx8000_write_eth_eeprom_data;
     *xcvr_driver = xcvr;
     XCVR_INFO("XCVR driver clx8000 initialization done.\r\n");

}
//clx_driver_clx8000_driver_define_initcall(clx_driver_clx8000_driver_clx8000_xcvr_init);

