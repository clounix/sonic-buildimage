#include <linux/io.h>

#include "drv_fan_clx.h"
#include "drv_platform_common.h"
#include "clounix/clounix_fpga.h"
#include "clx_driver.h"

// external function declaration
extern int32_t clx_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_write_word(int bus, int addr, int offset, uint16_t *buf);

extern void __iomem *clounix_fpga_base;

// internal function declaration
struct fan_driver_clx driver_fan_clx;

static u8 led_state_user_to_dev[] = {DEV_FAN_LED_DARK, DEV_FAN_LED_GREEN, DEV_FAN_LED_YELLOW, DEV_FAN_LED_RED,
                                     USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT};
static u8 led_state_dev_to_user[] = {USER_FAN_LED_DARK, USER_FAN_LED_GREEN, USER_FAN_LED_RED, USER_FAN_LED_YELLOW};

static int set_fan_eeprom_wp(void *fan, uint8_t enable)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 reg = FAN_EEPROM_WRITE_EN_OFFSET;

    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg, enable);
    clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &enable, 1);

    return DRIVER_OK;
}

static ssize_t get_fan_eeprom_wp(void *fan, char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 reg = FAN_EEPROM_WRITE_EN_OFFSET;
    uint8_t enable;

    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg, &enable, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg, enable);

    return sprintf(buf, "%d\n", enable);
}

static int drv_get_fan_number(void *fan)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    return dev->fan_if.fan_num;
}

static int drv_get_fan_motor_number(void *fan, unsigned int fan_index)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    return dev->fan_if.motor_per_fan;
}

