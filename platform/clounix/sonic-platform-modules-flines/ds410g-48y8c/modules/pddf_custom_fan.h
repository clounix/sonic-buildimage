#ifndef __PDDF_CUSTOM_FAN_H__
#define __PDDF_CUSTOM_FAN_H__
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/delay.h>

#include "pddf_client_defs.h"
#include "pddf_fan_defs.h"
#include "pddf_fan_api.h"
#include "pddf_fan_driver.h"

#define FAN_MAX_SPEED_DS410G 21000
#define FAN_EEPROM_SIZE 256
#define FAN_EEPROM_I2C_TIMEOUT (msecs_to_jiffies(500))
#define FAN_EEPROM_TX_FINISH_MASK (0x80)
#define FAN_EEPROM_TX_ERROR_MASK (0x40)

enum FAN_CPLD_REG
{
    FAN_EEPROM_SELECT_OFFSET = 0x20,
    FAN_EEPROM_IIC_SPEED_OFFSET = 0x21,
    FAN_EEPROM_IIC_REG_OFFSET = 0x22,
    FAN_EEPROM_DATA_SIZE_OFFSET = 0x23,
    FAN_EEPROM_BYTE_WRITE_OFFSET = 0x24,
    FAN_EEPROM_BYTE_READ_OFFSET = 0x25,
    FAN_EEPROM_IIC_MAGE_OFFSET = 0x26,
    FAN_EEPROM_IIC_START_OFFSET = 0x27,
    FAN_EEPROM_IIC_STATUS_OFFSET = 0x28
};

enum user_fan_led_state
{
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
enum dev_fan_led_state
{
    DEV_FAN_LED_DARK,
    DEV_FAN_LED_GREEN,
    DEV_FAN_LED_RED,
    DEV_FAN_LED_YELLOW,
};


static uint8_t led_state_user_to_dev[] = {DEV_FAN_LED_DARK, DEV_FAN_LED_GREEN, DEV_FAN_LED_YELLOW, DEV_FAN_LED_RED,
                                     USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT, USER_FAN_LED_NOT_SUPPORT};
static uint8_t led_state_dev_to_user[] = {USER_FAN_LED_DARK, USER_FAN_LED_GREEN, USER_FAN_LED_RED, USER_FAN_LED_YELLOW};


#endif /*__PDDF_CUSTOM_FAN_H__*/