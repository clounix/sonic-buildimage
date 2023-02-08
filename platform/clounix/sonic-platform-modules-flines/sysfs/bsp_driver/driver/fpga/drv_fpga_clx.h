#ifndef _DRV_FPGA_CLX_H_
#define _DRV_FPGA_CLX_H_

#include "fpga_interface.h"

struct fpga_driver_common {
    struct fpga_fn_if fpga_if;
    //private
    void __iomem *fpga_base;
    unsigned int index;
};

#define FPGA_CHIP_NUM 1
#define FPGA_BASE_ADDRESS           (0x0)

//register define
#define FPGA_HW_VERSION_ADDR           (0x0)
#define FPGA_FW_VERSION_ADDR           (0x4)

/*interrupt register define*/
#define FPGA_IRQ_CTRL_ADDR            0x700
#define FPGA_IRQ_STAT_ADDR            0x704
#define FPGA_IRQ_TRIG_ADDR            0x708
#define FPGA_CPLD0_INT_TRIG_ADDR      0x70C
#define FPGA_CPLD1_INT_TRIG_ADDR      0x710
#define FPGA_QDD_IRQ_TRIG_ADDR        0x714
#define FPGA_CPLD0_PRS_TRIG_ADDR      0x718
#define FPGA_CPLD1_PRS_TRIG_ADDR      0x71C
#define FPGA_DEBUG_TRIG_ADDR          0x010

#define IRQ_CTRL_RST_BIT              31
#define IRQ_CTRL_CFG_RST_BIT          0x8000007F
#define IRQ_CTRL_RST_PSU_BIT          0x00000001
#define IRQ_CTRL_RST_TEMP_BIT         0x00000002
#define IRQ_CTRL_RST_PMBUS_BIT        0x00000004
#define IRQ_CTRL_RST_USB_BIT          0x00000008
#define IRQ_CTRL_RST_FAN_BIT          0x00000010
#define IRQ_CTRL_RST_SPF_PRES_BIT     0x00000020
#define IRQ_CTRL_RST_SPF_INT_BIT      0x00000040
#define IRQ_CTRL_DISABLE_INT          0x0000007F
#define IRQ_CTRL_ENABLE_INT           0x00000000


#endif //_DRV_FPGA_CLX_H_
