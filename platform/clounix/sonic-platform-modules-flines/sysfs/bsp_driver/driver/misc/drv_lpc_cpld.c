#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/iomap.h>
#include "clounix/clounix_fpga.h"
#include "lpc_syscpld_interface.h"

#define DRIVER_NAME "clounix_cpld_lpc"
#define CPLD_REG_MAX_IDX   0x13
#define CPLD_LPC_BASE  0x900
#define CPLD_LPC_SIZE 0x100

#ifdef CONFIG_X86
#define LPC_PCI_VENDOR_ID_INTEL  0x8086
#define LPC_PCI_DEVICE_ID_INTEL 0x8c54
#define LPC_PCI_VENDOR_ID_ZX  0x1d17
#define LPC_PCI_DEVICE_ID_ZX 0x1001
#endif
#ifdef CONFIG_ARCH_PHYTIUM
#define PHYTIUM_LPC_BASE 0x20000000
#define PHYTIUM_LPC_CONFIG_BASE 0x27FFFF00
#define PHYTIUM_LPC_CONFIG_SIZE 0x100
#define INT_APB_SPCE_CONF 0xFC
#define CLK_LPC_RSTN_O    0xE4
#define NU_SERIRQ_CONFIG  0xE8
#define INT_MASK          0xD8
#define START_CYCLE       0xD4

#define GPIO_MUX_PAD_BASE      0x28180200
#define GPIO_MUX_PAD_SIZE   16
#define GPIO_MUX_PAD_1_ADDR 4
#define GPIO_MUX_PAD_1_VALUE 0x84888441
#define GPIO_MUX_PAD_2_ADDR 8
#define GPIO_MUX_PAD_2_VALUE 0x18800048

#define ACCESS_TYPE_IO 0
#define ACCESS_TYPE_MEM 1
#define ACCESS_TYPE_FIRMWARE  2
#define ACCESS_TYPE_DMA 3

void __iomem *g_cpld_lpc_base;
struct lpc_pdata {
   void __iomem *lpc_base;
   resource_size_t lpc_base_phy;
   size_t lpc_size ;
};
#endif


#define SYS_CPLD_SW_TIME0_YEAR  0x1
#define SYS_CPLD_SW_TIME1_MON   0x2
#define SYS_CPLD_SW_TIME2_DAY   0x3
#define SYS_CPLD_SW_TIME3_HOUR  0x4
#define SYS_CPLD_HW_VERISON     0x5
#define SYS_CPLD_TEST           0x6
#define SYS_CPLD_PSU_STATUS     0x7
#define SYS_CPLD_PSU_POWER_ON   0x8
#define SYS_CPLD_FAN_STATUS     0x9
#define SYS_CPLD_POWER_EN       0xa
#define SYS_CPLD_POWER_GD       0xB
#define SYS_CPLD_CLK_EN         0xC
#define SYS_CPLD_BOARD_PRESENT  0xD
#define SYS_CPLD_RST            0xE
#define SYS_CPLD_RST_STORE      0xF
#define SYS_CPLD_RECONF         0x10
#define SYS_CPLD_EEPROM_WP      0x11
#define SYS_CPLD_ID_LED         0x12
#define SYS_CPLD_POWER_CYCLE    0x13
/* Bitfields in SYS_CPLD_HW_VERISON */
#define SYS_CPLD_HW_VERSION_OFFSET 0
#define SYS_CPLD_HW_VERSION_SIZE 3

/* Bitfields in SYS_CPLD_PSU_STATUS */
#define SYS_CPLD_PSU0_PRESENT_OFFSET 0
#define SYS_CPLD_PSU0_PRESENT_SIZE 1
#define SYS_CPLD_PSU1_PRESENT_OFFSET 1
#define SYS_CPLD_PSU1_PRESENT_SIZE 1
#define SYS_CPLD_PSU0_ACOK_OFFSET 2
#define SYS_CPLD_PSU0_ACOK_SIZE 1
#define SYS_CPLD_PSU1_ACOK_OFFSET 3
#define SYS_CPLD_PSU1_ACOK_SIZE 1
#define SYS_CPLD_PSU0_PWOK_OFFSET 4
#define SYS_CPLD_PSU0_PWOK_SIZE 1
#define SYS_CPLD_PSU1_PWOK_OFFSET 5
#define SYS_CPLD_PSU1_PWOK_SIZE 1
#define SYS_CPLD_PSU0_INT_OFFSET 6
#define SYS_CPLD_PSU0_INT_SIZE 1
#define SYS_CPLD_PSU1_INT_OFFSET 7
#define SYS_CPLD_PSU1_INT_SIZE 1

/* Bitfields in SYS_CPLD_PSU_POWER_ON */
#define SYS_CPLD_PSU0_POWER_ON_OFFSET 0
#define SYS_CPLD_PSU0_POWER_ON_SIZE 1
#define SYS_CPLD_PSU1_POWER_ON_OFFSET 1
#define SYS_CPLD_PSU1_POWER_ON_SIZE 1