static ssize_t drv_get_fan_vendor_name(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "clounix\n");
}
/*
 * clx_get_fan_model_name - Used to get fan model name,
 * @fan_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_model_name(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "DFTA0456B2UP209\n");
}

/*
 * clx_get_fan_serial_number - Used to get fan serial number,
 * @fan_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_serial_number(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "N/A\n");
}

/*
 * clx_get_fan_part_number - Used to get fan part number,
 * @fan_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_part_number(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "N/A\n");
}

/*
 * clx_get_fan_hardware_version - Used to get fan hardware version,
 * @fan_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_hardware_version(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 data = 0;
    u8 reg = FAN_CPLD_VERSION_OFFSET;

    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg, &data, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "fan hw version:addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg, data);

    return sprintf(buf, "%02x\n", data);
}

/*
 * clx_get_fan_status - Used to get fan status,
 * filled the value to buf, fan status define as below:
 * 0: ABSENT
 * 1: OK
 * 2: NOT OK
 *
 * @fan_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_status(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 data = 0;
    u8 present = 0;
    u8 reg = FAN_PRESENT_OFFSET;

    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg, &data, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg, data);
    GET_BIT(data, fan_index, present);

    return sprintf(buf, "0x%02x\n", !present);
}

static u8 fan_led_get(struct fan_driver_clx *dev, unsigned char fan_index)
{
    u8 data1 = 0, data2 = 0;
    u8 reg1 = FAN_LED1_CONTROL_OFFSET, reg2 = FAN_LED2_CONTROL_OFFSET;
    u8 val1 = 0, val2 = 0;
    u8 dev_led_state = 0;

    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg1, &data1, 1);
    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg2, &data2, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg1: %x, data1: %x reg2:%x data3:%x\r\n", dev->fan_if.addr, reg1, data1, reg2, data2);
    GET_BIT(data1, fan_index, val1);
    GET_BIT(data2, fan_index, val2);
    dev_led_state = ((val1 | (val2 << 1)) & 0x3);

    return led_state_dev_to_user[dev_led_state];
}

static u8 fan_led_set(struct fan_driver_clx *dev, unsigned char fan_index, unsigned char user_led_state)
{
    u8 data = 0;
    u8 reg[FAN_LED_REG_MAX] = {FAN_LED1_CONTROL_OFFSET, FAN_LED2_CONTROL_OFFSET};
    u8 val = 0;
    u8 dev_led_state = led_state_user_to_dev[user_led_state & 0x3];
    u8 i;

    for (i = 0; i < FAN_LED_REG_MAX; i++)
    {
        clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg[i], &data, 1);
        val = ((dev_led_state >> i) & 0x1);
        if (val)
            SET_BIT(data, fan_index);
        else
            CLEAR_BIT(data, fan_index);
        clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg[i], &data, 1);
    }
    return dev_led_state;
}
/*
 * clx_get_fan_led_status - Used to get fan led status
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
 * @fan_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_led_status(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    return sprintf(buf, "%d\n", fan_led_get(dev, fan_index));
}

/*
 * clx_set_fan_led_status - Used to set fan led status
 * @fan_index: start with 0
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
static int drv_set_fan_led_status(void *fan, unsigned int fan_index, int status)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    return fan_led_set(dev, fan_index, status);
}

static unsigned int fan_vmon_get(struct fan_driver_clx *dev, unsigned char fan_index)
{
    uint8_t data = 0, i = 0;
    uint8_t fan_index_to_hw[6] = {2, 5, 3, 4, 1, 0};

    do
    {
        clx_i2c_read(dev->fan_if.bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
        if (data != 0x00)
        {
            data = 0x00;
            clx_i2c_write(dev->fan_if.bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
        }
        else
        {
            break;
        }

        usleep_range(500, 1000);

    } while (i++ < 5);
    if (i >= 5)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "fan_vmon_get error BANK_SEL = %x\r\n", data);
        return DRIVER_ERR;
    }

    if (dev->fan_if.fan_num == 6)
    {
        fan_index_to_hw[0] = 2;
        fan_index_to_hw[1] = 5;
        fan_index_to_hw[2] = 3;
        fan_index_to_hw[3] = 4;
        fan_index_to_hw[4] = 1;
        fan_index_to_hw[5] = 0;
    }
    else
    {
        fan_index_to_hw[0] = 2;
        fan_index_to_hw[1] = 3;
        fan_index_to_hw[2] = 4;
        fan_index_to_hw[3] = 1;
        fan_index_to_hw[4] = 0;
        fan_index_to_hw[5] = 5;
    }

    clx_i2c_read(dev->fan_if.bus, FAN_VMON_CHIP_ADDR, (FAN_VMON_VIN_LVL_BASE_REG + fan_index_to_hw[fan_index]), &data, 1);
    // v = (ADC[0:7]*5mV+0.2V)*VRANGE_MULT*HW
    if (data != 0)
    {
        return (((data * 5) + 200) * 11);
    }
    else
    {
        return 0;
    }
}

static ssize_t drv_get_fan_vmon(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;

    return sprintf(buf, "%d\n", fan_vmon_get(dev, fan_index));
}

int drv_fan_vmon_init(void *fan)
{
    struct fan_fn_if *fan_driver = (struct fan_fn_if *)fan;
    uint8_t data = 0, i = 0;
    uint16_t value = 0;
#if 0
    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_VENDOR_ID_REG, &data, 1);
    if (data != 0x18)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error VENDOR_ID = %x\r\n", data);
        return DRIVER_ERR;
    }
#endif
    do
    {
        clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_STAT_REG, &data, 1);
        if (data & (6 << 0x01))
        {
            break;
        }
        usleep_range(500, 1000);
    } while (i++ < 5);
    if (i >= 5)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error STAT_REG = %x\r\n", data);
        return DRIVER_ERR;
    }

    value = 0xd601;
    clx_i2c_write_word(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &value);
    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
    if (data != 0x01)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error clx_i2c_write BANK_SEL_REG = %x\r\n", data);
        return DRIVER_ERR;
    }

    value = 0x8700;
    clx_i2c_write_word(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_MISC_REG, &value);
    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_MISC_REG, &data, 1);
    if (data != 0x00)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error clx_i2c_write MISC_REG = %x\r\n", data);
        return DRIVER_ERR;
    }

    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_VIN_CH_EN_REG, &data, 1);
    if (data != 0x3f)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error VIN_CH_EN = %x\r\n", data);
        return DRIVER_ERR;
    }

    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_VRANGE_MULT_REG, &data, 1);
    if (data != 0x00)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error VRANGE_MULT = %x\r\n", data);
        return DRIVER_ERR;
    }

    data = 0x00;
    clx_i2c_write(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
    if (data != 0x00)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error clx_i2c_write BANK_SEL = %x\r\n", data);
        return DRIVER_ERR;
    }

    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_OFF_STAT_REG, &data, 1);

    if (fan_driver->fan_num == 6)
    {
        if ((data & 0x3F) != 0x00)
        {
            LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error VMON_OFF_STAT = %x\r\n", data);
            return DRIVER_ERR;
        }
    }
    else if (fan_driver->fan_num == 5)
    {
        if ((data & 0x1F) != 0x00)
        {
            LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error VMON_OFF_STAT = %x\r\n", data);
            return DRIVER_ERR;
        }
    }
    else
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init fan_num error VMON_OFF_STAT = %x\r\n", data);
        return DRIVER_ERR;
    }

    clx_i2c_read(fan_driver->bus, fan_driver->addr, FAN_VMON_ACT_CPLD_REG, &data, 1);
    data = (data | 0x01);
    clx_i2c_write(fan_driver->bus, fan_driver->addr, FAN_VMON_ACT_CPLD_REG, &data, 1);
    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_STAT_REG, &data, 1);
    if (data != 0x7e)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_init error FAN_VMON_ACT_CPLD VMON_STAT = %x\r\n", data);
        return DRIVER_ERR;
    }

    return DRIVER_OK;
}

#if 0
void drv_fan_vmon_uninit(void *fan)
{
    struct fan_fn_if *fan_driver = (struct fan_fn_if *)fan;
    uint8_t data = 0;

    data = 0x01;
    clx_i2c_write(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
    if (data != 0x01)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_uninit error clx_i2c_write BANK_SEL = %x\r\n", data);
    }

    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_CTL_REG, &data, 1);
    data = (data | (3 << 0x01));
    clx_i2c_write(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_CTL_REG, &data, 1);

    data = 0x00;
    clx_i2c_write(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
    clx_i2c_read(fan_driver->bus, FAN_VMON_CHIP_ADDR, FAN_VMON_BANK_SEL_REG, &data, 1);
    if (data != 0x00)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "drv_fan_vmon_uninit error clx_i2c_write BANK_SEL = %x\r\n", data);
    }
}
#endif
/*
 * clx_get_fan_direction - Used to get fan air flow direction,
 * filled the value to buf, air flow direction define as below:
 * 0: F2B
 * 1: B2F
 *
 * @fan_index: start with 0
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_direction(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 data = 0;
    u8 val = 0;
    u8 reg = FAN_AIR_DIRECTION_OFFSET;

    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg, &data, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg, data);
    GET_BIT(data, fan_index, val);

    return sprintf(buf, "%d\n", val);
}

/*
 * clx_get_fan_motor_speed - Used to get fan motor speed
 * filled the value to buf
 * @fan_index: start with 0
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_motor_speed(void *fan, unsigned int fan_index, unsigned int motor_index,
                                       char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 data1 = 0, data2 = 0;
    u8 reg1, reg2;

    // is it possible to update cpld?
    if (fan_index < 6)
    {
        reg1 = FAN1_INNER_RPM_OFFSET + fan_index;
        reg2 = FAN1_OUTER_RPM_OFFSET + fan_index;
    }
    else
    {
        return sprintf(buf, "0");
    }
    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg1, &data1, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg1, data1);

    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg2, &data2, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg2, data2);

    return sprintf(buf, "%d\n", (data1 + data2) * 60);
}

/*
 * clx_get_fan_motor_speed_tolerance - Used to get fan motor speed tolerance
 * filled the value to buf
 * @fan_index: start with 0
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_motor_speed_tolerance(void *fan, unsigned int fan_index, unsigned int motor_index,
                                                 char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;

    return sprintf(buf, "%d\n", (dev->fan_if.fan_max_speed / 10));
}

/*
 * clx_get_fan_motor_speed_target - Used to get fan motor speed target
 * filled the value to buf
 * @fan_index: start with 0
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_motor_speed_target(void *fan, unsigned int fan_index, unsigned int motor_index,
                                              char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 data = 0;
    u8 reg = FAN_PWM_CONTROL_OFFSET;

    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg, &data, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg, data);

    return sprintf(buf, "%d\n", data * 120);
}

/*
 * clx_get_fan_motor_speed_max - Used to get the maximum threshold of fan motor
 * filled the value to buf
 * @fan_index: start with 0
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_motor_speed_max(void *fan, unsigned int fan_index, unsigned int motor_index,
                                           char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;

    return sprintf(buf, "%d\n", dev->fan_if.fan_max_speed);
}

/*
 * clx_get_fan_motor_speed_min - Used to get the minimum threshold of fan motor
 * filled the value to buf
 * @fan_index: start with 0
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_motor_speed_min(void *fan, unsigned int fan_index, unsigned int motor_index,
                                           char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;

    return sprintf(buf, "%d\n", (dev->fan_if.fan_max_speed / 10));
}

/*
 * clx_get_fan_motor_ratio - Used to get the ratio of fan motor
 * filled the value to buf
 * @fan_index: start with 0
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_get_fan_motor_ratio(void *fan, unsigned int fan_index, unsigned int motor_index,
                                       char *buf, size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 data = 0;
    u8 reg = FAN_PWM_CONTROL_OFFSET;

    clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg, &data, 1);
    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg, data);
    data &= 0x7f;
    return sprintf(buf, "%d\n", data);
}

/*
 * clx_set_fan_motor_ratio - Used to set the ratio of fan motor
 * @fan_index: start with 0
 * @motor_index: start with 1
 * @ratio: motor speed ratio, from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int drv_set_fan_motor_ratio(void *fan, unsigned int fan_index, unsigned int motor_index,
                                   int ratio)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    u8 val = 0;
    u8 reg = FAN_PWM_CONTROL_OFFSET;

    val = (ratio & 0x7f);
    if (val > 0x64)
        val = 0x64;

    LOG_DBG(CLX_DRIVER_TYPES_FAN, "addr: 0x%x, reg: %x, data: %x\r\n", dev->fan_if.addr, reg, val);
    clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

    return DRIVER_OK;
}

/*
 * drv_get_fan_eeprom_size - Used to get fan eeprom size
 *
 * This function returns the size of fan eeprom,
 * otherwise it returns a negative value on failed.
 */
