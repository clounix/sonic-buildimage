#include <linux/io.h>

#include "fpga_driver_clx8000.h"
#include "clx_driver.h"

//external function declaration
extern void __iomem *clounix_fpga_base;

//internal function declaration
struct fpga_driver_clx8000 driver_fpga_clx8000;

static int clx_driver_clx8000_get_main_board_fpga_number(void *fpga)
{
    return FPGA_CHIP_NUM;
}

/*
 * clx_get_main_board_fpga_alias - Used to identify the location of fpga,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_fpga_alias(void *fpga, unsigned int fpga_index, char *buf, size_t count)
{
    size_t len = 0;

    len = (ssize_t)snprintf(buf, count, "FPGA-%d\n", fpga_index);
    FPGA_DBG("FPGA driver type:%s", buf);

    return len;
}

/*
 * clx_get_main_board_fpga_type - Used to get fpga model name
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_fpga_type(void *fpga, unsigned int fpga_index, char *buf, size_t count)
{
    size_t len = 0;

    len = (ssize_t)snprintf(buf, count, "Xilinx artix-7:xc7a75tfgg484-2\n");
    FPGA_DBG("FPGA driver type:%s", buf);

    return len;
}

/*
 * clx_get_main_board_fpga_firmware_version - Used to get fpga firmware version,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_fpga_firmware_version(void *fpga, unsigned int fpga_index, char *buf, size_t count)
{
    uint16_t data = 0;
    struct fpga_driver_clx8000 *dev = (struct fpga_driver_clx8000 *)fpga;

    data = readl(dev->fpga_base + FPGA_FW_VERSION_ADDR);
    //pega_print(DEBUG, " reg: %x, data: 0x%x\r\n", FPGA_FW_VERSION_ADDR, data);

    return (ssize_t)sprintf(buf, "%02x\n", (data & 0xf ));
}

/*
 * clx_get_main_board_fpga_board_version - Used to get fpga board version,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_fpga_board_version(void *fpga, unsigned int fpga_index, char *buf, size_t count)
{
    uint16_t data = 0;
    struct fpga_driver_clx8000 *dev = (struct fpga_driver_clx8000 *)fpga;

    data = readl(dev->fpga_base + FPGA_HW_VERSION_ADDR);
    //pega_print(DEBUG, " reg: %x, data: 0x%x\r\n", FPGA_HW_VERSION_ADDR, data);

    return (ssize_t)sprintf(buf, "%02x\n", (data & 0xf ));
}

/*
 * clx_get_main_board_fpga_test_reg - Used to test fpga register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_main_board_fpga_test_reg(void *fpga, unsigned int fpga_index, char *buf, size_t count)
{
    /* it is not supported */
    return DRIVER_OK;
}

/*
 * clx_set_main_board_fpga_test_reg - Used to test fpga register write
 * @fpga_index: start with 1
 * @value: value write to fpga
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_main_board_fpga_test_reg(void *fpga, unsigned int fpga_index, unsigned int value)
{
    /* it is not supported */
    return DRIVER_OK;
}

static int clx_driver_clx8000_fpga_dev_init(struct fpga_driver_clx8000 *fpga)
{
    if (clounix_fpga_base == NULL) {
        FPGA_ERR("fpga resource is not available.\r\n");
        return -ENXIO;
    }
    fpga->fpga_base = clounix_fpga_base + FPGA_BASE_ADDRESS;

    return DRIVER_OK;
}

//void clx_driver_clx8000_fpga_init(struct fpga_driver_clx8000 **fpga_driver)
void clx_driver_clx8000_fpga_init(void **fpga_driver)
{
     struct fpga_driver_clx8000 *fpga = &driver_fpga_clx8000;

     clx_driver_clx8000_fpga_dev_init(fpga);
     fpga->fpga_if.get_main_board_fpga_number = clx_driver_clx8000_get_main_board_fpga_number;
     fpga->fpga_if.get_main_board_fpga_alias = clx_driver_clx8000_get_main_board_fpga_alias;
     fpga->fpga_if.get_main_board_fpga_type = clx_driver_clx8000_get_main_board_fpga_type;
     fpga->fpga_if.get_main_board_fpga_firmware_version = clx_driver_clx8000_get_main_board_fpga_firmware_version;
     fpga->fpga_if.get_main_board_fpga_board_version = clx_driver_clx8000_get_main_board_fpga_board_version;
     fpga->fpga_if.get_main_board_fpga_test_reg = clx_driver_clx8000_get_main_board_fpga_test_reg;
     fpga->fpga_if.set_main_board_fpga_test_reg = clx_driver_clx8000_set_main_board_fpga_test_reg;
     *fpga_driver = fpga;
     FPGA_INFO("FPGA driver clx8000 initialization done.\r\n");
}
//clx_driver_define_initcall(clx_driver_clx8000_fpga_init);

