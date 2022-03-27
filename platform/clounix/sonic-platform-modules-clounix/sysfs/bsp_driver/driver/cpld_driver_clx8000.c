#include <linux/io.h>

#include "cpld_driver_clx8000.h"
#include "clx_driver.h"

//external function declaration
extern void __iomem *clounix_fpga_base;

//internal function declaration
struct cpld_driver_clx8000 driver_cpld_clx8000;

static int clx_driver_clx8000_get_main_board_cpld_number(void *cpld)
{
    return CPLD_CHIP_NUM;
}

/*
 * clx_get_main_board_cpld_alias - Used to identify the location of cpld,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_cpld_alias(void *cpld, unsigned int cpld_index, char *buf, size_t count)
{
    size_t len = 0;

    len = (ssize_t)snprintf(buf, count, "CPLD-%d\n", cpld_index);
    CPLD_DBG("CPLD driver alia:%s", buf);

    return len;
}

/*
 * clx_get_main_board_cpld_type - Used to get cpld model name
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_cpld_type(void *cpld, unsigned int cpld_index, char *buf, size_t count)
{
    size_t len = 0;

    len = (ssize_t)snprintf(buf, count, "LCMXO2-1200HC-%d\n", cpld_index);
    CPLD_DBG("CPLD driver type:%s", buf);

    return len;
}

/*
 * clx_get_main_board_cpld_firmware_version - Used to get cpld firmware version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_cpld_firmware_version(void *cpld, unsigned int cpld_index, char *buf, size_t count)
{
    uint16_t data = 0;
    struct cpld_driver_clx8000 *dev = (struct cpld_driver_clx8000 *)cpld;

    data = readl(dev->cpld_base + CPLD_VERSION_ADDR);
    //pega_print(DEBUG, " reg: %x, data: 0x%x\r\n", CPLD_VERSION_ADDR, data);
    CPLD_DBG("cpld_firmware_version: reg: %x, data: 0x%x, index:%d\r\n", (dev->cpld_base + CPLD_VERSION_ADDR), data, cpld_index);

    return sprintf(buf, "%02x\n", (data >> ((cpld_index - 1)* 8)) & 0xff );
}

/*
 * clx_get_main_board_cpld_board_version - Used to get cpld board version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_cpld_board_version(void *cpld, unsigned int cpld_index, char *buf, size_t count)
{
    /* it is not supported */
    return DRIVER_OK;
}

/*
 * clx_get_main_board_cpld_test_reg - Used to test cpld register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_cpld_test_reg(void *cpld, unsigned int cpld_index, char *buf, size_t count)
{
    /* it is not supported */
    return DRIVER_OK;
}

/*
 * clx_set_main_board_cpld_test_reg - Used to test cpld register write
 * @cpld_index: start with 1
 * @value: value write to cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_main_board_cpld_test_reg(void *cpld, unsigned int cpld_index, unsigned int value)
{
    /* it is not supported */
    return DRIVER_OK;
}

static int clx_driver_clx8000_cpld_dev_init(struct cpld_driver_clx8000 *cpld)
{
    if (clounix_fpga_base == NULL) {
        CPLD_ERR("fpga resource is not available.\r\n");
        return -ENXIO;
    }
    cpld->cpld_base = clounix_fpga_base + CPLD_BASE_ADDRESS;

    return DRIVER_OK;
}

//void clx_driver_clx8000_cpld_init(struct cpld_driver_clx8000 **cpld_driver)
void clx_driver_clx8000_cpld_init(void **cpld_driver)
{
    struct cpld_driver_clx8000 *cpld = &driver_cpld_clx8000;

    CPLD_INFO("clx_driver_clx8000_cpld_init\n");
    clx_driver_clx8000_cpld_dev_init(cpld);
    cpld->cpld_if.get_main_board_cpld_number = clx_driver_clx8000_get_main_board_cpld_number;
    cpld->cpld_if.get_main_board_cpld_alias = clx_driver_clx8000_get_main_board_cpld_alias;
    cpld->cpld_if.get_main_board_cpld_type = clx_driver_clx8000_get_main_board_cpld_type;
    cpld->cpld_if.get_main_board_cpld_firmware_version = clx_driver_clx8000_get_main_board_cpld_firmware_version;
    cpld->cpld_if.get_main_board_cpld_board_version = clx_driver_clx8000_get_main_board_cpld_board_version;
    cpld->cpld_if.get_main_board_cpld_test_reg = clx_driver_clx8000_get_main_board_cpld_test_reg;
    cpld->cpld_if.set_main_board_cpld_test_reg = clx_driver_clx8000_set_main_board_cpld_test_reg;
    *cpld_driver = cpld;
    printk(KERN_INFO "CPLD driver clx8000 initialization done.\r\n");
}
//clx_driver_define_initcall(clx_driver_clx8000_cpld_init);

