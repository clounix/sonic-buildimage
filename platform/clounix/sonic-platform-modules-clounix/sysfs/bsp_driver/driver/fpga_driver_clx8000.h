#ifndef _FPGA_DRIVER_CLX8000_H_
#define _FPGA_DRIVER_CLX8000_H_

#include "fpga_interface.h"

struct fpga_driver_clx8000 {
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

#endif //_FPGA_DRIVER_CLX8000_H_
