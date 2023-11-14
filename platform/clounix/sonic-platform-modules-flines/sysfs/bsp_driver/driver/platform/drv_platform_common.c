#include <linux/io.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "clx_driver.h"
#include "drv_platform_common.h"
#include "clx_platform_interface.h"

static int32_t clx_i2c_smbus_transfer(int bus, int addr, int read_write, int offset, uint8_t *buf, uint32_t size)
{
    int rv = DRIVER_ERR;
    struct i2c_adapter *i2c_adap;
    union i2c_smbus_data data;
    uint8_t retris = I2C_RETRIES_TIMES;

    i2c_adap = i2c_get_adapter(bus);
    if (i2c_adap == NULL) {
        LOG_ERR(CLX_DRIVER_TYPES_PLT, "get i2c bus[%d] adapter fail\r\n", bus);
        return DRIVER_ERR;
    }

    if (read_write == I2C_SMBUS_WRITE) {
        data.byte = *buf;
    } else {
        data.byte = 0;
    }

    while (retris) {
        rv = i2c_smbus_xfer(i2c_adap, addr, 0, read_write, offset, size, &data);
        if (rv < 0) {
            LOG_DBG(CLX_DRIVER_TYPES_PLT, "i2c dev[bus=%d addr=0x%x offset=0x%x size=%d rw=%d] transfer fail, rv=%d\r\n",
                bus, addr, offset, size, read_write, rv);
            rv = DRIVER_ERR;
            usleep_range(3000, 3500);
            retris--;
        } else {
            rv = DRIVER_OK;
            break;
        }
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
            LOG_ERR(CLX_DRIVER_TYPES_PLT, "clx_i2c_read[bus=%d addr=0x%x offset=0x%x]fail, rv=%d\r\n",
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
            LOG_ERR(CLX_DRIVER_TYPES_PLT, "clx_i2c_write[bus=%d addr=0x%x offset=0x%x]fail, rv=%d\r\n",
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
            LOG_ERR(CLX_DRIVER_TYPES_PLT, "clx_i2c_read[bus=%d addr=0x%x offset=0x%x]fail, rv=%d\r\n",
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
            LOG_ERR(CLX_DRIVER_TYPES_PLT, "clx_i2c_mux_write[bus=%d addr=0x%x offset=0x%x]fail, rv=%d\r\n",
                bus, addr, offset, rv);
            return rv;
        }
        offset++;
    }

    return size;

}
EXPORT_SYMBOL_GPL(clx_i2c_mux_write);

int clx_syseeprom_read(uint8_t *buf, int offset, uint32_t size)
{
    //char dummy = 0;

    //to be update for standard interface
    //clx_i2c_mux_write(CLX_SYSEEPROM_BUS, CLX_PCA9548_ADDR, CLX_PCA9548_CHANNEL_IDROM, &dummy, 1);
    return clx_i2c_read(CLX_SYSEEPROM_BUS, CLX_SYSEEPROM_ADDR, offset, buf, size); 
}

/* vi: set sw=4 ts=4: */
/*
 * CRC32 table fill function
 * Copyright (C) 2006 by Rob Sullivan <cogito.ergo.cogito@gmail.com>
 * (I can't really claim much credit however, as the algorithm is
 * very well-known)
 *
 * The following function creates a CRC32 table depending on whether
 * a big-endian (0x04c11db7) or little-endian (0xedb88320) CRC32 is
 * required. Admittedly, there are other CRC32 polynomials floating
 * around, but Busybox doesn't use them.
 *
 * endian = 1: big-endian
 * endian = 0: little-endian
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
static uint32_t global_crc[256];
uint32_t *global_crc32_table = (uint32_t *)&global_crc;

uint32_t* crc32_filltable(uint32_t *crc_table, int endian)
{
        uint32_t polynomial = endian ? 0x04c11db7 : 0xedb88320;
        uint32_t c;
        int i, j;

        if (!crc_table)
                crc_table = vmalloc(256 * sizeof(uint32_t));

        for (i = 0; i < 256; i++) {
                c = endian ? (i << 24) : i;
                for (j = 8; j; j--) {
                        if (endian)
                                c = (c&0x80000000) ? ((c << 1) ^ polynomial) : (c << 1);
                        else
                                c = (c&1) ? ((c >> 1) ^ polynomial) : (c >> 1);
                }
                *crc_table++ = c;
        }

        return crc_table - 256;
}

uint32_t crc32_block_endian1(uint32_t val, const void *buf, unsigned len, uint32_t *crc_table)
{
        const void *end = (uint8_t*)buf + len;

        while (buf != end) {
                val = (val << 8) ^ crc_table[(val >> 24) ^ *(uint8_t*)buf];
                buf = (uint8_t*)buf + 1;
        }
        return val;
}

uint32_t crc32_block_endian0(uint32_t val, const void *buf, unsigned len, uint32_t *crc_table)
{
        const void *end = (uint8_t*)buf + len;

        while (buf != end) {
                val = crc_table[(uint8_t)val ^ *(uint8_t*)buf] ^ (val >> 8);
                buf = (uint8_t*)buf + 1;
        }
        return val;
}


unsigned long crc32 (unsigned long crc, const unsigned char *buf, unsigned len)
{
    crc32_filltable(global_crc32_table, 0);
    return crc32_block_endian0( crc ^ 0xffffffffL, buf, len, global_crc32_table) ^ 0xffffffffL;
}

/*
 *  is_valid_tlv
 *
 *  Perform basic sanity checks on a TLV field. The TLV is pointed to
 *  by the parameter provided.
 *      1. The type code is not reserved (0x00 or 0xFF)
 */
static inline bool is_valid_tlv(tlvinfo_tlv_t *tlv)
{
    return((tlv->type != 0x00) && (tlv->type != 0xFF));
}

/*
 *  is_valid_tlvinfo_header
 *
 *  Perform sanity checks on the first 11 bytes of the TlvInfo EEPROM
 *  data pointed to by the parameter:
 *      1. First 8 bytes contain null-terminated ASCII string "TlvInfo"
 *      2. Version byte is 1
 *      3. Total length bytes contain value which is less than or equal
 *         to the allowed maximum (2048-11)
 *
 */
static inline bool is_valid_tlvinfo_header(tlvinfo_header_t *hdr)
{
    int max_size = TLV_TOTAL_LEN_MAX;
    return((strcmp(hdr->signature, TLV_INFO_ID_STRING) == 0) &&
           (hdr->version == TLV_INFO_VERSION) &&
           (be16_to_cpu(hdr->totallen) <= max_size) );
}

/*
 *  is_checksum_valid
 *
 *  Validate the checksum in the provided TlvInfo EEPROM data. First,
 *  verify that the TlvInfo header is valid, then make sure the last
 *  TLV is a CRC-32 TLV. Then calculate the CRC over the EEPROM data
 *  and compare it to the value stored in the EEPROM CRC-32 TLV.
 */
static bool is_checksum_valid(u_int8_t *eeprom)
{
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t    * eeprom_crc;
    unsigned int       calc_crc;
    unsigned int       stored_crc;

    // Is the eeprom header valid?
    if (!is_valid_tlvinfo_header(eeprom_hdr)) {
        return(FALSE);
    }

    // Is the last TLV a CRC?
    eeprom_crc = (tlvinfo_tlv_t *) &eeprom[sizeof(tlvinfo_header_t) +
                                           be16_to_cpu(eeprom_hdr->totallen) -
                                           (sizeof(tlvinfo_tlv_t) + 4)];
    if ((eeprom_crc->type != TLV_CODE_CRC_32) || (eeprom_crc->length != 4)) {
        return(FALSE);
    }

    // Calculate the checksum
    calc_crc = crc32(0, (void *)eeprom, sizeof(tlvinfo_header_t) +
                     be16_to_cpu(eeprom_hdr->totallen) - 4);
    stored_crc = ((eeprom_crc->value[0] << 24) | (eeprom_crc->value[1] << 16) |
                  (eeprom_crc->value[2] <<  8) | eeprom_crc->value[3]);
    return(calc_crc == stored_crc);
}

/*
 *  tlvinfo_find_tlv
 *
 *  This function finds the TLV with the supplied code in the EERPOM.
 *  An offset from the beginning of the EEPROM is returned in the
 *  eeprom_index parameter if the TLV is found.
 */
bool tlvinfo_find_tlv(u_int8_t *eeprom, u_int8_t tcode,
                             int *eeprom_index)
{
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t    * eeprom_tlv;
    int eeprom_end;

    // Search through the TLVs, looking for the first one which matches the
    // supplied type code.
    *eeprom_index = sizeof(tlvinfo_header_t);
    eeprom_end = sizeof(tlvinfo_header_t) + be16_to_cpu(eeprom_hdr->totallen);
    while (*eeprom_index < eeprom_end) {
        eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[*eeprom_index];
        if (!is_valid_tlv(eeprom_tlv)) {
            return(FALSE);
        }
        if (eeprom_tlv->type == tcode) {
            return(TRUE);
        }
        *eeprom_index += sizeof(tlvinfo_tlv_t) + eeprom_tlv->length;
    }
    return(FALSE);
}

/*
 *  update_crc
 *
 *  This function updates the CRC-32 TLV. If there is no CRC-32 TLV, then
 *  one is added. This function should be called after each update to the
 *  EEPROM structure, to make sure the CRC is always correct.
 */
static void update_crc(u_int8_t *eeprom)
{
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t    * eeprom_crc;
    unsigned int       calc_crc;
    int                eeprom_index;

    // Discover the CRC TLV
    if (!tlvinfo_find_tlv(eeprom, TLV_CODE_CRC_32, &eeprom_index)) {
        if ((be16_to_cpu(eeprom_hdr->totallen) + sizeof(tlvinfo_tlv_t) + 4) >
            TLV_TOTAL_LEN_MAX) {
            return;
        }
        eeprom_index = sizeof(tlvinfo_header_t) +
            be16_to_cpu(eeprom_hdr->totallen);
        eeprom_hdr->totallen = cpu_to_be16(be16_to_cpu(eeprom_hdr->totallen) +
                                           sizeof(tlvinfo_tlv_t) + 4);
    }
    eeprom_crc = (tlvinfo_tlv_t *) &eeprom[eeprom_index];
    eeprom_crc->type = TLV_CODE_CRC_32;
    eeprom_crc->length = 4;

    // Calculate the checksum
    calc_crc = crc32(0, (void *)eeprom,
                     sizeof(tlvinfo_header_t) +
                     be16_to_cpu(eeprom_hdr->totallen) - 4);
    eeprom_crc->value[0] = (calc_crc >> 24) & 0xFF;
    eeprom_crc->value[1] = (calc_crc >> 16) & 0xFF;
    eeprom_crc->value[2] = (calc_crc >>  8) & 0xFF;
    eeprom_crc->value[3] = (calc_crc >>  0) & 0xFF;
    return;
}

/*
 *  read_eeprom
 *
 *  Read the EEPROM into memory, if it hasn't already been read.
 */
int read_eeprom(u_int8_t *eeprom)
{
    int ret;
    tlvinfo_header_t *eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t *eeprom_tlv = (tlvinfo_tlv_t *)&eeprom[
        sizeof(tlvinfo_header_t)];

    /* Read the header */
    ret = clx_syseeprom_read((void *)eeprom_hdr, 0, sizeof(tlvinfo_header_t));
    /* If the header was successfully read, read the TLVs */
    if ((ret >= 0) && is_valid_tlvinfo_header(eeprom_hdr)) {
        ret = clx_syseeprom_read((void *)eeprom_tlv, sizeof(tlvinfo_header_t),
                              be16_to_cpu(eeprom_hdr->totallen));
    }
    // If the contents are invalid, start over with default contents
    if (!is_valid_tlvinfo_header(eeprom_hdr))
        LOG_ERR(CLX_DRIVER_TYPES_PLT, 
                "Notice:  Invalid TLV header found.  Using default contents.\n");
    if (!is_checksum_valid(eeprom))
        LOG_ERR(CLX_DRIVER_TYPES_PLT, 
                "Notice:  Invalid TLV checksum found.  Using default contents.\n");
     
    if ( !is_valid_tlvinfo_header(eeprom_hdr) || !is_checksum_valid(eeprom) ){
        strcpy(eeprom_hdr->signature, TLV_INFO_ID_STRING);
        eeprom_hdr->version = TLV_INFO_VERSION;
        eeprom_hdr->totallen = cpu_to_be16(0);
        update_crc(eeprom);
    }

    return ret;
}

/*
 *  decode_tlv_value
 *
 *  Decode a single TLV value into a string.

 *  The validity of EEPROM contents and the TLV field have been verified
 *  prior to calling this function.
 */
#define DECODE_NAME_MAX     20

static void decode_tlv_value(tlvinfo_tlv_t * tlv, char* value)
{
    int i;

    switch (tlv->type) {
    case TLV_CODE_PRODUCT_NAME:
    case TLV_CODE_PART_NUMBER:
    case TLV_CODE_SERIAL_NUMBER:
    case TLV_CODE_MANUF_DATE:
    case TLV_CODE_LABEL_REVISION:
    case TLV_CODE_PLATFORM_NAME:
    case TLV_CODE_ONIE_VERSION:
    case TLV_CODE_MANUF_NAME:
    case TLV_CODE_MANUF_COUNTRY:
    case TLV_CODE_VENDOR_NAME:
    case TLV_CODE_DIAG_VERSION:
    case TLV_CODE_SERVICE_TAG:
        memcpy(value, tlv->value, tlv->length);
        value[tlv->length] = 0;
        break;
    case TLV_CODE_MAC_BASE:
        sprintf(value, "%02X:%02X:%02X:%02X:%02X:%02X",
                tlv->value[0], tlv->value[1], tlv->value[2],
                tlv->value[3], tlv->value[4], tlv->value[5]);
        break;
    case TLV_CODE_DEVICE_VERSION:
        sprintf(value, "%u", tlv->value[0]);
        break;
    case TLV_CODE_MAC_SIZE:
        sprintf(value, "%u", (tlv->value[0] << 8) | tlv->value[1]);
        break;
    case TLV_CODE_VENDOR_EXT:
        value[0] = 0;
        for (i = 0; (i < (TLV_DECODE_VALUE_MAX_LEN/5)) && (i < tlv->length);
             i++) {
            sprintf(value, "%s 0x%02X", value, tlv->value[i]);
        }
        break;
    case TLV_CODE_CRC_32:
        sprintf(value, "0x%02X%02X%02X%02X",
                tlv->value[0], tlv->value[1], tlv->value[2],
                tlv->value[3]);
        break;
    default:
        value[0] = 0;
        for (i = 0; (i < (TLV_DECODE_VALUE_MAX_LEN/5)) && (i < tlv->length);
             i++) {
            sprintf(value, "%s 0x%02X", value, tlv->value[i]);
        }
        break;
    }

}

/*
 *  tlvinfo_decode_tlv
 *
 *  This function finds the TLV with the supplied code in the EERPOM
 *  and decodes the value into the buffer provided.
 */
bool tlvinfo_decode_tlv(u_int8_t *eeprom, u_int8_t tcode, char* value)
{
    int eeprom_index;
    tlvinfo_tlv_t * eeprom_tlv;

    // Find the TLV and then decode it
    if (tlvinfo_find_tlv(eeprom, tcode, &eeprom_index)) {
        eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[eeprom_index];
        decode_tlv_value(eeprom_tlv, value);
        return TRUE;
    }

    return FALSE;
}

static u_int8_t eeprom[SYS_EEPROM_SIZE];
static char tlv_value[TLV_DECODE_VALUE_MAX_LEN];
int clx_driver_common_init(char *hw_platform)
{
    int ret = DRIVER_OK;

    if (read_eeprom(eeprom) < 0) {
        LOG_ERR(CLX_DRIVER_TYPES_PLT, "clx_driver_common_init:failed to read syseeprom");
        return DRIVER_ERR;
    }
    if (tlvinfo_decode_tlv(eeprom, TLV_CODE_PLATFORM_NAME, tlv_value)) {
        LOG_DBG(CLX_DRIVER_TYPES_PLT, "decode product name:%s", tlv_value);
        snprintf(hw_platform, PRODUCT_NAME_LEN_MAX, "%s", tlv_value);
    } else {
        LOG_ERR(CLX_DRIVER_TYPES_PLT, "clx_driver_common_init:failed to decode syseeprom");
        ret = DRIVER_ERR;
    }

    return ret;
}

