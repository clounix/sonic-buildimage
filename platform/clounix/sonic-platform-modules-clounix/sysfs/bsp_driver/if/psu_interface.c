#include "psu_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

extern int drv_psu_clx_init(void **psu_driver);


struct psu_fn_if *psu_driver;

static struct driver_map psu_drv_map[] = {
	{"drv_psu_clx", drv_psu_clx_init, NULL},
};	


struct psu_fn_if *get_psu(void)
{
	return psu_driver;
}

int psu_if_create_driver(void)
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	LOG_INFO(CLX_DRIVER_TYPES_PSU, "psu_if_create_driver\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_PSU);
    for (i = 0; i < sizeof(psu_drv_map)/sizeof(psu_drv_map[0]); i++)
    {
	    it = &psu_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    return it->driver_init((void *)&psu_driver);
	    }
    }

    return -ENODATA;
}

void psu_if_delete_driver(void) 
{
}

