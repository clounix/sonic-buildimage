#include <linux/io.h>

#include "watchdog_driver_clx8000.h"
#include "clx_driver.h"

//external function declaration
extern void __iomem *clounix_fpga_base;

//internal function declaration
struct watchdog_driver_clx8000 driver_watchdog_clx8000;

/*
 * clx_get_watchdog_identify - Used to get watchdog identify, such as iTCO_wdt
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_identify(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
}

/*
 * clx_get_watchdog_state - Used to get watchdog state,
 * filled the value to buf, 0: inactive, 1: active
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_state(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
}

/*
 * clx_get_watchdog_timeleft - Used to get watchdog timeleft,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_timeleft(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
}

/*
 * clx_get_watchdog_timeout - Used to get watchdog timeout,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_timeout(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
}

/*
 * clx_set_watchdog_timeout - Used to set watchdog timeout,
 * @value: timeout value
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_watchdog_timeout(void *driver, int value)
{
    /* add vendor codes here */
    return -1;
}

/*
 * clx_get_watchdog_enable_status - Used to get watchdog enable status,
 * filled the value to buf, 0: disable, 1: enable
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_enable_status(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    return -1;
}

/*
 * clx_set_watchdog_enable_status - Used to set watchdog enable status,
 * @value: enable status value, 0: disable, 1: enable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_watchdog_enable_status(void *driver, int value)
{
    /* add vendor codes here */
    return -1;
}

static int clx_driver_clx8000_watchdog_dev_init(struct watchdog_driver_clx8000 *watchdog)
{
    if (clounix_fpga_base == NULL) {
        WDT_INFO("fpga resource is not available.\r\n");
        return -ENXIO;
    }
    watchdog->watchdog_base = clounix_fpga_base + WATCHDOG_BASE_ADDRESS;

    return DRIVER_OK;
}

//void clx_driver_clx8000_watchdog_init(struct watchdog_driver_clx8000 **watchdog_driver)
void clx_driver_clx8000_watchdog_init(void **watchdog_driver)
{
    struct watchdog_driver_clx8000 *watchdog = &driver_watchdog_clx8000;

    WDT_INFO("clx_driver_clx8000_watchdog_init\n");
    clx_driver_clx8000_watchdog_dev_init(watchdog);
    watchdog->watchdog_if.get_watchdog_identify = clx_driver_clx8000_get_watchdog_identify;
    watchdog->watchdog_if.get_watchdog_state = clx_driver_clx8000_get_watchdog_state;
    watchdog->watchdog_if.get_watchdog_timeleft = clx_driver_clx8000_get_watchdog_timeleft;
    watchdog->watchdog_if.get_watchdog_timeout = clx_driver_clx8000_get_watchdog_timeout;
    watchdog->watchdog_if.set_watchdog_timeout = clx_driver_clx8000_set_watchdog_timeout;
    watchdog->watchdog_if.get_watchdog_enable_status = clx_driver_clx8000_get_watchdog_enable_status;
    watchdog->watchdog_if.set_watchdog_enable_status = clx_driver_clx8000_set_watchdog_enable_status;
    *watchdog_driver = watchdog;
    WDT_INFO("WATCHDOG driver clx8000 initialization done.\r\n");
}
//clx_driver_define_initcall(clx_driver_clx8000_watchdog_init);