/* Bitfields in SYS_CPLD_FAN_STATUS */
#define SYS_CPLD_FAN_PRESENT_OFFSET 0
#define SYS_CPLD_FAN_PRESENT_SIZE 1
#define SYS_CPLD_FAN_ALARM_OFFSET 1
#define SYS_CPLD_FAN_ALARM_SIZE 1

/* Bitfields in SYS_CPLD_POWER_EN */
#define SYS_CPLD_P12V_STBY_OFFSET 0
#define SYS_CPLD_P12V_STBY_SIZE 1
#define SYS_CPLD_P12V_CPU_OFFSET 1
#define SYS_CPLD_P12V_CPU_SIZE 1
#define SYS_CPLD_P12V_MAIN_OFFSET 2
#define SYS_CPLD_P12V_MAIN_SIZE 1
#define SYS_CPLD_P5V_CPU_OFFSET 3
#define SYS_CPLD_P5V_CPU_SIZE 1
#define SYS_CPLD_P3V3_PU_OFFSET 4
#define SYS_CPLD_P3V3_PU_SIZE 1
#define SYS_CPLD_P5V_USB_OFFSET 5
#define SYS_CPLD_P5V_USB_SIZE 1

/* Bitfields in SYS_CPLD_POWER_GD */
#define SYS_CPLD_PG_P12V_CPU_OFFSET 0
#define SYS_CPLD_PG_P12V_CPU_SIZE 1
#define SYS_CPLD_PG_P5V_CPU_OFFSET 1
#define SYS_CPLD_PG_P5V_CPU_SIZE 1
#define SYS_CPLD_PG_P3V3_PU_OFFSET 2
#define SYS_CPLD_PG_P3V3_PU_SIZE 1
#define SYS_CPLD_PG_PWOK_MAIN_OFFSET 3
#define SYS_CPLD_PG_PWOK_MAIN_SIZE 1
/* Bitfields in SYS_CPLD_BOARD_PRESENT */
#define SYS_CPLD_MAIN_BOARD_PRESENT_OFFSET 0
#define SYS_CPLD_MAIN_BOARD_PRESENT_SIZE 1
#define SYS_CPLD_BMC_BOARD_PRESENT_OFFSET 0
#define SYS_CPLD_BMC_BOARD_PRESENT_SIZE 1

/* Bitfields in SYS_CPLD_RST */
#define SYS_CPLD_RST_MAIN_BOARD_OFFSET 0
#define SYS_CPLD_RST_MAIN_BOARD_SIZE 1
#define SYS_CPLD_RST_FAN_BOARD_OFFSET 1
#define SYS_CPLD_RST_FAN_BOARD_SIZE 1
#define SYS_CPLD_RST_PCA9548_OFFSET 2
#define SYS_CPLD_RST_PCA9548_SIZE 1
#define SYS_CPLD_RST_NVME_OFFSET 3
#define SYS_CPLD_RST_NVME_SIZE 1
#define SYS_CPLD_RST_COMe_OFFSET 4
#define SYS_CPLD_RST_COMe_SIZE 1
/* Bitfields in SYS_CPLD_RST_STORE */
#define SYS_CPLD_RST_WDT_OFFSET 0
#define SYS_CPLD_RST_WDT_SIZE 1
#define SYS_CPLD_RST_BUTTON_OFFSET 1
#define SYS_CPLD_RST_BUTTON_SIZE 1

