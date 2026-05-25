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
#include <linux/ctype.h>

#include "pddf_client_defs.h"
#include "pddf_fan_defs.h"
#include "pddf_fan_api.h"
#include "pddf_fan_driver.h"

#define FAN_EEPROM_SELECT_OFFSET        (0x20)
#define FAN_EEPROM_IIC_REG_OFFSET       (0x22)
#define FAN_EEPROM_DATA_SIZE_OFFSET     (0x23)
#define FAN_EEPROM_BYTE_WRITE_OFFSET    (0x24)
#define FAN_EEPROM_BYTE_READ_OFFSET     (0x25)
#define FAN_EEPROM_IIC_MAGE_OFFSET      (0x26)
#define FAN_EEPROM_IIC_START_OFFSET     (0x27)
#define FAN_EEPROM_IIC_STATUS_OFFSET    (0x28)

#define FAN_EEPROM_IIC_START_MASK       (1 << 7)
#define FAN_EEPROM_TX_FINISH_MASK       (1 << 7)
#define FAN_EEPROM_TX_ERROR_MASK        (1 << 6)

#define FAN_MAX_SPEED_DS410G            (21000)
#define FAN_EEPROM_I2C_TIMEOUT          (msecs_to_jiffies(500))
#define FAN_EEPROM_MAX_SIZE             (256)
#define FAN_STR_HEAD_LEN                (0x0f)

struct fan_eeprom_priv {
    struct i2c_client   *client;
    struct mutex        lock;
};
#endif /*__PDDF_CUSTOM_FAN_H__*/