#include "fan_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_fan_init(struct fan_fn_if **fan_driver);
extern void clx_driver_clx8000_fan_init(void **fan_driver);


struct fan_fn_if *fan_driver;

static struct driver_map fan_drv_map[] = {
	{"fan_clx8000", clx_driver_clx8000_fan_init},
};	


struct fan_fn_if *get_fan(void)
{
	return fan_driver;
}

void fan_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	printk(KERN_INFO "clx_driver_clx8000_fan_init\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_FAN);
    for (i = 0; i < sizeof(fan_drv_map)/sizeof(fan_drv_map[0]); i++)
    {
	    it = &fan_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&fan_driver);
	    }
    }
}

void fan_if_delete_driver(void) 
{
}

