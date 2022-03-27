#include "cpld_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_cpld_init(struct cpld_fn_if **cpld_driver);
extern void clx_driver_clx8000_cpld_init(void **cpld_driver);
extern int g_dev_cpld_loglevel;

struct cpld_fn_if *cpld_driver;

static struct driver_map cpld_drv_map[] = {
	{"cpld_clx8000", clx_driver_clx8000_cpld_init},
};

struct cpld_fn_if *get_cpld(void)
{
	return cpld_driver;
}

void cpld_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	CPLD_INFO("clx_driver_clx8000_cpld_init\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_CPLD);
    for (i = 0; i < sizeof(cpld_drv_map)/sizeof(cpld_drv_map[0]); i++)
    {
	    it = &cpld_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&cpld_driver);
	    }
    }
//	__initcall_clx_driverA_cpld_init(&cpld_driver);

}
void cpld_if_delete_driver(void) 
{
}

