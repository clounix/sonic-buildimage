#include <linux/io.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "drv_fpga_reboot_cause_eeprom.h"
#include "drv_platform_common.h"
#include "clx_driver.h"

// external function declaration
extern int32_t clx_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_mux_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_mux_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

// internal function declaration
struct reboot_eeprom_driver driver_reboot_eeprom;

static ssize_t drv_fpga_write_reboot_eeprom(struct reboot_eeprom_driver *driver, char *buf, loff_t offset, size_t count)
{
    return clx_i2c_write(driver->bus, driver->addr, offset, buf, count);
}

static ssize_t drv_fpga_read_reboot_eeprom(struct reboot_eeprom_driver *driver, char *buf, loff_t offset, size_t count)
{
    return clx_i2c_read(driver->bus, driver->addr, offset, buf, count);
}
/*****************************************reboot eeprom*******************************************/
/*
 * clx_get_reboot_eeprom_size - Used to get reboot eeprom size
 *
 * This function returns the size of reboot eeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int drv_fpga_get_reboot_eeprom_size(void *driver)
{
    /* add vendor codes here */
    return 8;
}

/*
 * clx_read_reboot_eeprom_data - Used to read reboot eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read reboot_eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_fpga_read_reboot_eeprom_data(void *driver, char *buf, loff_t offset, size_t count)
{
    struct reboot_eeprom_driver *reboot_eeprom = (struct reboot_eeprom_driver *)driver;
    LOG_DBG(CLX_DRIVER_TYPES_REBOOT_EEPROM,"read bus:%d addr:0x%x offset:0x%x, count:%d", reboot_eeprom->bus, reboot_eeprom->addr, offset, count);
    return drv_fpga_read_reboot_eeprom(reboot_eeprom, buf, offset, count);
}

/*
 * clx_write_reboot_eeprom_data - Used to write reboot_eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write reboot_eeprom data
 * @count: length of buf
 *
 * This function returns the written length of reboot_eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_fpga_write_reboot_eeprom_data(void *driver, char *buf, loff_t offset, size_t count)
{
    struct reboot_eeprom_driver *reboot_eeprom = (struct reboot_eeprom_driver *)driver;
    LOG_DBG(CLX_DRIVER_TYPES_REBOOT_EEPROM,"write bus:%d addr:0x%x offset:0x%x, count:%d", reboot_eeprom->bus, reboot_eeprom->addr, offset, count);
    return drv_fpga_write_reboot_eeprom(reboot_eeprom, buf, offset, count);
}

static int drv_fpga_reboot_eeprom_dev_init(struct reboot_eeprom_driver *reboot_eeprom)
{
    return DRIVER_OK;
}
int drv_fpga_reboot_eeprom_init(void **reboot_eeprom_driver)
{
    struct reboot_eeprom_driver *reboot_eeprom = &driver_reboot_eeprom;

    LOG_INFO(CLX_DRIVER_TYPES_REBOOT_EEPROM,"drv_fpga_reboot_eeprom_init\n");
    drv_fpga_reboot_eeprom_dev_init(reboot_eeprom);
    reboot_eeprom->reboot_eeprom_if.get_reboot_eeprom_size = drv_fpga_get_reboot_eeprom_size;
    reboot_eeprom->reboot_eeprom_if.read_reboot_eeprom_data = drv_fpga_read_reboot_eeprom_data;
    reboot_eeprom->reboot_eeprom_if.write_reboot_eeprom_data = drv_fpga_write_reboot_eeprom_data;
    reboot_eeprom->bus = CLX_REBOOT_EEPROM_BUS;
    reboot_eeprom->addr = CLX_REBOOT_EEPROM_ADDR;
    *reboot_eeprom_driver = reboot_eeprom;
    LOG_INFO(CLX_DRIVER_TYPES_REBOOT_EEPROM,"reboot eeprom driver clx8000 initialization done.\r\n");
}
// clx_driver_define_initcall(drv_fpga_reboot_eeprom_init);
