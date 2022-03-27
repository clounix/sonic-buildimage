#include "syseeprom_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_syseeprom_init(struct syseeprom_fn_if **syseeprom_driver);
extern void clx_driver_clx8000_syseeprom_init(void **syseeprom_driver);


struct syseeprom_fn_if *syseeprom_driver;

static struct driver_map syseeprom_drv_map[] = {
	{"syseeprom_clx8000", clx_driver_clx8000_syseeprom_init},
};	


struct syseeprom_fn_if *get_syseeprom(void)
{
	return syseeprom_driver;
}

void syseeprom_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	SYSE2_DBG("syseeprom_if_create_driver\n");
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_SYSEEPROM);
	SYSE2_DBG("syseeprom_if_create_driver type:%s\n", driver_type);
    for (i = 0; i < sizeof(syseeprom_drv_map)/sizeof(syseeprom_drv_map[0]); i++)
    {
	    it = &syseeprom_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&syseeprom_driver);
	    }
    }
//	__initcall_clx_driverA_syseeprom_init(&syseeprom_driver);

}
void syseeprom_if_delete_driver(void) 
{
}

