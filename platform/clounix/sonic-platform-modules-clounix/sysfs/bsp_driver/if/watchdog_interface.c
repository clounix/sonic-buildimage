#include "watchdog_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_watchdog_init(struct watchdog_fn_if **watchdog_driver);
extern void clx_driver_clx8000_watchdog_init(void **watchdog_driver);


struct watchdog_fn_if *watchdog_driver;

static struct driver_map watchdog_drv_map[] = {
	{"watchdog_clx8000", clx_driver_clx8000_watchdog_init},
};	


struct watchdog_fn_if *get_watchdog(void)
{
	return watchdog_driver;
}

void watchdog_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	printk(KERN_INFO "clx_driver_clx8000_watchdog_init\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_WATCHDOG);
    for (i = 0; i < sizeof(watchdog_drv_map)/sizeof(watchdog_drv_map[0]); i++)
    {
	    it = &watchdog_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&watchdog_driver);
	    }
    }
}

void watchdog_if_delete_driver(void) 
{
}

