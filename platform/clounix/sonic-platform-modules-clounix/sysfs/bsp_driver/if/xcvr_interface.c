#include "xcvr_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_xcvr_init(struct xcvr_fn_if **xcvr_driver);
extern void clx_driver_clx8000_xcvr_init(void **xcvr_driver);

struct xcvr_fn_if *xcvr_driver;

static struct driver_map xcvr_drv_map[] = {
	{"xcvr_clx8000", clx_driver_clx8000_xcvr_init},
};	


struct xcvr_fn_if *get_xcvr(void)
{
	return xcvr_driver;
}

void xcvr_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_XCVR);
    for (i = 0; i < sizeof(xcvr_drv_map)/sizeof(xcvr_drv_map[0]); i++)
    {
	    it = &xcvr_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&xcvr_driver);
	    }
    }
//	__initcall_clx_driverA_xcvr_init(&xcvr_driver);

}
void xcvr_if_delete_driver(void) 
{
}

