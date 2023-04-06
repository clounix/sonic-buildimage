#include "current_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

extern int drv_sensor_current_init(void **current_driver);


struct current_fn_if *current_driver;

static struct driver_map current_drv_map[] = {
	{"drv_curr_sensor", drv_sensor_current_init, NULL},
};	


struct current_fn_if *get_curr(void)
{
	return current_driver;
}

int current_if_create_driver(void)
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	printk(KERN_INFO "current_if_create_driver\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_CURR);
    for (i = 0; i < sizeof(current_drv_map)/sizeof(current_drv_map[0]); i++)
    {
	    it = &current_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    return it->driver_init((void *)&current_driver);
	    }
    }

    return -ENODATA;
}

void current_if_delete_driver(void) 
{
}

