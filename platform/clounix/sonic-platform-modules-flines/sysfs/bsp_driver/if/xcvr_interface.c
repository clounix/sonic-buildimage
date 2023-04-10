#include "xcvr_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

extern int drv_xcvr_fpga_init(void **xcvr_driver);

struct xcvr_fn_if *xcvr_driver;

static struct driver_map xcvr_drv_map[] = {
	{"drv_xcvr_fpga", drv_xcvr_fpga_init, NULL},
};	

struct xcvr_fn_if *get_xcvr(void)
{
	return xcvr_driver;
}

int xcvr_if_create_driver(void)
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;
	struct board_info *bd;
	int rc = DRIVER_ERR;
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_XCVR);
    for (i = 0; i < sizeof(xcvr_drv_map)/sizeof(xcvr_drv_map[0]); i++)
    {
	    it = &xcvr_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    rc = it->driver_init((void *)&xcvr_driver);
	    }
    }

    //to be update, clx_driver_get_platform_bd is used for all module?
    bd = clx_driver_get_platform_bd();
    xcvr_driver->port_max = bd->xcvr.port_max;
    xcvr_driver->clk_div = bd->xcvr.clk_div;
	rc = xcvr_driver->dev_init(xcvr_driver);
    return rc;
}
void xcvr_if_delete_driver(void) 
{
}

