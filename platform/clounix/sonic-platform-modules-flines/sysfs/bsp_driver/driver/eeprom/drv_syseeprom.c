#include <linux/io.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/jiffies.h>

#include "drv_syseeprom.h"
#include "drv_platform_common.h"
#include "clx_driver.h"

//internal function declaration
struct drv_syseeprom driver_syseeprom;

int32_t clx_ko_write_file(char *fpath, int32_t addr, uint8_t *val, int32_t write_bytes)
{
    int32_t ret;
    struct file *filp;
    loff_t pos;

    if ((fpath == NULL) || (val == NULL) || (addr < 0) || (write_bytes <= 0)) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "input arguments error, addr=%d write_bytes=%d\n", addr, write_bytes);
        return -1;
    }

    filp = filp_open(fpath, O_WRONLY, 0);
    if (IS_ERR(filp)) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "open file[%s] fail\n", fpath);
        return -1;
    }
    pos = addr;
    ret = kernel_write(filp, val, write_bytes, &pos);
    if (ret < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM,"kernel_write failed, path=%s, addr=%d, size=%d, ret=%d\n", fpath, addr, write_bytes, ret);
        ret = -1;
    }

    filp_close(filp, NULL);
    return ret;
}

int32_t clx_ko_read_file(char *fpath, int32_t addr, uint8_t *val, int32_t read_bytes)
{
    int32_t ret;
    struct file *filp;
    loff_t pos;

    if ((fpath == NULL) || (val == NULL) || (addr < 0) || (read_bytes < 0)) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "input arguments error, addr=%d read_bytes=%d\n", addr, read_bytes);
        return -1;
    }

    filp = filp_open(fpath, O_RDONLY, 0);
    if (IS_ERR(filp)){
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "open file[%s] fail\n", fpath);
        return -1;
    }
    pos = addr;
    ret = kernel_read(filp, val, read_bytes, &pos);
    if (ret < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "kernel_read failed, path=%s, addr=%d, size=%d, ret=%d\n", fpath, addr, read_bytes, ret);
        ret = -1;
    }
    filp_close(filp, NULL);
    return ret;
}

/*****************************************syseeprom*******************************************/
/*
 * clx_get_syseeprom_size - Used to get syseeprom size
 *
 * This function returns the size of syseeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int drv_get_syseeprom_size(void *driver)
{
    /* add vendor codes here */
	struct drv_syseeprom *syseeprom = (struct drv_syseeprom *)driver;
    return syseeprom->syseeprom_if.size;
}

/*
 * clx_read_syseeprom_data - Used to read syseeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read syseeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_read_syseeprom_data(void *driver, char *buf, loff_t offset, size_t count)
{
	ssize_t rd_len;
    char eeprom_path[256] = {0};
    struct drv_syseeprom *syseeprom = (struct drv_syseeprom *)driver;
	
    LOG_DBG(CLX_DRIVER_TYPES_SYSEEPROM, "read bus:%d addr:0x%x offset:0x%llx, count:%ld",syseeprom->syseeprom_if.bus, syseeprom->syseeprom_if.addr, offset, count);
	memset(buf, 0, count);
	sprintf(eeprom_path, "/sys/bus/i2c/devices/i2c-%d/%d-00%02x/eeprom",syseeprom->syseeprom_if.bus,syseeprom->syseeprom_if.bus, syseeprom->syseeprom_if.addr);
    rd_len = clx_ko_read_file(eeprom_path, offset, buf, count);
    if (rd_len < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "read eeprom data failed, loc: %s, offset: 0x%llx, \
        rd_count: %lu, ret: %ld,\n", eeprom_path, offset, count, rd_len);
    } else {
        LOG_DBG(CLX_DRIVER_TYPES_SYSEEPROM, "read eeprom data success, loc: %s, offset: 0x%llx, \
            rd_count: %lu, rd_len: %ld,\n", eeprom_path, offset, count, rd_len);
    }

    return rd_len;
}

/*
 * clx_write_syseeprom_data - Used to write syseeprom data
 * @buf: Data write buffer
 * @offset: offset address to write syseeprom data
 * @count: length of buf
 *
 * This function returns the written length of syseeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_write_syseeprom_data(void *driver, char *buf, loff_t offset, size_t count)
{
	ssize_t wr_len;
	char eeprom_path[256] = {0};
    struct drv_syseeprom *syseeprom = (struct drv_syseeprom *)driver;

    LOG_DBG(CLX_DRIVER_TYPES_SYSEEPROM, "write bus:%d addr:0x%x offset:0x%llx, count:%ld",syseeprom->syseeprom_if.bus, syseeprom->syseeprom_if.addr, offset, count);
	sprintf(eeprom_path, "/sys/bus/i2c/devices/i2c-%d/%d-00%02x/eeprom",syseeprom->syseeprom_if.bus,syseeprom->syseeprom_if.bus, syseeprom->syseeprom_if.addr);
	wr_len = clx_ko_write_file(eeprom_path, offset, buf, count);
    if (wr_len < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSEEPROM, "write eeprom data failed, loc:%s, offset: 0x%llx, \
            wr_count: %lu, ret: %ld.\n", eeprom_path, offset, count, wr_len);
    } else {
        LOG_DBG(CLX_DRIVER_TYPES_SYSEEPROM, "write eeprom data success, loc:%s, offset: 0x%llx, \
            wr_count: %lu, wr_len: %ld.\n", eeprom_path, offset, count, wr_len);
    }

	return wr_len;
}

int drv_syseeprom_init(void **syseeprom_driver)
{
    struct drv_syseeprom *syseeprom = &driver_syseeprom;
    LOG_INFO(CLX_DRIVER_TYPES_SYSEEPROM, "drv_syseeprom_init\n");
    syseeprom->syseeprom_if.get_syseeprom_size = drv_get_syseeprom_size;
    syseeprom->syseeprom_if.read_syseeprom_data = drv_read_syseeprom_data;
    syseeprom->syseeprom_if.write_syseeprom_data = drv_write_syseeprom_data;
    *syseeprom_driver = syseeprom;
    LOG_INFO(CLX_DRIVER_TYPES_SYSEEPROM, "SYSEEPROM driver clx12800 initialization done.\r\n");
    return DRIVER_OK;
}
//clx_driver_define_initcall(drv_syseeprom_init);

