#ifndef __FPGA_DATA_HEADER_H__
#define __FPGA_DATA_HEADER_H__

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#define UPGRADE_MAGIC 0x50554C43

enum item_tag_s{
    TAG_CHIP_TYPE = 1,
    TAG_BOARD_TYPE,
    TAG_VER,
    TAG_INFO,
    TAG_CPLD_FB = 11,
    TAG_CPLD_MB,
    TAG_CPLD_MB0,
    TAG_CPLD_MB1,
    TAG_CPLD_SEQ,
    TAG_FPGA_PRIMARY = 21,
    TAG_FPGA_BACKUP,
    TAG_MAX
};

typedef struct header_s {
    unsigned int magic;
    unsigned int num;
    unsigned char reserve[8];
    unsigned int crc;
    unsigned int len;
    unsigned char data[0];
} header_t;

typedef struct item_s {
    unsigned int size:24;
    unsigned int tag:8;
    unsigned int offset;
    unsigned char data[0];
} item_t;

int parse_data_header(uint8_t *image, size_t size, uint32_t region);

#endif /*__FPGA_DATA_HEADER_H__*/