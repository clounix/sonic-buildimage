#include "voltage_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_voltage_init(struct voltage_fn_if **voltage_driver);
extern void clx_driver_clx8000_voltage_init(void **voltage_driver);


struct voltage_fn_if *voltage_driver;

static struct driver_map voltage_drv_map[] = {
	{"voltage_clx8000", clx_driver_clx8000_voltage_init},
};	


struct voltage_fn_if *get_voltage(void)
{
	return voltage_driver;
}

void voltage_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	printk(KERN_INFO "clx_driver_clx8000_voltage_init\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_VOL);
    for (i = 0; i < sizeof(voltage_drv_map)/sizeof(voltage_drv_map[0]); i++)
    {
	    it = &voltage_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&voltage_driver);
	    }
    }
}

void voltage_if_delete_driver(void) 
{
}

