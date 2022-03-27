#include "sysled_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_sysled_init(struct sysled_fn_if **sysled_driver);
extern void clx_driver_clx8000_sysled_init(void **sysled_driver);


struct sysled_fn_if *sysled_driver;

static struct driver_map sysled_drv_map[] = {
	{"sysled_clx8000", clx_driver_clx8000_sysled_init},
};	


struct sysled_fn_if *get_sysled(void)
{
	return sysled_driver;
}

void sysled_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	SYSLED_INFO("clx_driver_clx8000_sysled_init\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_SYSLED);
    for (i = 0; i < sizeof(sysled_drv_map)/sizeof(sysled_drv_map[0]); i++)
    {
	    it = &sysled_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&sysled_driver);
	    }
    }
//	__initcall_clx_driverA_sysled_init(&sysled_driver);

}
void sysled_if_delete_driver(void) 
{
}

