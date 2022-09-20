#ifndef _XCVR_DRIVER_CLX8000_H_
#define _XCVR_DRIVER_CLX8000_H_
#include <linux/types.h>
#include <linux/io.h>

#include "xcvr_interface.h"
#include "clounix/clounix_pub.h"

#define XCVR_PORT_MAX 56
#define XCVR_CHIP_NUM 1
#define XCVR_BASE_ADDRESS           (0x0)

//register define
#define XCVR_HW_VERSION_ADDR           (0x0)
#define XCVR_FW_VERSION_ADDR           (0x4)
#define MAX_DEBUG_INFO_LEN  1024

#define NUM_ADDRESS 2

/* The maximum length of a port name */
#define MAX_PORT_NAME_LEN 20

/* fundamental unit of addressing for EEPROM */
#define SFP_PAGE_SIZE 128

/*
 * Single address devices (eg QSFP) have 256 pages, plus the unpaged
 * low 128 bytes.  If the device does not support paging, it is
 * only 2 'pages' long.
 */
#define SFP_ARCH_PAGES 256
#define ONE_ADDR_EEPROM_SIZE ((1 + SFP_ARCH_PAGES) * SFP_PAGE_SIZE)
#define ONE_ADDR_EEPROM_UNPAGED_SIZE (2 * SFP_PAGE_SIZE)

/*
 * Dual address devices (eg SFP) have 256 pages, plus the unpaged
 * low 128 bytes, plus 256 bytes at 0x50.  If the device does not
 * support paging, it is 4 'pages' long.
 */
#define TWO_ADDR_EEPROM_SIZE ((3 + SFP_ARCH_PAGES) * SFP_PAGE_SIZE)
#define TWO_ADDR_EEPROM_UNPAGED_SIZE (4 * SFP_PAGE_SIZE)
#define TWO_ADDR_NO_0X51_SIZE (2 * SFP_PAGE_SIZE)

/*
 * flags to distinguish one-address (QSFP family) from two-address (SFP family)
 * If the family is not known, figure it out when the device is accessed
 */
#define ONE_ADDR  1
#define TWO_ADDR  2
#define CMIS_ADDR 3

/* a few constants to find our way around the EEPROM */\
#define SFP_EEPROM_A0_ADDR  0xA0
#define SFP_EEPROM_A2_ADDR  0xA2
#define SFP_PAGE_SELECT_REG   0x7F
#define ONE_ADDR_PAGEABLE_REG 0x02
#define QSFP_NOT_PAGEABLE (1<<2)
#define CMIS_NOT_PAGEABLE (1<<7)
#define TWO_ADDR_PAGEABLE_REG 0x40
#define TWO_ADDR_PAGEABLE (1<<4)
#define TWO_ADDR_0X51_REG 92
#define TWO_ADDR_0X51_SUPP (1<<6)
#define SFP_ID_REG 0
#define SFP_READ_OP 0
#define SFP_WRITE_OP 1
#define SFP_EOF 0  /* used for access beyond end of device */
#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

#define SFP_FAST_BYTE_LENGTH 4

/*
 * This parameter is to help this driver avoid blocking other drivers out
 * of I2C for potentially troublesome amounts of time. With a 100 kHz I2C
 * clock, one 256 byte read takes about 1/43 second which is excessive;
 * but the 1/170 second it takes at 400 kHz may be quite reasonable; and
 * at 1 MHz (Fm+) a 1/430 second delay could easily be invisible.
 *
 * This value is forced to be a power of two so that writes align on pages.
 */
//static unsigned int io_limit = SFP_PAGE_SIZE;

/*
 * specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned int write_timeout = 250;

static char fpga_debug[MAX_DEBUG_INFO_LEN] = "fpga debug info:            \n";

struct sfp_platform_data {
    u32          byte_len;       /* size (sum of all addr) */
    u16          page_size;      /* for writes */
    u8           flags;
    void        *dummy1;        /* backward compatibility */
    void        *dummy2;        /* backward compatibility */

    /* dev_class: ONE_ADDR (QSFP) or TWO_ADDR (SFP) */
    int          dev_class;
    int          slave_addr;
    unsigned int write_max;
};

