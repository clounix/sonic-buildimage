#include "temp_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_temp_init(struct temp_fn_if **temp_driver);
extern void clx_driver_clx8000_temp_init(void **temp_driver);


struct temp_fn_if *temp_driver;

static struct driver_map temp_drv_map[] = {
	{"temp_clx8000", clx_driver_clx8000_temp_init},
};	


struct temp_fn_if *get_temp(void)
{
	return temp_driver;
}

void temp_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	printk(KERN_INFO "clx_driver_clx8000_temp_init\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_TEMP);
    for (i = 0; i < sizeof(temp_drv_map)/sizeof(temp_drv_map[0]); i++)
    {
	    it = &temp_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&temp_driver);
	    }
    }
}

void temp_if_delete_driver(void) 
{
}