static int drv_get_fan_eeprom_size(void *fan, unsigned int fan_index)
{
    return FAN_EEPROM_SIZE;
}

static int fan_eeprom_wait_bus_tx_done(struct fan_driver_clx *driver)
{
    unsigned char val = 0;
    unsigned char reg = 0;
    unsigned long timeout = jiffies + FAN_EEPROM_I2C_TIMEOUT;

    do
    {
        val = 0;
        reg = FAN_EEPROM_IIC_STATUS_OFFSET;
        clx_i2c_read(driver->fan_if.bus, driver->fan_if.addr, reg, &val, 1);
        if (val & FAN_EEPROM_TX_FINISH_MASK)
        {
            if (val & FAN_EEPROM_TX_ERROR_MASK)
            {
                LOG_DBG(CLX_DRIVER_TYPES_FAN, "fan_eeprom_wait_bus_tx_done data ECOMM error\r\n");
                return -ECOMM;
            }

            return 0;
        }

    } while (time_before(jiffies, timeout));

    LOG_DBG(CLX_DRIVER_TYPES_FAN, "fan_eeprom_wait_bus_tx_done data ETIMEDOUT error\r\n");

    return -ETIMEDOUT;
}

/*
 * drv_read_fan_eeprom_data - Used to read fan eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read fan eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_read_fan_eeprom_data(void *fan, unsigned int fan_index, char *buf, loff_t offset,
                                        size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    unsigned char val = 0;
    unsigned char reg = 0;
    unsigned int i = 0;

    if (count > FAN_EEPROM_SIZE)
    {
        return -EMSGSIZE;
    }

    val = fan_index;
    reg = FAN_EEPROM_SELECT_OFFSET;
    clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

    val = 0x01;
    reg = FAN_EEPROM_DATA_SIZE_OFFSET;
    clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

    val = 0x01;
    reg = FAN_EEPROM_IIC_MAGE_OFFSET;
    clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

    // LOG_DBG(CLX_DRIVER_TYPES_FAN,"drv_read_fan_eeprom len = %d\r\n",count);

    for (i = 0; i < count; i++)
    {
        val = (offset + i);
        reg = FAN_EEPROM_IIC_REG_OFFSET;
        clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

        val = 0x80;
        reg = FAN_EEPROM_IIC_START_OFFSET;
        clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

        if (fan_eeprom_wait_bus_tx_done(dev) != 0)
        {
            return -ETIMEDOUT;
        }
        else
        {
            val = 0;
            reg = FAN_EEPROM_BYTE_READ_OFFSET;
            clx_i2c_read(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);
            buf[i] = val;
            // LOG_DBG(CLX_DRIVER_TYPES_FAN,"drv_read_fan_eeprom i = %d ,buf = 0x%x\r\n",i,buf[i]);
        }
    }

    usleep_range(50, 100);

    return count;
}

/*
 * drv_write_fan_eeprom_data - Used to write fan eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write fan eeprom data
 * @count: length of buf
 *
 * This function returns the written length of fan eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t drv_write_fan_eeprom_data(void *fan, unsigned int fan_index, char *buf, loff_t offset,
                                         size_t count)
{
    struct fan_driver_clx *dev = (struct fan_driver_clx *)fan;
    unsigned char val = 0;
    unsigned char reg = 0;
    unsigned int i = 0;

    if (count > FAN_EEPROM_SIZE)
    {
        return -EMSGSIZE;
    }

    val = fan_index;
    reg = FAN_EEPROM_SELECT_OFFSET;
    clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

    val = 0x01;
    reg = FAN_EEPROM_DATA_SIZE_OFFSET;
    clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

    val = 0x04;
    reg = FAN_EEPROM_IIC_MAGE_OFFSET;
    clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

    // LOG_DBG(CLX_DRIVER_TYPES_FAN,"drv_write_fan_eeprom len = %d\r\n",count);

    for (i = 0; i < count; i++)
    {
        val = (offset + i);
        reg = FAN_EEPROM_IIC_REG_OFFSET;
        clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

        val = buf[i];
        reg = FAN_EEPROM_BYTE_WRITE_OFFSET;
        clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

        // LOG_DBG(CLX_DRIVER_TYPES_FAN,"drv_write_fan_eeprom i = %d ,data = 0x%x\r\n",i,val);

        val = 0x80;
        reg = FAN_EEPROM_IIC_START_OFFSET;
        clx_i2c_write(dev->fan_if.bus, dev->fan_if.addr, reg, &val, 1);

        if (fan_eeprom_wait_bus_tx_done(dev) != 0)
        {
            return -ETIMEDOUT;
        }

        usleep_range(4000, 5000);
    }

    return count;
}

static int drv_fan_clx_dev_init(struct fan_driver_clx *fan)
{
    if (clounix_fpga_base == NULL)
    {
        LOG_ERR(CLX_DRIVER_TYPES_FAN, "fpga resource is not available.\r\n");
        return -ENXIO;
    }
    fan->fan_base = clounix_fpga_base + FAN_BASE_ADDRESS;

    return DRIVER_OK;
}

int drv_fan_clx_init(void **fan_driver)
{
    struct fan_driver_clx *fan = &driver_fan_clx;
    int ret;

    LOG_INFO(CLX_DRIVER_TYPES_FAN, "clx_driver_fan_init\n");
    ret = drv_fan_clx_dev_init(fan);
    if (ret != DRIVER_OK)
        return ret;

    fan->fan_if.get_fan_eeprom_wp = get_fan_eeprom_wp;
    fan->fan_if.set_fan_eeprom_wp = set_fan_eeprom_wp;
    fan->fan_if.get_fan_number = drv_get_fan_number;
    fan->fan_if.get_fan_motor_number = drv_get_fan_motor_number;
    fan->fan_if.get_fan_vendor_name = drv_get_fan_vendor_name;
    fan->fan_if.get_fan_model_name = drv_get_fan_model_name;
    fan->fan_if.get_fan_serial_number = drv_get_fan_serial_number;
    fan->fan_if.get_fan_part_number = drv_get_fan_part_number;
    fan->fan_if.get_fan_hardware_version = drv_get_fan_hardware_version;
    fan->fan_if.get_fan_status = drv_get_fan_status;
    fan->fan_if.get_fan_led_status = drv_get_fan_led_status;
    fan->fan_if.set_fan_led_status = drv_set_fan_led_status;
    fan->fan_if.get_fan_vmon = drv_get_fan_vmon;
    fan->fan_if.get_fan_direction = drv_get_fan_direction;
    fan->fan_if.get_fan_motor_speed = drv_get_fan_motor_speed;
    fan->fan_if.get_fan_motor_speed_tolerance = drv_get_fan_motor_speed_tolerance;
    fan->fan_if.get_fan_motor_speed_target = drv_get_fan_motor_speed_target;
    fan->fan_if.get_fan_motor_speed_max = drv_get_fan_motor_speed_max;
    fan->fan_if.get_fan_motor_speed_min = drv_get_fan_motor_speed_min;
    fan->fan_if.get_fan_motor_ratio = drv_get_fan_motor_ratio;
    fan->fan_if.set_fan_motor_ratio = drv_set_fan_motor_ratio;
    fan->fan_if.get_fan_eeprom_size = drv_get_fan_eeprom_size;
    fan->fan_if.read_fan_eeprom_data = drv_read_fan_eeprom_data;
    fan->fan_if.write_fan_eeprom_data = drv_write_fan_eeprom_data;

    *fan_driver = fan;

    LOG_INFO(CLX_DRIVER_TYPES_FAN, "FAN driver initialization done.\r\n");

    return DRIVER_OK;
}
// clx_driver_define_initcall(drv_fan_clx_init);
