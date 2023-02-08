#include <linux/io.h>

#include "drv_cpld_lattice.h"
#include "clx_driver.h"
#include "clounix/clounix_fpga.h"

//external function declaration
extern void __iomem *clounix_fpga_base;

//internal function declaration
struct drv_cpld_lattice driver_cpld_clx8000;

static int set_cpld_enable(struct drv_cpld_lattice *cpld, unsigned int cpld_index, unsigned int flag)
{
    uint32_t data = 0;
    int en_bit = 0;

    data = readl(cpld->cpld_base + CPLD_INTR_CFG_ADDR);
    LOG_DBG(CLX_DRIVER_TYPES_CPLD, " reg: %x, data: %x\r\n", CPLD_INTR_CFG_ADDR, data);
    if(0 == cpld_index) {
        en_bit = CPLD0_EN_BIT;
    }else{
        en_bit = CPLD1_EN_BIT;
    }
    if(flag)
        SET_BIT(data, en_bit);
    else
        CLEAR_BIT(data, en_bit);
    writel(data, cpld->cpld_base + CPLD_INTR_CFG_ADDR);
    return 0;
}

static int set_cpld_reset(struct drv_cpld_lattice *cpld,  unsigned int cpld_index, unsigned int flag)
{
    uint32_t data = 0;
    int rst_bit = 0;

    data = readl(cpld->cpld_base + CPLD_INTR_CFG_ADDR);
    LOG_DBG(CLX_DRIVER_TYPES_CPLD, " reg: %x, data: %x\r\n", CPLD_INTR_CFG_ADDR, data);
    if(0 == cpld_index) {
        rst_bit = CPLD0_RST_BIT;
    }else{
        rst_bit = CPLD1_RST_BIT;
    }
    if(flag)
        SET_BIT(data,rst_bit);
    else
        CLEAR_BIT(data, rst_bit);
    writel(data, cpld->cpld_base + CPLD_INTR_CFG_ADDR);
    return 0;
}


static int drv_cpld_get_main_board_cpld_number(void *cpld)
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
static ssize_t drv_cpld_get_main_board_cpld_alias(void *cpld, unsigned int cpld_index, char *buf, size_t count)
{
    size_t len = 0;

    len = (ssize_t)snprintf(buf, count, "CPLD-%d\n", cpld_index);
    LOG_DBG(CLX_DRIVER_TYPES_CPLD, "CPLD driver alia:%s", buf);

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
static ssize_t drv_cpld_get_main_board_cpld_type(void *cpld, unsigned int cpld_index, char *buf, size_t count)
{
    size_t len = 0;

    len = (ssize_t)snprintf(buf, count, "LCMXO2-1200HC-%d\n", cpld_index);
    LOG_DBG(CLX_DRIVER_TYPES_CPLD, "CPLD driver type:%s", buf);

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
static ssize_t drv_cpld_get_main_board_cpld_firmware_version(void *cpld, unsigned int cpld_index, char *buf, size_t count)
{
    uint16_t data = 0;
    struct drv_cpld_lattice *dev = (struct drv_cpld_lattice *)cpld;

    data = readl(dev->cpld_base + CPLD_VERSION_ADDR);
    //pega_print(DEBUG, " reg: %x, data: 0x%x\r\n", CPLD_VERSION_ADDR, data);
    LOG_DBG(CLX_DRIVER_TYPES_CPLD, "cpld_firmware_version: reg: %p, data: 0x%x, index:%d\r\n", (dev->cpld_base + CPLD_VERSION_ADDR), data, cpld_index);

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
static ssize_t drv_cpld_get_main_board_cpld_board_version(void *cpld, unsigned int cpld_index, char *buf, size_t count)
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
static ssize_t drv_cpld_get_main_board_cpld_test_reg(void *cpld, unsigned int cpld_index, char *buf, size_t count)
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
static int drv_cpld_set_main_board_cpld_test_reg(void *cpld, unsigned int cpld_index, unsigned int value)
{
    /* it is not supported */
    return DRIVER_OK;
}

static int drv_cpld_dev_init(struct drv_cpld_lattice *cpld)
{
    unsigned int data;

    if (clounix_fpga_base == NULL) {
        LOG_ERR(CLX_DRIVER_TYPES_CPLD, "fpga resource is not available.\r\n");
        return -ENXIO;
    }
    cpld->cpld_base = clounix_fpga_base + CPLD_BASE_ADDRESS;
    //enable CPLD interface
    data = 0x5000c34f;
    writel(data, cpld->cpld_base + CPLD_INTR_CFG_ADDR);
    data = readl(cpld->cpld_base + CPLD_INTR_CFG_ADDR);
    LOG_INFO(CLX_DRIVER_TYPES_CPLD, "CPLD interface config:0x%x.\r\n", data);
    #if 0
    for (cpld_index = 0; cpld_index < CPLD_CHIP_NUM; cpld_index++) {
        set_cpld_reset(cpld, cpld_index, 1);
        set_cpld_reset(cpld, cpld_index, 0);
        set_cpld_enable(cpld, cpld_index, 1);
    }
    #endif

    return DRIVER_OK;
}

int drv_cpld_lattice_init(void **cpld_driver)
{
    struct drv_cpld_lattice *cpld = &driver_cpld_clx8000;
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_CPLD, "clx_driver_cpld_init\n");
    ret = drv_cpld_dev_init(cpld);
    if (ret != DRIVER_OK)
        return ret;

    cpld->cpld_if.get_main_board_cpld_number = drv_cpld_get_main_board_cpld_number;
    cpld->cpld_if.get_main_board_cpld_alias = drv_cpld_get_main_board_cpld_alias;
    cpld->cpld_if.get_main_board_cpld_type = drv_cpld_get_main_board_cpld_type;
    cpld->cpld_if.get_main_board_cpld_firmware_version = drv_cpld_get_main_board_cpld_firmware_version;
    cpld->cpld_if.get_main_board_cpld_board_version = drv_cpld_get_main_board_cpld_board_version;
    cpld->cpld_if.get_main_board_cpld_test_reg = drv_cpld_get_main_board_cpld_test_reg;
    cpld->cpld_if.set_main_board_cpld_test_reg = drv_cpld_set_main_board_cpld_test_reg;
    *cpld_driver = cpld;
    LOG_INFO(CLX_DRIVER_TYPES_CPLD, "CPLD driver initialization done.\r\n");
    return DRIVER_OK;
}
//clx_driver_define_initcall(drv_cpld_init);

