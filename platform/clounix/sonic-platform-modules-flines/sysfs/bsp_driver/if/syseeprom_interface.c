#include "syseeprom_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

extern int drv_syseeprom_init(void **syseeprom_driver);
extern int drv_clx_syseeprom_init(void **syseeprom_driver);

struct syseeprom_fn_if *syseeprom_driver;

static struct driver_map syseeprom_drv_map[] = {
	{"drv_syseeprom", drv_syseeprom_init, NULL},
};	


struct syseeprom_fn_if *get_syseeprom(void)
{
	return syseeprom_driver;
}

int syseeprom_if_create_driver(void) 
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;
	struct board_info *bd;
	int rc = DRIVER_ERR;

    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_SYSEEPROM);
	LOG_DBG(CLX_DRIVER_TYPES_SYSEEPROM, "syseeprom_if_create_driver type:%s\n", driver_type);
    for (i = 0; i < sizeof(syseeprom_drv_map)/sizeof(struct driver_map); i++)
    {
	    it = &syseeprom_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    rc = it->driver_init((void *)&syseeprom_driver);
			break;
	    }
    }

    if (DRIVER_OK == rc) {
        bd = clx_driver_get_platform_bd();
        syseeprom_driver->bus = bd->syse2p.bus;
        syseeprom_driver->addr = bd->syse2p.addr;
		syseeprom_driver->size = bd->syse2p.size;
        syseeprom_driver->mux_addr = bd->syse2p.mux_addr;
        syseeprom_driver->mux_channel = bd->syse2p.mux_channel;
    }

    return rc;
//	__initcall_clx_driverA_syseeprom_init(&syseeprom_driver);

}
void syseeprom_if_delete_driver(void) 
{
}

