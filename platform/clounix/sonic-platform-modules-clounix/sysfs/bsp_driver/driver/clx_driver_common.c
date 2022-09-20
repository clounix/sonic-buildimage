#include <linux/io.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "clx_driver.h"
#include "clx_driver_common.h"

static int32_t clx_i2c_smbus_transfer(int bus, int addr, int read_write, int offset, uint8_t *buf, uint32_t size)
{
    int rv = DRIVER_ERR;
    struct i2c_adapter *i2c_adap;
    union i2c_smbus_data data;

    i2c_adap = i2c_get_adapter(bus);
    if (i2c_adap == NULL) {
        printk(KERN_ERR "get i2c bus[%d] adapter fail\r\n", bus);
        return DRIVER_ERR;
    }

    if (read_write == I2C_SMBUS_WRITE) {
        data.byte = *buf;
    } else {
        data.byte = 0;
    }
    rv = i2c_smbus_xfer(i2c_adap, addr, 0, read_write, offset, size, &data);
    if (rv < 0) {
         printk(KERN_ERR "i2c dev[bus=%d addr=0x%x offset=0x%x size=%d rw=%d] transfer fail, rv=%d\r\n",
            bus, addr, offset, size, read_write, rv);
        rv = DRIVER_ERR;
    } else {
            rv = DRIVER_OK;
    }

    if (read_write == I2C_SMBUS_READ) {
        if (rv == DRIVER_OK) {
            *buf = data.byte;
        } else {
            *buf = 0;
        }
    }

    i2c_put_adapter(i2c_adap);
    return rv;
}

int32_t clx_i2c_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int i, rv;

    for (i = 0; i < size; i++) {
        rv = clx_i2c_smbus_transfer(bus, addr, I2C_SMBUS_READ, offset, &buf[i], I2C_SMBUS_BYTE_DATA);
        if (rv < 0) {
            printk(KERN_ERR "clx_i2c_read[bus=%d addr=0x%x offset=0x%x] fail, rv=%d\r\n",
                bus, addr, offset, rv);
            return rv;
        }
        offset++;
    }

    return size;
}
EXPORT_SYMBOL_GPL(clx_i2c_read);

int32_t clx_i2c_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int i, rv;

    for (i = 0; i < size; i++) {
        rv = clx_i2c_smbus_transfer(bus, addr, I2C_SMBUS_WRITE, offset, &buf[i], I2C_SMBUS_BYTE_DATA);
        if (rv < 0) {
            printk(KERN_ERR "clx_i2c_write[bus=%d addr=0x%x offset=0x%x] fail, rv=%d\r\n",
                bus, addr, offset, rv);
            return rv;
        }
        offset++;
    }

    return size;

}
EXPORT_SYMBOL_GPL(clx_i2c_write);

int32_t clx_i2c_mux_read(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int i, rv;

    for (i = 0; i < size; i++) {
        rv = clx_i2c_smbus_transfer(bus, addr, I2C_SMBUS_READ, offset, &buf[i], I2C_SMBUS_BYTE);
        if (rv < 0) {
            printk(KERN_ERR "clx_i2c_read[bus=%d addr=0x%x offset=0x%x] fail, rv=%d\r\n",
                bus, addr, offset, rv);
            return rv;
        }
        offset++;
    }

    return size;
}
EXPORT_SYMBOL_GPL(clx_i2c_mux_read);

int32_t clx_i2c_mux_write(int bus, int addr, int offset, uint8_t *buf, uint32_t size)
{
    int i, rv;

    for (i = 0; i < size; i++) {
        rv = clx_i2c_smbus_transfer(bus, addr, I2C_SMBUS_WRITE, offset, &buf[i], I2C_SMBUS_BYTE);
        if (rv < 0) {
            printk(KERN_ERR "clx_i2c_write[bus=%d addr=0x%x offset=0x%x] fail, rv=%d\r\n",
                bus, addr, offset, rv);
            return rv;
        }
        offset++;
    }

    return size;

}
EXPORT_SYMBOL_GPL(clx_i2c_mux_write);

int clx_i2c_init(void)
{
    int ret = DRIVER_OK;

    return ret;
}

