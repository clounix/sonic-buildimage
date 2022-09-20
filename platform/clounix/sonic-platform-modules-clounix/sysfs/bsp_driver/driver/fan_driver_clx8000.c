#include <linux/io.h>

#include "fan_driver_clx8000.h"
#include "clx_driver_common.h"
#include "clounix_fpga_clx8000.h"
#include "clx_driver.h"

//external function declaration
extern int32_t clx_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size);
extern int32_t clx_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size);

extern void __iomem *clounix_fpga_base;

//internal function declaration
struct fan_driver_clx8000 driver_fan_clx8000;

static u8 led_state_user_to_dev[] = { DEV_FAN_LED_DARK, DEV_FAN_LED_GREEN, DEV_FAN_LED_YELLOW, DEV_FAN_LED_RED,
USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT };
static u8 led_state_dev_to_user[] = { USER_FAN_LED_DARK, USER_FAN_LED_GREEN, USER_FAN_LED_RED, USER_FAN_LED_YELLOW };


static int clx_driver_clx8000_get_fan_number(void *fan)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
    return dev->fan_num;
}

static int clx_driver_clx8000_get_fan_motor_number(void *fan, unsigned int fan_index)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
    return dev->motor_per_fan;
}


static ssize_t clx_driver_clx8000_get_fan_vendor_name(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "clounix");
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
static ssize_t clx_driver_clx8000_get_fan_model_name(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "SFD-GB0412UHG");
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
static ssize_t clx_driver_clx8000_get_fan_serial_number(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "N/A");
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
static ssize_t clx_driver_clx8000_get_fan_part_number(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "N/A");
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
static ssize_t clx_driver_clx8000_get_fan_hardware_version(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    return sprintf(buf, "1.0");
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
static ssize_t clx_driver_clx8000_get_fan_status(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
    u8 data = 0;
    u8 present = 0;
    u8 reg = FAN_PRESENT_OFFSET;

    clx_i2c_read(dev->bus, dev->addr, reg, &data, 1);
    FAN_DBG("addr: 0x%x, reg: %x, data: %x\r\n", dev->addr, reg, data);
    GET_BIT(data, fan_index, present);

    return sprintf(buf, "0x%02x\n", !present);
}

static u8 fan_led_get(struct fan_driver_clx8000 *dev, unsigned char fan_index)
{
    u8 data1 = 0, data2 = 0;
    u8 reg1 = FAN_LED1_CONTROL_OFFSET, reg2 = FAN_LED2_CONTROL_OFFSET;
    u8 val1 = 0, val2 = 0;
    u8 dev_led_state = 0;

    clx_i2c_read(dev->bus, dev->addr, reg1, &data1, 1);
    clx_i2c_read(dev->bus, dev->addr, reg2, &data2, 1);
    FAN_DBG("addr: 0x%x, reg1: %x, data1: %x reg2:%x data3:%x\r\n", dev->addr, reg1, data1, reg2, data2);
    GET_BIT(data1, fan_index, val1);
    GET_BIT(data2, fan_index, val2);
    dev_led_state = ((val1 | (val2 << 1)) & 0x3);

    return led_state_dev_to_user[dev_led_state];
}

static u8 fan_led_set(struct fan_driver_clx8000 *dev, unsigned char fan_index, unsigned char user_led_state)
{
    u8 data = 0;
    u8 reg[FAN_LED_REG_MAX] = {FAN_LED1_CONTROL_OFFSET, FAN_LED2_CONTROL_OFFSET};
    u8 val = 0;
    u8 dev_led_state = led_state_user_to_dev[user_led_state & 0x3];
    u8 i;

    for (i = 0; i < FAN_LED_REG_MAX; i++)
    {
        clx_i2c_read(dev->bus, dev->addr, reg[i], &data, 1);
        val = ((dev_led_state >> i) & 0x1);
        if(val)
            SET_BIT(data, fan_index);
        else
            CLEAR_BIT(data, fan_index);
        clx_i2c_write(dev->bus, dev->addr, reg[i], &data, 1);
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
static ssize_t clx_driver_clx8000_get_fan_led_status(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
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
static int clx_driver_clx8000_set_fan_led_status(void *fan, unsigned int fan_index, int status)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;    
    return fan_led_set(dev, fan_index, status);
}

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
static ssize_t clx_driver_clx8000_get_fan_direction(void *fan, unsigned int fan_index, char *buf, size_t count)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
    u8 data = 0;
    u8 val = 0;
    u8 reg = FAN_AIR_DIRECTION_OFFSET;

    clx_i2c_read(dev->bus, dev->addr, reg, &data, 1);
    FAN_DBG("addr: 0x%x, reg: %x, data: %x\r\n", dev->addr, reg, data);
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
static ssize_t clx_driver_clx8000_get_fan_motor_speed(void *fan, unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
    u8 data1 = 0, data2=0;
    u8 reg1 = FAN1_INNER_RPM_OFFSET + fan_index;
    u8 reg2 = FAN1_OUTER_RPM_OFFSET + fan_index;

    clx_i2c_read(dev->bus, dev->addr, reg1, &data1, 1);
    FAN_DBG("addr: 0x%x, reg: %x, data: %x\r\n", dev->addr, reg1, data1);

    clx_i2c_read(dev->bus, dev->addr, reg2, &data2, 1);
    FAN_DBG("addr: 0x%x, reg: %x, data: %x\r\n", dev->addr, reg2, data2);

    return sprintf(buf, "%d\n", (data1+data2)*60);
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
static ssize_t clx_driver_clx8000_get_fan_motor_speed_tolerance(void *fan, unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    /* to be update: is it not supported from hardware */
    return sprintf(buf, "2820");
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
static ssize_t clx_driver_clx8000_get_fan_motor_speed_target(void *fan, unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
    u8 data = 0;
    u8 reg = FAN_PWM_CONTROL_OFFSET;

    clx_i2c_read(dev->bus, dev->addr, reg, &data, 1);
    FAN_DBG( "addr: 0x%x, reg: %x, data: %x\r\n", dev->addr, reg, data);

    return sprintf(buf, "%d\n", data*120);
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
static ssize_t clx_driver_clx8000_get_fan_motor_speed_max(void *fan, unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    return sprintf(buf, "28200");
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
static ssize_t clx_driver_clx8000_get_fan_motor_speed_min(void *fan, unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    return sprintf(buf, "2820");
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
static ssize_t clx_driver_clx8000_get_fan_motor_ratio(void *fan, unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
    u8 data = 0;
    u8 reg = FAN_PWM_CONTROL_OFFSET;

    clx_i2c_read(dev->bus, dev->addr, reg, &data, 1);
    FAN_DBG("addr: 0x%x, reg: %x, data: %x\r\n", dev->addr, reg, data);
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
static int clx_driver_clx8000_set_fan_motor_ratio(void *fan, unsigned int fan_index, unsigned int motor_index,
                   int ratio)
{
    struct fan_driver_clx8000 *dev = (struct fan_driver_clx8000 *)fan;
    u8 val = 0;
    u8 reg = FAN_PWM_CONTROL_OFFSET;

    val = (ratio & 0x7f);
    if(val > 0x64)
        val =  0x64;
    FAN_DBG("addr: 0x%x, reg: %x, data: %x\r\n", dev->addr, reg, val);
    clx_i2c_write(dev->bus, dev->addr, reg, &val, 1);

    return DRIVER_OK;
}

static int clx_driver_clx8000_fan_dev_init(struct fan_driver_clx8000 *fan)
{
    if (clounix_fpga_base == NULL) {
        FAN_ERR("fpga resource is not available.\r\n");
        return -ENXIO;
    }
    fan->fan_base = clounix_fpga_base + FAN_BASE_ADDRESS;

    return DRIVER_OK;
}

//void clx_driver_clx8000_fan_init(struct fan_driver_clx8000 **fan_driver)
void clx_driver_clx8000_fan_init(void **fan_driver)
{
    struct fan_driver_clx8000 *fan = &driver_fan_clx8000;

    FAN_INFO("clx_driver_clx8000_fan_init\n");
    clx_driver_clx8000_fan_dev_init(fan);

    fan->fan_if.get_fan_number = clx_driver_clx8000_get_fan_number;
    fan->fan_if.get_fan_motor_number = clx_driver_clx8000_get_fan_motor_number;
    fan->fan_if.get_fan_vendor_name = clx_driver_clx8000_get_fan_vendor_name;
    fan->fan_if.get_fan_model_name = clx_driver_clx8000_get_fan_model_name;
    fan->fan_if.get_fan_serial_number = clx_driver_clx8000_get_fan_serial_number;
    fan->fan_if.get_fan_part_number = clx_driver_clx8000_get_fan_part_number;
    fan->fan_if.get_fan_hardware_version = clx_driver_clx8000_get_fan_hardware_version;
    fan->fan_if.get_fan_status = clx_driver_clx8000_get_fan_status;
    fan->fan_if.get_fan_led_status = clx_driver_clx8000_get_fan_led_status;
    fan->fan_if.set_fan_led_status = clx_driver_clx8000_set_fan_led_status;
    fan->fan_if.get_fan_direction = clx_driver_clx8000_get_fan_direction;
    fan->fan_if.get_fan_motor_speed = clx_driver_clx8000_get_fan_motor_speed;
    fan->fan_if.get_fan_motor_speed_tolerance = clx_driver_clx8000_get_fan_motor_speed_tolerance;
    fan->fan_if.get_fan_motor_speed_target = clx_driver_clx8000_get_fan_motor_speed_target;
    fan->fan_if.get_fan_motor_speed_max = clx_driver_clx8000_get_fan_motor_speed_max;
    fan->fan_if.get_fan_motor_speed_min = clx_driver_clx8000_get_fan_motor_speed_min;
    fan->fan_if.get_fan_motor_ratio = clx_driver_clx8000_get_fan_motor_ratio;
    fan->fan_if.set_fan_motor_ratio = clx_driver_clx8000_set_fan_motor_ratio;
    fan->fan_num = FAN_CLX8000_MAX;
    fan->motor_per_fan = MOTOR_NUM_PER_FAN;
    fan->bus = CLX_FAN_BUS;
    fan->addr = CLX_FAN_ADDR;
    *fan_driver = fan;
    //FAN_DBG("FAN driver clx8000 initialization done.\r\n");

    return;
}
//clx_driver_define_initcall(clx_driver_clx8000_fan_init);

