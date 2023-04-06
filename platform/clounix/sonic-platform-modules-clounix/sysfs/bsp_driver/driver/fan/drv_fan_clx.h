#ifndef _FAN_DRIVER_CLX8000_H_
#define _FAN_DRIVER_CLX8000_H_

#include "fan_interface.h"

struct fan_driver_clx8000 {
    struct fan_fn_if fan_if;
    //private
    void __iomem *fan_base;
};

#define FAN_CHIP_NUM 2

#define FAN_BASE_ADDRESS           (0x0300)

//register define
#define FAN_VERSION_ADDR           (0x4)

enum hwmon_fan_offset
{
    FAN_CPLD_VERSION_OFFSET = 0x00,
    FAN_BORAD_ID_OFFSET,
    FAN_PRESENT_OFFSET,
    FAN_PWM_CONTROL_OFFSET,

    FAN1_OUTER_RPM_OFFSET = 0x04,
    FAN2_OUTER_RPM_OFFSET,
    FAN3_OUTER_RPM_OFFSET,
    FAN4_OUTER_RPM_OFFSET,
    FAN5_OUTER_RPM_OFFSET,

    FAN1_INNER_RPM_OFFSET = 0x09,
    FAN2_INNER_RPM_OFFSET,
    FAN3_INNER_RPM_OFFSET,
    FAN4_INNER_RPM_OFFSET,
    FAN5_INNER_RPM_OFFSET,

    FAN_LED1_CONTROL_OFFSET = 0x0e,
    FAN_LED2_CONTROL_OFFSET = 0x0f,

    FAN_OUTER_EN_OFFSET = 0x10,
    FAN_INNER_EN_OFFSET,
    FAN_INT_OFFSET,
    FAN_EEPROM_WRITE_EN_OFFSET,

    FAN_AIR_DIRECTION_OFFSET = 0x14,
    FAN_USB_EN_OFFSET =0x15,
    FAN6_OUTER_RPM_OFFSET = 0x16,
    FAN6_INNER_RPM_OFFSET = 0x17    
};

enum user_fan_led_state {
    USER_FAN_LED_DARK,
    USER_FAN_LED_GREEN,
    USER_FAN_LED_YELLOW,
    USER_FAN_LED_RED,
    USER_FAN_LED_NOT_SUPPORT
};
/*
*extract the value from FAN_LED2_CONTROL_OFFSET FAN_LED1_CONTROL_OFFSET, and mapping is as below
* 00 DARK
* 01 GREEN
* 10 RED
* 11 YELLOW
*/
enum dev_fan_led_state {
    DEV_FAN_LED_DARK,
    DEV_FAN_LED_GREEN,
    DEV_FAN_LED_RED,
    DEV_FAN_LED_YELLOW,
};
#define FAN_LED_REG_MAX 2

#endif //_FAN_DRIVER_CLX8000_H_
