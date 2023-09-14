#include <linux/io.h>

#include "drv_sysled_fpga.h"
#include "clx_driver.h"
#include "clounix/io_signal_ctrl.h"

//external function declaration
extern void __iomem *clounix_fpga_base;

//internal function declaration
struct drv_sysled_fpga driver_sysled_clx8000;

enum user_sysled_state {
    USER_SYSLED_DARK,
    USER_SYSLED_GREEN,
    USER_SYSLED_YELLOW,
    USER_SYSLED_RED,
    USER_SYSLED_BLUE,
    USER_SYSLED_GREEN_BLINKING,
    USER_SYSLED_YELLOW_BLINKING,
    USER_SYSLED_RED_BLINKING,
    USER_SYSLED_BLUE_BLINKING,
    USER_SYSLED_NOT_SUPPORT
};

/*
 * clx_get_sys_led_status - Used to get sys led status
 * filled the value to buf, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */

static void led_color_set(char color, char *green, char *red, char *blue)
{
    if (green != NULL && red != NULL) {
        switch (color)
        {
            case USER_SYSLED_DARK:
                *green = 0;
                *red = 0;
                break;

            case USER_SYSLED_GREEN:
                *green = 1;
                *red = 0;
                break;

            case USER_SYSLED_YELLOW:
                *green = 1;
                *red = 1;
                break;

            case USER_SYSLED_RED:
                *green = 0;
                *red = 1;
                break;

                break;
        }
    } else if (blue != NULL) {
        *blue = (color == USER_SYSLED_BLUE) ? 1 : 0;
    }

    return;
}

static int led_color_get(char green, char red, char blue)
{
    char color;

    if (green == 1 && red == 1)
        color = USER_SYSLED_YELLOW;
    else if (green == 1)
        color = USER_SYSLED_GREEN;
    else if (red == 1)
        color = USER_SYSLED_RED;
    else
        color = USER_SYSLED_DARK;

    return color;
}

static ssize_t drv_get_sys_led_status(void *driver, char *buf, size_t count)
{
    unsigned char green;
    unsigned char red;
    char color;

    green = read_io_sig_desc(SYS_LED_G, 0);
    red = read_io_sig_desc(SYS_LED_R, 0);

    color = led_color_get(green, red, 0);

    return sprintf(buf, "%d\n", color);
}

/*
 * clx_set_sys_led_status - Used to set sys led status
 * @status: led status, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_set_sys_led_status(void *driver, int status)
{
    unsigned char green;
    unsigned char red;

    led_color_set(status, &green, &red, NULL);

    write_io_sig_desc(SYS_LED_G, 0, green);
    write_io_sig_desc(SYS_LED_R, 0, red);

    return 1;
}

/* Similar to clx_get_sys_led_status */
static ssize_t drv_get_bmc_led_status(void *driver, char *buf, size_t count)
{
    return -ENOSYS;
}

/* Similar to clx_set_sys_led_status */
static int drv_set_bmc_led_status(void *driver, int status)
{
    return -ENOSYS;
}

/* Similar to clx_get_sys_led_status */
static ssize_t drv_get_sys_fan_led_status(void *driver, char *buf, size_t count)
{
    unsigned char green;
    unsigned char red;
    char color;

    green = read_io_sig_desc(FAN_LED_G, 0);
    red = read_io_sig_desc(FAN_LED_R, 0);

    color = led_color_get(green, red, 0);

    return sprintf(buf, "%d\n", color);
}

/* Similar to clx_set_sys_led_status */
static int drv_set_sys_fan_led_status(void *driver, int status)
{
    unsigned char green = 0;
    unsigned char red = 0;

    led_color_set(status, &green, &red, NULL);

    write_io_sig_desc(FAN_LED_G, 0, green);
    write_io_sig_desc(FAN_LED_R, 0, red);

    return 1;
}

/* Similar to clx_get_sys_led_status */
static ssize_t drv_get_sys_psu_led_status(void *driver, char *buf, size_t count)
{
    unsigned char green;
    unsigned char red;
    char color;

    green = read_io_sig_desc(PSU_LED_G, 0);
    red = read_io_sig_desc(PSU_LED_R, 0);

    color = led_color_get(green, red, 0);

    return sprintf(buf, "%d\n", color);
}

/* Similar to clx_set_sys_led_status */
static int drv_set_sys_psu_led_status(void *driver, int status)
{
    unsigned char green = 0;
    unsigned char red = 0;

    led_color_set(status, &green, &red, NULL);

    write_io_sig_desc(PSU_LED_G, 0, green);
    write_io_sig_desc(PSU_LED_R, 0, red);

    return 1;
}

/* Similar to clx_get_sys_led_status */
static ssize_t drv_get_id_led_status(void *driver, char *buf, size_t count)
{
    unsigned char status;

    status = read_io_sig_desc(ID_LED_B, 0);
    status = status > 0 ? 4 : 0;

    return sprintf(buf, "%d\n", status);
}

/* Similar to clx_set_sys_led_status */
static int drv_set_id_led_status(void *driver, int status)
{
    status = status > 0 ? 1 : 0;

    return write_io_sig_desc(ID_LED_B, 0, status);
}

static int drv_sysled_dev_init(struct drv_sysled_fpga *sysled)
{
    if (clounix_fpga_base == NULL) {
        LOG_ERR(CLX_DRIVER_TYPES_SYSLED, "fpga resource is not available.\r\n");
        return -ENXIO;
    }
    sysled->sysled_base = clounix_fpga_base + LED_MANAGER_OFFSET;

    return DRIVER_OK;
}

int drv_sysled_init(void **sysled_driver)
{
    struct drv_sysled_fpga *sysled = &driver_sysled_clx8000;
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_SYSLED, "clx_driver_sysled_init\n");
    ret = drv_sysled_dev_init(sysled);
    if (ret != DRIVER_OK)
        return ret;

    sysled->sysled_if.get_sys_led_status = drv_get_sys_led_status;
    sysled->sysled_if.set_sys_led_status = drv_set_sys_led_status;
    sysled->sysled_if.get_bmc_led_status = drv_get_bmc_led_status;
    sysled->sysled_if.set_bmc_led_status = drv_set_bmc_led_status;
    sysled->sysled_if.get_sys_fan_led_status = drv_get_sys_fan_led_status;
    sysled->sysled_if.set_sys_fan_led_status = drv_set_sys_fan_led_status;
    sysled->sysled_if.get_sys_psu_led_status = drv_get_sys_psu_led_status;
    sysled->sysled_if.set_sys_psu_led_status = drv_set_sys_psu_led_status;
    sysled->sysled_if.get_id_led_status = drv_get_id_led_status;
    sysled->sysled_if.set_id_led_status = drv_set_id_led_status;
    *sysled_driver = sysled;
    LOG_INFO(CLX_DRIVER_TYPES_SYSLED, "SYSLED driver clx8000 initialization done.\r\n");
    return DRIVER_OK;
}
//clx_driver_define_initcall(drv_sysled_init);

