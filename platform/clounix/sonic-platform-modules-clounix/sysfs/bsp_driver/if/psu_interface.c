#include "psu_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

//extern void clx_driver_clx8000_psu_init(struct psu_fn_if **psu_driver);
extern void clx_driver_clx8000_psu_init(void **psu_driver);


struct psu_fn_if *psu_driver;

static struct driver_map psu_drv_map[] = {
	{"psu_clx8000", clx_driver_clx8000_psu_init},
};	


struct psu_fn_if *get_psu(void)
{
	return psu_driver;
}

void psu_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	printk(KERN_INFO "clx_driver_clx8000_psu_init\n");
    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_PSU);
    for (i = 0; i < sizeof(psu_drv_map)/sizeof(psu_drv_map[0]); i++)
    {
	    it = &psu_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    it->driver_init((void *)&psu_driver);
	    }
    }
}

void psu_if_delete_driver(void) 
{
}

