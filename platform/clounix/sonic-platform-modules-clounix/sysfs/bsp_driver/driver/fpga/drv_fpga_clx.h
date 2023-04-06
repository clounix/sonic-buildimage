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

#endif //_DRV_FPGA_CLX_H_
