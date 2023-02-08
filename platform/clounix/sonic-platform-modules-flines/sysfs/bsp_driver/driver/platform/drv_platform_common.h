#ifndef _DRV_PLATFORM_COMMON_H_
#define _DRV_PLATFORM_COMMON_H_

#define CLX_SYSEEPROM_BUS 0
#define CLX_SYSEEPROM_ADDR 0x50

#define CLX_PCA9548_ADDR 0x70
#define CLX_PCA9548_CHANNEL_IDROM 0x40

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define SYS_EEPROM_SIZE 4096
/*
 * Tlvinf header: Layout of the header for the TlvInfo format
 *
 * See the end of this file for details of this eeprom format
 */
struct __attribute__ ((__packed__)) tlvinfo_header_s {
    char    signature[8];   /* 0x00 - 0x07 EEPROM Tag "TlvInfo" */
    u_int8_t      version;  /* 0x08        Structure version */
    u_int16_t     totallen; /* 0x09 - 0x0A Length of all data which follows */
};
typedef struct tlvinfo_header_s tlvinfo_header_t;

// Header Field Constants
#define TLV_INFO_ID_STRING      "TlvInfo"
#define TLV_INFO_VERSION        0x01
#define TLV_TOTAL_LEN_MAX       (SYS_EEPROM_SIZE - sizeof(tlvinfo_header_t))

/*
 * TlvInfo TLV: Layout of a TLV field
 */
struct __attribute__ ((__packed__)) tlvinfo_tlv_s {
    u_int8_t  type;
    u_int8_t  length;
    u_int8_t  value[0];
};
typedef struct tlvinfo_tlv_s tlvinfo_tlv_t;

/* Maximum length of a TLV value in bytes */
#define TLV_VALUE_MAX_LEN        255

/**
 *  The TLV Types.
 *
 *  Keep these in sync with tlv_code_list in cmd_sys_eeprom.c
 */
#define TLV_CODE_PRODUCT_NAME   0x21
#define TLV_CODE_PART_NUMBER    0x22
#define TLV_CODE_SERIAL_NUMBER  0x23
#define TLV_CODE_MAC_BASE       0x24
#define TLV_CODE_MANUF_DATE     0x25
#define TLV_CODE_DEVICE_VERSION 0x26
#define TLV_CODE_LABEL_REVISION 0x27
#define TLV_CODE_PLATFORM_NAME  0x28
#define TLV_CODE_ONIE_VERSION   0x29
#define TLV_CODE_MAC_SIZE       0x2A
#define TLV_CODE_MANUF_NAME     0x2B
#define TLV_CODE_MANUF_COUNTRY  0x2C
#define TLV_CODE_VENDOR_NAME    0x2D
#define TLV_CODE_DIAG_VERSION   0x2E
#define TLV_CODE_SERVICE_TAG    0x2F
#define TLV_CODE_VENDOR_EXT     0xFD
#define TLV_CODE_CRC_32         0xFE

/*
 * The max decode value is currently for the 'raw' type or the 'vendor
 * extension' type, both of which have the same decode format.  The
 * max decode string size is computed as follows:
 *
 *   strlen(" 0xFF") * TLV_VALUE_MAX_LEN + 1
 *
 */
#define TLV_DECODE_VALUE_MAX_LEN    ((5 * TLV_VALUE_MAX_LEN) + 1)

#define I2C_RETRIES_TIMES 3

#endif //_DRV_PLATFORM_COMMON_H_