struct clounix_priv_data {
    struct mutex lock;
    void __iomem *mmio;
    struct sfp_platform_data chip[XCVR_PORT_MAX];
};

struct xcvr_driver_clx8000 {
    struct xcvr_fn_if xcvr_if;
    //private
    void __iomem *xcvr_base;
    struct clounix_priv_data dev;
};

inline static void fpga_reg_write(struct clounix_priv_data *priv, int reg, int val)
{
    writel(val, (priv->mmio + reg));
}
inline static int fpga_reg_read(struct clounix_priv_data *priv, int reg)
{
    return readl(priv->mmio + reg);
}

#define CPLD_BASE_ADDRESS 0x300
#define QSFP_CONFIG_ADDRESS_BASE         (CPLD_BASE_ADDRESS + 0x30)
#define QSFP_CONFIG_RESET_OFFSET         0
#define QSFP_CONFIG_POWER_MODE_OFFSET    8
#define QSFP_CONFIG_POWER_EN_OFFSET      16
#define QSFP_STATUS_ADDRESS_BASE         (CPLD_BASE_ADDRESS + 0x34)
#define QSFP_STATUS_PRESENT_OFFSET       0
#define QSFP_STATUS_IRQ_OFFSET           8
#define QSFP_STATUS_POWER_FAULT_OFFSET   16 
#define QSFP_START_PORT                  48

#define FPGA_PORT_BASE      (0x1000) 
#define FPGA_PORT_MGR0_CFG  (FPGA_PORT_BASE + 0x00)
#define FPGA_PORT_MGR0_CTRL (FPGA_PORT_BASE + 0x04)
#define FPGA_PORT_MGR0_STAT (FPGA_PORT_BASE + 0x08)
#define FPGA_PORT_MGR0_MUX  (FPGA_PORT_BASE + 0x10)

#define FPGA_PORT_MGR1_CFG  (FPGA_PORT_BASE + 0x20)
#define FPGA_PORT_MGR1_CTRL (FPGA_PORT_BASE + 0x24)
#define FPGA_PORT_MGR1_STAT (FPGA_PORT_BASE + 0x28)
#define FPGA_PORT_MGR1_MUX  (FPGA_PORT_BASE + 0x30)

#define FPGA_PORT_MGR2_CFG  (FPGA_PORT_BASE + 0x40)
#define FPGA_PORT_MGR2_CTRL (FPGA_PORT_BASE + 0x44)
#define FPGA_PORT_MGR2_STAT (FPGA_PORT_BASE + 0x48)
#define FPGA_PORT_MGR2_MUX  (FPGA_PORT_BASE + 0x50)

#define FPGA_MGR_RST    (1 << 31)
#define FPGA_MGR_ENABLE ((1 << 30) | (0x30 << 16) | (0xa1 << 8) | 0xa0) 
#define FPGA_MGR_RD     (0x81 << 24)
#define FPGA_MGR_WT     (0x84 << 24)

#define PORT_REG_1      (24)
#define PORT_REG_2      (48)

#define DSFP_RESET_ADDRESS_BASE       (CPLD_BASE_ADDRESS+0x10)
#define DSFP_LOW_POWER_ADDRESS_BASE   (CPLD_BASE_ADDRESS+0x14)
#define DSFP_IRQ_STATUS_ADDRESS_BASE  (CPLD_BASE_ADDRESS+0x18)
#define DSFP_PRESENT_ADDRESS_BASE     (CPLD_BASE_ADDRESS+0x1c)

#define GET_DSFP_RST_ADDRESS(idx, reg) \
            reg = (DSFP_RESET_ADDRESS_BASE + (0x10 * (idx /30))); 

#define GET_DSFP_LOWPOWER_ADDRESS(idx, reg) \
            reg = (DSFP_LOW_POWER_ADDRESS_BASE + (0x10 * (idx /30))); 

#define GET_DSFP_IRQ_STATUS_ADDRESS(idx, reg) \
            reg = (DSFP_IRQ_STATUS_ADDRESS_BASE + (0x10 * (idx /30)));

#define GET_DSFP_PRESENT_ADDRESS(idx, reg) \
            reg = (DSFP_PRESENT_ADDRESS_BASE + (0x10 * (idx /30)));

#endif //_XCVR_DRIVER_CLX8000_H_
