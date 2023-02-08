#include "fan_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

extern int drv_fan_clx_init(void **fan_driver);

struct fan_fn_if *fan_driver;

static struct driver_map fan_drv_map[] = {
	{"drv_fan_clx", drv_fan_clx_init, NULL},
};	


struct fan_fn_if *get_fan(void)
{
	return fan_driver;
}

int fan_if_create_driver(void)
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;
	struct board_info *bd;
	int rc = DRIVER_ERR;

	printk(KERN_INFO "clx_driver_fan_init\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_FAN);
    for (i = 0; i < sizeof(fan_drv_map)/sizeof(fan_drv_map[0]); i++)
    {
	    it = &fan_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    rc = it->driver_init((void *)&fan_driver);
	    }
    }
    if (DRIVER_OK == rc) {
        bd = clx_driver_get_platform_bd();
        fan_driver->fan_num = bd->fan.fan_num;
        fan_driver->motor_per_fan = bd->fan.motor_per_fan;
        fan_driver->bus = bd->fan.bus;
        fan_driver->addr = bd->fan.addr;
    }	

    return rc;
}

void fan_if_delete_driver(void) 
{
}

