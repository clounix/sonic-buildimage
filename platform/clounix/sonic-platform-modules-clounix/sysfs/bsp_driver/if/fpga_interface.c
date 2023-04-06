#include "fpga_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

extern int clx_driver_fpga_common_init(void **fpga_driver);
extern int drv_fpga_anlogic_init(void **fpga_driver);
extern int drv_fpga_xilinx_init(void **fpga_driver);
extern void drv_fpga_anlogic_exit(void **fpga_driver);
extern void drv_fpga_xilinx_exit(void **fpga_driver);

struct fpga_fn_if *fpga_driver;

static struct driver_map fpga_drv_map[] = {
    {"drv_fpga_xilinx", drv_fpga_xilinx_init, drv_fpga_xilinx_exit},
    {"drv_fpga_anlogic", drv_fpga_anlogic_init, drv_fpga_anlogic_exit},
};


struct fpga_fn_if *get_fpga(void)
{
    return fpga_driver;
}

int fpga_if_create_driver(void)
{
    char *driver_type = NULL;
    struct driver_map *it;
    int i, ret;
    //get driver 

    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_FPGA);
    for (i = 0; i < sizeof(fpga_drv_map)/sizeof(fpga_drv_map[0]); i++)
    {
        it = &fpga_drv_map[i];
        if (strcmp((const char*)driver_type, (const char*)it->name) == 0)
        {
             ret = it->driver_init((void *)&fpga_driver);
             if (ret != 0)
                 return ret;
        }
    }
    
    clx_driver_fpga_common_init((void *)&fpga_driver);

    return 0;
}

void fpga_if_delete_driver(void) 
{
    char *driver_type = NULL;
    struct driver_map *it;
    int i;
    
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_FPGA);
    for (i = 0; i < sizeof(fpga_drv_map)/sizeof(fpga_drv_map[0]); i++) {
        it = &fpga_drv_map[i];
        if (strcmp((const char*)driver_type, (const char*)it->name) == 0) {
            it->driver_exit((void *)&fpga_driver);
            break;
        }
    }
}