/* Bit manipulation macros */
#define SYS_CPLD_BIT(name)					\
	(1 << SYS_CPLD_##name##_OFFSET)
#define SYS_CPLD_BF(name,value)				\
	(((value) & ((1 << SYS_CPLD_##name##_SIZE) - 1))	\
	 << SYS_CPLD_##name##_OFFSET)
#define SYS_CPLD_BFEXT(name,value)\
	(((value) >> SYS_CPLD_##name##_OFFSET)		\
	 & ((1 << SYS_CPLD_##name##_SIZE) - 1))
#define SYS_CPLD_BFINS(name,value,old)			\
	(((old) & ~(((1 << SYS_CPLD_##name##_SIZE) - 1)	\
		    << SYS_CPLD_##name##_OFFSET))		\
	 | SYS_CPLD_BF(name,value))

#ifdef CONFIG_X86
u8 lpc_cpld_read_reg(u16 address)
{
    u8 reg_val;
    reg_val = inb(CPLD_LPC_BASE + (address & 0xff));
    
    LOG_DBG(CLX_DRIVER_TYPES_LPC, "cpld_base:0x%x, address:0x%x, value:0x%02x\n", 
        CPLD_LPC_BASE, address, reg_val);

    return reg_val;
}

void lpc_cpld_write_reg(u16 address, u8 reg_val)
{
    outb((reg_val & 0xff), CPLD_LPC_BASE + (address & 0xff));
    
    LOG_DBG(CLX_DRIVER_TYPES_LPC, "cpld_base:0x%x, address:0x%x, value:0x%02x\r\n", 
        CPLD_LPC_BASE, address, reg_val);
    return;
}
#endif
#ifdef CONFIG_ARCH_PHYTIUM
u8 lpc_cpld_read_reg(u32 address)
{
    u8 reg_val;
    reg_val =  readb(g_cpld_lpc_base + address);
    
    LOG_DBG(CLX_DRIVER_TYPES_LPC, "cpld_base:0x%p, address:0x%x, value:0x%02x\n", 
        g_cpld_lpc_base, address, reg_val);

    return reg_val;
}

void lpc_cpld_write_reg(u32 address, u8 reg_val)
{
    writeb(reg_val, g_cpld_lpc_base + address);
    
    LOG_DBG(CLX_DRIVER_TYPES_LPC, "cpld_base:0x%p, address:0x%x, value:0x%02x\r\n", 
        g_cpld_lpc_base, address, reg_val);
    return;
}
#endif
#if 0
void reset_mux_pca9548(void)
{
    uint32_t data=0;
    data = lpc_cpld_read_reg(SYS_CPLD_RST);
    
    CLEAR_BIT(data, SYS_CPLD_RST_PCA9548_OFFSET);
    lpc_cpld_write_reg(SYS_CPLD_RST,data);
    udelay(1);

    SET_BIT(data, SYS_CPLD_RST_PCA9548_OFFSET);
    lpc_cpld_write_reg(SYS_CPLD_RST,data);
    return;
}
EXPORT_SYMBOL_GPL(reset_mux_pca9548);
#endif
static ssize_t get_sys_cpld_all_reg(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 i = 0,value = 0 ,count = 0;
    u16 total = 0;
    for(i = 0 ;i < CPLD_REG_MAX_IDX+1; i++)
    {
        value = lpc_cpld_read_reg(i);
        count = sprintf(buf, "reg 0x%x:0x%02x\n", i,value);
        buf += count;
        total +=count;
    }
    return total;
}

static ssize_t get_sys_cpld_build_time(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 year = 0, mon = 0, day = 0 , hour = 0;
    year = lpc_cpld_read_reg(SYS_CPLD_SW_TIME0_YEAR);
    mon = lpc_cpld_read_reg(SYS_CPLD_SW_TIME1_MON);
    day = lpc_cpld_read_reg(SYS_CPLD_SW_TIME2_DAY);
    hour = lpc_cpld_read_reg(SYS_CPLD_SW_TIME3_HOUR);
    return sprintf(buf, "%d-%d-%d-%d\n", year,mon,day,hour);
}
static ssize_t get_sys_cpld_hw_verison(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 hw_verison = 0;
    hw_verison = lpc_cpld_read_reg(SYS_CPLD_HW_VERISON);
    return sprintf(buf, "0x%x\n",hw_verison);
}

static ssize_t get_sys_cpld_test(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_TEST);
    return sprintf(buf, "0x%x\n", value);;
}
static ssize_t set_sys_cpld_test(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    lpc_cpld_write_reg(SYS_CPLD_TEST,value);
    return count;
}

static ssize_t get_sys_cpld_psu_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fpga_device_attribute *attr = to_fpga_dev_attr(da);   
    u8 value = 0; 
    value = lpc_cpld_read_reg(SYS_CPLD_PSU_STATUS);
    if(0 == attr->index)
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU0_PRESENT,value));
    else
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU1_PRESENT,value));
}
static ssize_t get_sys_cpld_psu_acok(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fpga_device_attribute *attr = to_fpga_dev_attr(da);   
    u8 value = 0;
    value = lpc_cpld_read_reg(SYS_CPLD_PSU_STATUS);
    if(0 == attr->index)
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU0_ACOK,value));
    else
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU1_ACOK,value));
}
static ssize_t get_sys_cpld_psu_pwok(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fpga_device_attribute *attr = to_fpga_dev_attr(da);   
    u8 value = 0;
    value = lpc_cpld_read_reg(SYS_CPLD_PSU_STATUS);
    if(0 == attr->index)
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU0_PWOK,value));
    else
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU1_PWOK,value));
}
static ssize_t get_sys_cpld_psu_int(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fpga_device_attribute *attr = to_fpga_dev_attr(da);    
    u8 value = 0;
    value = lpc_cpld_read_reg(SYS_CPLD_PSU_STATUS);
    if(0 == attr->index)
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU0_INT,value));
    else
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU1_INT,value));
}
static ssize_t get_sys_cpld_psu_poweron(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct fpga_device_attribute *attr = to_fpga_dev_attr(da);   
    u8 value = 0;
    value = lpc_cpld_read_reg(SYS_CPLD_PSU_POWER_ON);
    if(0 == attr->index)
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU0_POWER_ON,value));
    else
        return sprintf(buf, "%x ", SYS_CPLD_BFEXT(PSU1_POWER_ON,value));
}
static ssize_t set_sys_cpld_psu_poweron(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    struct fpga_device_attribute *attr = to_fpga_dev_attr(da);   
     
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_PSU_POWER_ON);

    if(0 == attr->index)
        lpc_cpld_write_reg(SYS_CPLD_PSU_POWER_ON,SYS_CPLD_BFINS(PSU0_POWER_ON,value,old));
    else
        lpc_cpld_write_reg(SYS_CPLD_PSU_POWER_ON,SYS_CPLD_BFINS(PSU1_POWER_ON,value,old));
    return count;
}
static ssize_t get_sys_cpld_fan_present(struct device *dev, struct device_attribute *da,
             char *buf)
{ 
    u8 value = 0;
    value = lpc_cpld_read_reg(SYS_CPLD_FAN_STATUS);

    return sprintf(buf, "%x", SYS_CPLD_BFEXT(FAN_PRESENT,value));
}
static ssize_t get_sys_cpld_fan_alarm(struct device *dev, struct device_attribute *da,
             char *buf)
{ 
    u8 value = 0;
    value = lpc_cpld_read_reg(SYS_CPLD_FAN_STATUS);

    return sprintf(buf, "%x ", SYS_CPLD_BFEXT(FAN_ALARM,value));
}
static ssize_t get_sys_cpld_P12V_STBY_en(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(P12V_STBY,value));;
}
static ssize_t set_sys_cpld_P12V_STBY_en(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0, old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    lpc_cpld_write_reg(SYS_CPLD_POWER_EN,SYS_CPLD_BFINS(P12V_STBY,value,old));
    return count;
}
static ssize_t get_sys_cpld_P12V_CPU_en(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(P12V_CPU,value));;
}
static ssize_t set_sys_cpld_P12V_CPU_en(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0, old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    lpc_cpld_write_reg(SYS_CPLD_POWER_EN,SYS_CPLD_BFINS(P12V_CPU,value,old));
    return count;
}
static ssize_t get_sys_cpld_P12V_MAIN_en(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(P12V_MAIN,value));;
}
static ssize_t set_sys_cpld_P12V_MAIN_en(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    lpc_cpld_write_reg(SYS_CPLD_POWER_EN,SYS_CPLD_BFINS(P12V_MAIN,value,old));
    return count;
}
static ssize_t get_sys_cpld_P5V_CPU_en(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(P5V_CPU,value));;
}
static ssize_t set_sys_cpld_P5V_CPU_en(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0, old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    lpc_cpld_write_reg(SYS_CPLD_POWER_EN,SYS_CPLD_BFINS(P5V_CPU,value,old));
    return count;
}
static ssize_t get_sys_cpld_P3V3_PU_en(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(P3V3_PU,value));;
}
static ssize_t set_sys_cpld_P3V3_PU_en(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    lpc_cpld_write_reg(SYS_CPLD_POWER_EN,SYS_CPLD_BFINS(P3V3_PU,value,old));
    return count;
}
static ssize_t get_sys_cpld_P5V_USB_en(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(P5V_USB,value));;
}
static ssize_t set_sys_cpld_P5V_USB_en(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_POWER_EN);
    lpc_cpld_write_reg(SYS_CPLD_POWER_EN,SYS_CPLD_BFINS(P5V_USB,value,old));
    return count;
}
static ssize_t get_sys_cpld_PG_P12V_CPU(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_GD);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(PG_P12V_CPU,value));;
}
static ssize_t get_sys_cpld_PG_P5V_CPU(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_GD);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(PG_P5V_CPU,value));;
}
static ssize_t get_sys_cpld_PG_P3V3_PU(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_GD);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(PG_P3V3_PU,value));;
}
static ssize_t get_sys_cpld_PG_PWOK_MAIN(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_GD);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(PG_PWOK_MAIN,value));;
}
static ssize_t get_sys_cpld_clk_en(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_CLK_EN);
    return sprintf(buf, "0x%x\n", value);
}
static ssize_t set_sys_cpld_clk_en(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    lpc_cpld_write_reg(SYS_CPLD_CLK_EN,value);
    return count;
}
static ssize_t get_sys_cpld_main_board_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_BOARD_PRESENT);
    return sprintf(buf, "0x%x\n",  SYS_CPLD_BFEXT(MAIN_BOARD_PRESENT,value));
}
static ssize_t get_sys_cpld_bmc_board_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_BOARD_PRESENT);
    return sprintf(buf, "0x%x\n",  SYS_CPLD_BFEXT(BMC_BOARD_PRESENT,value));
}
static ssize_t get_sys_cpld_main_board_rst(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_RST);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(RST_MAIN_BOARD,value));;
}
static ssize_t set_sys_cpld_main_board_rst(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_RST);
    lpc_cpld_write_reg(SYS_CPLD_RST,SYS_CPLD_BFINS(RST_MAIN_BOARD,value,old));
    return count;
}
static ssize_t get_sys_cpld_fan_board_rst(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_RST);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(RST_FAN_BOARD,value));;
}
static ssize_t set_sys_cpld_fan_board_rst(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_RST);
    lpc_cpld_write_reg(SYS_CPLD_RST,SYS_CPLD_BFINS(RST_FAN_BOARD,value,old));
    return count;
}
static ssize_t get_sys_cpld_pca9548_rst(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_RST);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(RST_PCA9548,value));;
}
static ssize_t set_sys_cpld_pca9548_rst(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_RST);
    lpc_cpld_write_reg(SYS_CPLD_RST,SYS_CPLD_BFINS(RST_PCA9548,value,old));
    return count;
}
static ssize_t get_sys_cpld_nvme_rst(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_RST);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(RST_NVME,value));
}
static ssize_t set_sys_cpld_nvme_rst(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_RST);
    lpc_cpld_write_reg(SYS_CPLD_RST,SYS_CPLD_BFINS(RST_NVME,value,old));
    return count;
}
static ssize_t get_sys_cpld_COMe_rst(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_RST);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(RST_COMe,value));
}
static ssize_t set_sys_cpld_COMe_rst(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0,old = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    old = lpc_cpld_read_reg(SYS_CPLD_RST);
    lpc_cpld_write_reg(SYS_CPLD_RST,SYS_CPLD_BFINS(RST_COMe,value,old));
    return count;
}
static ssize_t get_sys_cpld_wdt_overflow(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_RST_STORE);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(RST_WDT,value));;
}
static ssize_t get_sys_cpld_button_push(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_RST_STORE);
    return sprintf(buf, "0x%x\n", SYS_CPLD_BFEXT(RST_BUTTON,value));;
}
static ssize_t get_sys_cpld_reconf(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_RECONF);
    GET_BIT(value, 0, value);
    return sprintf(buf, "0x%x\n", value);;
}
static ssize_t set_sys_cpld_reconf(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    lpc_cpld_write_reg(SYS_CPLD_RECONF,value);
    return count;
}
static ssize_t get_sys_cpld_eeprom_wp(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_EEPROM_WP);
    GET_BIT(value, 0, value);
    return sprintf(buf, "0x%x\n", value);;
}
static ssize_t set_sys_cpld_eeprom_wp(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    lpc_cpld_write_reg(SYS_CPLD_EEPROM_WP,value);
    return count;
}
static ssize_t get_sys_cpld_id_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_ID_LED);
    GET_BIT(value, 0, value);
    return sprintf(buf, "0x%x\n", value);;
}
static ssize_t set_sys_cpld_id_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    lpc_cpld_write_reg(SYS_CPLD_ID_LED,value);
    return count;
}
static ssize_t get_sys_cpld_power_cycle(struct device *dev, struct device_attribute *da,
             char *buf)
{
    u8 value = lpc_cpld_read_reg(SYS_CPLD_POWER_CYCLE);
    return sprintf(buf, "0x%x\n", value);;
}
static ssize_t set_sys_cpld_power_cycle(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    u32 value = 0;
    if (kstrtouint(buf, 16, &value))
    {
        return -EINVAL;
    }
    lpc_cpld_write_reg(SYS_CPLD_POWER_CYCLE,value);
    return count;
}
static FPGA_DEVICE_ATTR(all_cpld_reg, S_IRUGO, get_sys_cpld_all_reg, NULL, 0);
static FPGA_DEVICE_ATTR(build_time, S_IRUGO, get_sys_cpld_build_time, NULL, 0);
static FPGA_DEVICE_ATTR(hw_verison, S_IRUGO, get_sys_cpld_hw_verison, NULL, 0);
static FPGA_DEVICE_ATTR(test, S_IRUGO | S_IWUSR, get_sys_cpld_test, set_sys_cpld_test, 0);
/*PSU status start */
#define GET_SYS_CPLD_PSU_STATUS(_num) \
    static FPGA_DEVICE_ATTR(psu##_num##_present, S_IRUGO, get_sys_cpld_psu_present, NULL, _num);\
    static FPGA_DEVICE_ATTR(psu##_num##_acok, S_IRUGO, get_sys_cpld_psu_acok, NULL, _num);\
    static FPGA_DEVICE_ATTR(psu##_num##_pwok, S_IRUGO, get_sys_cpld_psu_pwok, NULL, _num);\
    static FPGA_DEVICE_ATTR(psu##_num##_int, S_IRUGO, get_sys_cpld_psu_int, NULL, _num)

GET_SYS_CPLD_PSU_STATUS(0);
GET_SYS_CPLD_PSU_STATUS(1);
/*PSU status end */
#define SYS_CPLD_PSU_POWERON(_num) \
    static FPGA_DEVICE_ATTR(psu##_num##_on, S_IRUGO| S_IWUSR, get_sys_cpld_psu_poweron, set_sys_cpld_psu_poweron, _num);
SYS_CPLD_PSU_POWERON(0);
SYS_CPLD_PSU_POWERON(1);
static FPGA_DEVICE_ATTR(fan_present, S_IRUGO, get_sys_cpld_fan_present, NULL, 0);
static FPGA_DEVICE_ATTR(fan_alarm, S_IRUGO, get_sys_cpld_fan_alarm, NULL, 0);
/*PW_EN*/
static FPGA_DEVICE_ATTR(P12V_STBY_EN, S_IRUGO | S_IWUSR, get_sys_cpld_P12V_STBY_en, set_sys_cpld_P12V_STBY_en, 0);
static FPGA_DEVICE_ATTR(P12V_CPU_EN, S_IRUGO | S_IWUSR, get_sys_cpld_P12V_CPU_en, set_sys_cpld_P12V_CPU_en, 0);
static FPGA_DEVICE_ATTR(P12V_MAIN_EN, S_IRUGO | S_IWUSR, get_sys_cpld_P12V_MAIN_en, set_sys_cpld_P12V_MAIN_en, 0);
static FPGA_DEVICE_ATTR(P5V_CPU_EN, S_IRUGO | S_IWUSR, get_sys_cpld_P5V_CPU_en, set_sys_cpld_P5V_CPU_en, 0);
static FPGA_DEVICE_ATTR(P3V3_PU_EN, S_IRUGO | S_IWUSR, get_sys_cpld_P3V3_PU_en, set_sys_cpld_P3V3_PU_en, 0);
static FPGA_DEVICE_ATTR(P5V_USB_EN, S_IRUGO | S_IWUSR, get_sys_cpld_P5V_USB_en, set_sys_cpld_P5V_USB_en, 0);
/*PW GD*/
static FPGA_DEVICE_ATTR(PG_P12V_CPU, S_IRUGO, get_sys_cpld_PG_P12V_CPU, NULL, 0);
static FPGA_DEVICE_ATTR(PG_P5V_CPU, S_IRUGO, get_sys_cpld_PG_P5V_CPU, NULL, 0);
static FPGA_DEVICE_ATTR(PG_P3V3_PU, S_IRUGO, get_sys_cpld_PG_P3V3_PU, NULL, 0);
static FPGA_DEVICE_ATTR(PG_PWOK_MAIN, S_IRUGO, get_sys_cpld_PG_PWOK_MAIN, NULL, 0);
/*CLK_EN*/
static FPGA_DEVICE_ATTR(CLK_EN, S_IRUGO | S_IWUSR, get_sys_cpld_clk_en, set_sys_cpld_clk_en, 0);

/*BOARD_PRESENT*/
static FPGA_DEVICE_ATTR(main_board_present, S_IRUGO , get_sys_cpld_main_board_present, NULL, 0);
static FPGA_DEVICE_ATTR(main_bmc_present, S_IRUGO , get_sys_cpld_bmc_board_present, NULL, 0);

/**RST*/
static FPGA_DEVICE_ATTR(rst_main_board, S_IRUGO | S_IWUSR, get_sys_cpld_main_board_rst, set_sys_cpld_main_board_rst, 0);
static FPGA_DEVICE_ATTR(rst_fan_board, S_IRUGO | S_IWUSR, get_sys_cpld_fan_board_rst, set_sys_cpld_fan_board_rst, 0);
static FPGA_DEVICE_ATTR(rst_pca9548, S_IRUGO | S_IWUSR, get_sys_cpld_pca9548_rst, set_sys_cpld_pca9548_rst, 0);
static FPGA_DEVICE_ATTR(rst_NVME, S_IRUGO | S_IWUSR, get_sys_cpld_nvme_rst, set_sys_cpld_nvme_rst, 0);
static FPGA_DEVICE_ATTR(rst_COMe, S_IRUGO | S_IWUSR, get_sys_cpld_COMe_rst, set_sys_cpld_COMe_rst, 0);
/**RST STORE*/
static FPGA_DEVICE_ATTR(wdt_overflow, S_IRUGO, get_sys_cpld_wdt_overflow, NULL, 0);
static FPGA_DEVICE_ATTR(button_push, S_IRUGO, get_sys_cpld_button_push, NULL, 0);

/*CPLD_RECONF*/
static FPGA_DEVICE_ATTR(cpld_reconf, S_IRUGO | S_IWUSR, get_sys_cpld_reconf, set_sys_cpld_reconf, 0);

/*EEPROM_WP*/
static FPGA_DEVICE_ATTR(eeprom_wp, S_IRUGO | S_IWUSR, get_sys_cpld_eeprom_wp, set_sys_cpld_eeprom_wp, 0);

/*ID_LED*/
static FPGA_DEVICE_ATTR(id_led, S_IRUGO | S_IWUSR, get_sys_cpld_id_led, set_sys_cpld_id_led, 0);
static FPGA_DEVICE_ATTR(power_cycle, S_IRUGO | S_IWUSR, get_sys_cpld_power_cycle, set_sys_cpld_power_cycle, 0);
static struct attribute *drv_lpc_cpld_attributes[] = {
    &fpga_dev_attr_all_cpld_reg.dev_attr.attr,
    &fpga_dev_attr_build_time.dev_attr.attr,
    &fpga_dev_attr_hw_verison.dev_attr.attr,
    &fpga_dev_attr_test.dev_attr.attr,
    &fpga_dev_attr_psu0_present.dev_attr.attr,
    &fpga_dev_attr_psu0_acok.dev_attr.attr,
    &fpga_dev_attr_psu0_pwok.dev_attr.attr,
    &fpga_dev_attr_psu0_int.dev_attr.attr,
    &fpga_dev_attr_psu1_present.dev_attr.attr,
    &fpga_dev_attr_psu1_acok.dev_attr.attr,
    &fpga_dev_attr_psu1_pwok.dev_attr.attr,
    &fpga_dev_attr_psu1_int.dev_attr.attr,
    &fpga_dev_attr_psu0_on.dev_attr.attr,
    &fpga_dev_attr_psu1_on.dev_attr.attr,
    &fpga_dev_attr_fan_present.dev_attr.attr,
    &fpga_dev_attr_fan_alarm.dev_attr.attr,

    &fpga_dev_attr_P12V_STBY_EN.dev_attr.attr,
    &fpga_dev_attr_P12V_CPU_EN.dev_attr.attr,
    &fpga_dev_attr_P12V_MAIN_EN.dev_attr.attr,
    &fpga_dev_attr_P5V_CPU_EN.dev_attr.attr,
    &fpga_dev_attr_P3V3_PU_EN.dev_attr.attr,
    &fpga_dev_attr_P5V_USB_EN.dev_attr.attr,
    
    &fpga_dev_attr_PG_P12V_CPU.dev_attr.attr,
    &fpga_dev_attr_PG_P5V_CPU.dev_attr.attr,
    &fpga_dev_attr_PG_P3V3_PU.dev_attr.attr,
    &fpga_dev_attr_PG_PWOK_MAIN.dev_attr.attr,
    &fpga_dev_attr_CLK_EN.dev_attr.attr,

    &fpga_dev_attr_main_board_present.dev_attr.attr,
    &fpga_dev_attr_main_bmc_present.dev_attr.attr,

    &fpga_dev_attr_rst_main_board.dev_attr.attr,
    &fpga_dev_attr_rst_fan_board.dev_attr.attr,
    &fpga_dev_attr_rst_pca9548.dev_attr.attr,
    &fpga_dev_attr_rst_NVME.dev_attr.attr,
    &fpga_dev_attr_rst_COMe.dev_attr.attr,
    
    &fpga_dev_attr_wdt_overflow.dev_attr.attr,
    &fpga_dev_attr_button_push.dev_attr.attr,

    &fpga_dev_attr_cpld_reconf.dev_attr.attr,

    &fpga_dev_attr_eeprom_wp.dev_attr.attr,
    &fpga_dev_attr_id_led.dev_attr.attr,
    &fpga_dev_attr_power_cycle.dev_attr.attr,

    NULL
};

static const struct attribute_group drv_lpc_cpld_group = { .attrs = drv_lpc_cpld_attributes};
#ifdef CONFIG_X86
int drv_lpc_syscpld_init(void **driver)
{
    struct pci_dev *pdev = NULL;
    uint32_t status = 0;
    printk("clounix_cpld_lpc_init\n");

    pdev = pci_get_device(LPC_PCI_VENDOR_ID_INTEL, LPC_PCI_DEVICE_ID_INTEL, pdev);
    if (pdev) {
        /* 
     * LPC I/F Generic Decode Range 4 Register for cpld1 0x0900-0x09FF
     */
        status = pci_write_config_dword(pdev, 0x90, 0xfc0901);
        if(status)
        { 
            LOG_ERR(CLX_DRIVER_TYPES_LPC, "pci_write_config_word error %d!\n",status);
            return status;
        } 
    } else {
        pdev = pci_get_device(LPC_PCI_VENDOR_ID_ZX, LPC_PCI_DEVICE_ID_ZX, pdev);
        if (pdev) {
            status = pci_write_config_word(pdev, 0x5C, 0x900);
            if(status)
            { 
                LOG_ERR(CLX_DRIVER_TYPES_LPC, "pci_write_config_word error %d!\n",status);
                return status;
            }  
            status = pci_write_config_byte(pdev, 0x66, 0x01);
            if(status)
            { 
                LOG_ERR(CLX_DRIVER_TYPES_LPC, "pci_write_config_byte error %d!\n",status);
                return status;
            }  
        } else {
            printk("pdev vend is 0x%x,deviceid 0x%x\n",pdev->vendor,pdev->device);
            return -ENODEV;
        }

    }
    if (!request_region(CPLD_LPC_BASE, CPLD_LPC_SIZE, "lpc_sys_cpld")) {
        printk("request_region 0x%x failed!\n", CPLD_LPC_BASE);
		return -EBUSY;
	}
	status = sysfs_create_group(&pdev->dev.kobj, &drv_lpc_cpld_group);
    if (status) {
        LOG_ERR(CLX_DRIVER_TYPES_LPC,  "lpc_pld_group error status %d\r\n", status);
        return status;
    }
    return 0; 
}
void drv_lpc_syscpld_exit(void **driver)
{
    struct pci_dev *pdev = pci_get_device(LPC_PCI_VENDOR_ID_INTEL, LPC_PCI_DEVICE_ID_INTEL, NULL);
    release_region(CPLD_LPC_BASE, CPLD_LPC_SIZE);
    if(pdev) {
        sysfs_remove_group(&pdev->dev.kobj, &drv_lpc_cpld_group);
    }
    else {
        pdev = pci_get_device(LPC_PCI_VENDOR_ID_ZX, LPC_PCI_DEVICE_ID_ZX, NULL);
        if(pdev)
            sysfs_remove_group(&pdev->dev.kobj, &drv_lpc_cpld_group);
    }    

}
#else
static int cpld_lpc_drv_probe(struct platform_device *pdev)
{
    uint32_t status = 0;
    struct resource *res;
    void __iomem *pad_base,*lpc_config_base;
    struct lpc_pdata *pdata = devm_kzalloc(&pdev->dev, sizeof(struct lpc_pdata),
                                            GFP_KERNEL);
    printk("clounix_cpld_lpc_probe\n");

    /* enable LPC*/
    pad_base = ioremap(GPIO_MUX_PAD_BASE,GPIO_MUX_PAD_SIZE);
    if (!pad_base) {
		LOG_ERR(CLX_DRIVER_TYPES_LPC,  "@0x%x: Unable to map LPC PAD registers\n", GPIO_MUX_PAD_BASE);
		return -ENOMEM;
	}
    writel(GPIO_MUX_PAD_1_VALUE, pad_base + GPIO_MUX_PAD_1_ADDR);
    writel(GPIO_MUX_PAD_2_VALUE, pad_base + GPIO_MUX_PAD_2_ADDR);
    iounmap(pad_base);
    LOG_DBG(CLX_DRIVER_TYPES_LPC, "Enable LPC PAD registers successful\n");
    lpc_config_base = ioremap(PHYTIUM_LPC_CONFIG_BASE,PHYTIUM_LPC_CONFIG_SIZE);
    if (!lpc_config_base) {
		LOG_ERR(CLX_DRIVER_TYPES_LPC,  "@0x%x: Unable to map LPC PAD registers\n", PHYTIUM_LPC_CONFIG_BASE);
		return -ENOMEM;
	}
        /*select memory access*/
    writel(1, lpc_config_base + CLK_LPC_RSTN_O);
    writel(ACCESS_TYPE_MEM, lpc_config_base + INT_APB_SPCE_CONF);
    writel(0, lpc_config_base + INT_MASK);
    writel(0x15, lpc_config_base + START_CYCLE);

    iounmap(lpc_config_base);
    LOG_DBG(CLX_DRIVER_TYPES_LPC, "Enable LPC config registers successful\n");
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (unlikely(!res)) {
        printk(KERN_ERR "Specified Resource Not Available...\n");
        return -ENODEV;
    }
    pdata->lpc_base_phy = res->start;
    pdata->lpc_size     = resource_size(res);
    pdata->lpc_base = devm_ioremap_resource(&pdev->dev, res);
	if (!pdata->lpc_base)
	{
        LOG_ERR(CLX_DRIVER_TYPES_LPC,  "@%p: Unable to map LPC  registers\n", res->start);
        return -ENOMEM;
    }
    g_cpld_lpc_base = pdata->lpc_base;
    LOG_DBG(CLX_DRIVER_TYPES_LPC, "@%p:  map LPC  registers succsess\n", pdata->lpc_base);
    platform_set_drvdata(pdev, pdata);

	status = sysfs_create_group(&pdev->dev.kobj, &drv_lpc_cpld_group);
    if (status) {
        LOG_ERR(CLX_DRIVER_TYPES_LPC,  "drv_lpc_cpld_group error status %d\r\n", status);
        return -ENODEV;
    }
    return 0;
}
static int cpld_lpc_drv_remove(struct platform_device *pdev)
{
    struct lpc_pdata *pdata = platform_get_drvdata(pdev);
    sysfs_remove_group(&pdev->dev.kobj, &drv_lpc_cpld_group);
    printk("clounix_cpld_lpc_remove\n");
    if(pdata)
    {
        kfree(pdata);
    }
    return 0;
}

static void cpld_lpc_dev_release( struct device * dev)
{
    return;
}
static struct resource cpld_lpc_resources[] = {
    {
        .start  = PHYTIUM_LPC_BASE + CPLD_LPC_BASE,
        .end    = PHYTIUM_LPC_BASE + CPLD_LPC_BASE + CPLD_LPC_SIZE,
        .flags  = IORESOURCE_MEM,
    },
};

static struct platform_device cpld_lpc_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(cpld_lpc_resources),
    .resource       = cpld_lpc_resources,
    .dev = {
        .release = cpld_lpc_dev_release,
    }
};

static struct platform_driver cpld_lpc_drv = {
    .probe  = cpld_lpc_drv_probe,
    .remove = cpld_lpc_drv_remove,
    .driver = {
        .name   = DRIVER_NAME,
    },
};

int drv_lpc_syscpld_init(void **driver)
{
    // Register platform device and platform driver
    platform_device_register(&cpld_lpc_dev);
    platform_driver_register(&cpld_lpc_drv);
    return 0;
}

void drv_lpc_syscpld_exit(void **driver)
{
    // Unregister platform device and platform driver
    platform_driver_unregister(&cpld_lpc_drv);
    platform_device_unregister(&cpld_lpc_dev);
}
#endif
