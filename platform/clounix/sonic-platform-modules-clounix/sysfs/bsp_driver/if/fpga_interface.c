#include "fpga_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_fpga_init(struct fpga_fn_if **fpga_driver);
extern void clx_driver_clx8000_fpga_init(void **fpga_driver);


struct fpga_fn_if *fpga_driver;

static struct driver_map fpga_drv_map[] = {
	{"fpga_clx8000", clx_driver_clx8000_fpga_init},
};	


struct fpga_fn_if *get_fpga(void)
{
	return fpga_driver;
}

void fpga_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_FPGA);
    for (i = 0; i < sizeof(fpga_drv_map)/sizeof(fpga_drv_map[0]); i++)
    {
	    it = &fpga_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&fpga_driver);
	    }
    }
//	__initcall_clx_driverA_fpga_init(&fpga_driver);

}
void fpga_if_delete_driver(void) 
{
}

