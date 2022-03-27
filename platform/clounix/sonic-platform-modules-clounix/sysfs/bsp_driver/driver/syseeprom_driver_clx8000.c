#include <linux/io.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "syseeprom_driver_clx8000.h"
#include "clx_driver_common.h"
#include "clx_driver.h"

//external function declaration
extern int32_t clx_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_mux_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_mux_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

//internal function declaration
struct syseeprom_driver_clx8000 driver_syseeprom_clx8000;

static ssize_t clx_driver_clx8000_write_syseeprom(struct syseeprom_driver_clx8000 *driver, char *buf, loff_t offset, size_t count)
{
    char *dummy = buf;    
    //select IDROM channel
    clx_i2c_mux_write(driver->bus, driver->mux_addr, driver->mux_channel, dummy, 1);
    return clx_i2c_write(driver->bus, driver->addr, offset, buf, count);
}

static ssize_t clx_driver_clx8000_read_syseeprom(struct syseeprom_driver_clx8000 *driver, char *buf, loff_t offset, size_t count)
{
    char *dummy = buf;
    //select IDROM channel
    clx_i2c_mux_write(driver->bus, driver->mux_addr, driver->mux_channel, dummy, 1);
    return clx_i2c_read(driver->bus, driver->addr, offset, buf, count);  
}
/*****************************************syseeprom*******************************************/
/*
 * clx_get_syseeprom_size - Used to get syseeprom size
 *
 * This function returns the size of syseeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_get_syseeprom_size(void *driver)
{
    /* add vendor codes here */
    return 256;
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
static ssize_t clx_driver_clx8000_read_syseeprom_data(void *driver, char *buf, loff_t offset, size_t count)
{
    struct syseeprom_driver_clx8000 *syseeprom = (struct syseeprom_driver_clx8000 *)driver;
    SYSE2_DBG("read bus:%d addr:0x%x offset:0x%x, count:%d",syseeprom->bus, syseeprom->addr, offset, count);
    return clx_driver_clx8000_read_syseeprom(syseeprom, buf, offset, count);
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
static ssize_t clx_driver_clx8000_write_syseeprom_data(void *driver, char *buf, loff_t offset, size_t count)
{
    struct syseeprom_driver_clx8000 *syseeprom = (struct syseeprom_driver_clx8000 *)driver;
    SYSE2_DBG("write bus:%d addr:0x%x offset:0x%x, count:%d",syseeprom->bus, syseeprom->addr, offset, count);
    return clx_driver_clx8000_write_syseeprom(syseeprom, buf, offset, count);
}

static int clx_driver_clx8000_syseeprom_dev_init(struct syseeprom_driver_clx8000 *syseeprom)
{
    return DRIVER_OK;
}
void clx_driver_clx8000_syseeprom_init(void **syseeprom_driver)
{
    struct syseeprom_driver_clx8000 *syseeprom = &driver_syseeprom_clx8000;

    SYSE2_INFO("clx_driver_clx8000_syseeprom_init\n");
    clx_driver_clx8000_syseeprom_dev_init(syseeprom);
    syseeprom->syseeprom_if.get_syseeprom_size = clx_driver_clx8000_get_syseeprom_size;
    syseeprom->syseeprom_if.read_syseeprom_data = clx_driver_clx8000_read_syseeprom_data;
    syseeprom->syseeprom_if.write_syseeprom_data = clx_driver_clx8000_write_syseeprom_data;
    syseeprom->bus = CLX_SYSEEPROM_BUS;
    syseeprom->addr = CLX_SYSEEPROM_ADDR;
    syseeprom->mux_addr = CLX_PCA9548_ADDR;
    syseeprom->mux_channel = CLX_PCA9548_CHANNEL_IDROM;
    *syseeprom_driver = syseeprom;
    SYSE2_INFO("SYSEEPROM driver clx8000 initialization done.\r\n");
}
//clx_driver_define_initcall(clx_driver_clx8000_syseeprom_init);

