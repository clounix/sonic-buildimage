/*
 * transceiver_device_driver.c
 *
 * This module realize /sys/s3ip/transceiver attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "transceiver_sysfs.h"
#include "xcvr_interface.h"

static int g_loglevel = 0;

/****************************************transceiver******************************************/

static ssize_t clx_get_transceiver_loglevel(char *buf, size_t count)
{
    return sprintf(buf, "0x%x\n", g_dev_loglevel[CLX_DRIVER_TYPES_CPLD]);
}

static ssize_t clx_set_transceiver_loglevel(char *buf, size_t count)
{
    int loglevel = 0;

    if (kstrtouint(buf, 16, &loglevel))
    {
        return -EINVAL;
    }
    g_dev_loglevel[CLX_DRIVER_TYPES_XCVR] = loglevel;
    return count;
}

static ssize_t clx_get_transceiver_debug(char *buf, size_t count)
{
    return -ENOSYS;
}

static ssize_t clx_set_transceiver_debug(char *buf, size_t count)
{
    return -ENOSYS;
}
static int clx_get_eth_number(void)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_number);
    ret = xcvr_dev->get_eth_number(xcvr_dev);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_transceiver_power_on_status - Used to get the whole machine port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_transceiver_power_on_status(char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_transceiver_power_on_status);
    ret = xcvr_dev->get_transceiver_power_on_status(xcvr_dev, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_set_transceiver_power_on_status - Used to set the whole machine port power on status,
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_transceiver_power_on_status(int status)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->set_transceiver_power_on_status);
    ret = xcvr_dev->set_transceiver_power_on_status(xcvr_dev, status);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_transceiver_presence_status - Used to get the whole machine port presence status,
 * filled the value to buf, 0: No presence, 1: Presence
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_transceiver_presence_status(char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_transceiver_presence_status);
    ret = xcvr_dev->get_transceiver_presence_status(xcvr_dev, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_power_on_status - Used to get single port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_eth_power_on_status(unsigned int eth_index, char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_power_on_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_power_on_status(xcvr_dev, eth_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_set_eth_power_on_status - Used to set single port power on status,
 * @eth_index: start with 1
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_eth_power_on_status(unsigned int eth_index, int status)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->set_eth_power_on_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->set_eth_power_on_status(xcvr_dev, eth_index, status);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_tx_fault_status - Used to get port tx_fault status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_eth_tx_fault_status(unsigned int eth_index, char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_tx_fault_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_tx_fault_status(xcvr_dev, eth_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_tx_disable_status - Used to get port tx_disable status,
 * filled the value to buf, 0: tx_enable, 1: tx_disable
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_eth_tx_disable_status(unsigned int eth_index, char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_tx_disable_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_tx_disable_status(xcvr_dev, eth_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_set_eth_tx_disable_status - Used to set port tx_disable status,
 * @eth_index: start with 1
 * @status: tx_disable status, 0: tx_enable, 1: tx_disable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_eth_tx_disable_status(unsigned int eth_index, int status)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->set_eth_tx_disable_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->set_eth_tx_disable_status(xcvr_dev, eth_index, status);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_present_status - Used to get port present status,
 * filled the value to buf, 1: present, 0: absent
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_eth_present_status(unsigned int eth_index, char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_present_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_present_status(xcvr_dev, eth_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_rx_los_status - Used to get port rx_los status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_eth_rx_los_status(unsigned int eth_index, char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_rx_los_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_rx_los_status(xcvr_dev, eth_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_reset_status - Used to get port reset status,
 * filled the value to buf, 0: unreset, 1: reset
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_eth_reset_status(unsigned int eth_index, char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_reset_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_reset_status(xcvr_dev, eth_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_set_eth_reset_status - Used to set port reset status,
 * @eth_index: start with 1
 * @status: reset status, 0: unreset, 1: reset
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_set_eth_reset_status(unsigned int eth_index, int status)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->set_eth_reset_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->set_eth_reset_status(xcvr_dev, eth_index, status);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_low_power_mode_status - Used to get port low power mode status,
 * filled the value to buf, 0: high power mode, 1: low power mode
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_eth_low_power_mode_status(unsigned int eth_index, char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_low_power_mode_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_low_power_mode_status(xcvr_dev, eth_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_interrupt_status - Used to get port interruption status,
 * filled the value to buf, 0: no interruption, 1: interruption
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_get_eth_interrupt_status(unsigned int eth_index, char *buf, size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_interrupt_status);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_interrupt_status(xcvr_dev, eth_index, buf, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_get_eth_eeprom_size - Used to get port eeprom size
 *
 * This function returns the size of port eeprom,
 * otherwise it returns a negative value on failed.
 */
static int clx_get_eth_eeprom_size(unsigned int eth_index)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->get_eth_eeprom_size);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->get_eth_eeprom_size(xcvr_dev, eth_index);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_read_eth_eeprom_data - Used to read port eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read port eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_read_eth_eeprom_data(unsigned int eth_index, char *buf, loff_t offset,
                   size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->read_eth_eeprom_data);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->read_eth_eeprom_data(xcvr_dev, eth_index, buf, offset, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}

/*
 * clx_write_eth_eeprom_data - Used to write port eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write port eeprom data
 * @count: length of buf
 *
 * This function returns the written length of port eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_write_eth_eeprom_data(unsigned int eth_index, char *buf, loff_t offset,
                   size_t count)
{
    int ret;
    struct xcvr_fn_if *xcvr_dev = get_xcvr();

    XCVR_DEV_VALID(xcvr_dev);
    XCVR_DEV_VALID(xcvr_dev->write_eth_eeprom_data);
    XCVR_ETH_INDEX_MAPPING(eth_index);
    ret = xcvr_dev->write_eth_eeprom_data(xcvr_dev, eth_index, buf, offset, count);
    if (ret < 0)
    {
        return -ENOSYS;
    }

    return ret;
}
/************************************end of transceiver***************************************/

static struct s3ip_sysfs_transceiver_drivers_s drivers = {
    /*
     * set ODM transceiver drivers to /sys/s3ip/transceiver,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_eth_number = clx_get_eth_number,
    .get_loglevel = clx_get_transceiver_loglevel,
    .set_loglevel = clx_set_transceiver_loglevel,
    .get_debug = clx_get_transceiver_debug,
    .set_debug = clx_set_transceiver_debug,
    .get_transceiver_power_on_status = clx_get_transceiver_power_on_status,
    .set_transceiver_power_on_status = clx_set_transceiver_power_on_status,
    .get_transceiver_presence_status = clx_get_transceiver_presence_status,
    .get_eth_power_on_status = clx_get_eth_power_on_status,
    .set_eth_power_on_status = clx_set_eth_power_on_status,
    .get_eth_tx_fault_status = clx_get_eth_tx_fault_status,
    .get_eth_tx_disable_status = clx_get_eth_tx_disable_status,
    .set_eth_tx_disable_status = clx_set_eth_tx_disable_status,
    .get_eth_present_status = clx_get_eth_present_status,
    .get_eth_rx_los_status = clx_get_eth_rx_los_status,
    .get_eth_reset_status = clx_get_eth_reset_status,
    .set_eth_reset_status = clx_set_eth_reset_status,
    .get_eth_low_power_mode_status = clx_get_eth_low_power_mode_status,
    .get_eth_interrupt_status = clx_get_eth_interrupt_status,
    .get_eth_eeprom_size = clx_get_eth_eeprom_size,
    .read_eth_eeprom_data = clx_read_eth_eeprom_data,
    .write_eth_eeprom_data = clx_write_eth_eeprom_data,
};

static int __init sff_dev_drv_init(void)
{
    int ret;

    XCVR_INFO("sff_init...\n");
    xcvr_if_create_driver();

    ret = s3ip_sysfs_sff_drivers_register(&drivers);
    if (ret < 0) {
        XCVR_ERR("transceiver drivers register err, ret %d.\n", ret);
        return ret;
    }
    XCVR_INFO("sff_init success.\n");
    return 0;
}

static void __exit sff_dev_drv_exit(void)
{
    xcvr_if_delete_driver();
    s3ip_sysfs_sff_drivers_unregister();
    XCVR_INFO("sff_exit success.\n");
    return;
}

module_init(sff_dev_drv_init);
module_exit(sff_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("transceiver device driver");
