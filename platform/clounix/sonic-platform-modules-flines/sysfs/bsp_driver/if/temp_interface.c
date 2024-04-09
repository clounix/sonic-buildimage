#include "temp_interface.h"
#include "clx_driver.h"
//#include <linux/compiler.h>

extern int drv_sensor_temp_init(void **temp_driver);
extern int drv_sensor_temp_clx12800_init(void **temp_driver);

struct temp_fn_if *temp_driver;

static struct driver_map temp_drv_map[] = {
	{"drv_temp_sensor", drv_sensor_temp_init, NULL},
};	


struct temp_fn_if *get_temp(void)
{
	return temp_driver;
}

int temp_if_create_driver(void)
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;
	struct board_info *bd;
	int rc = DRIVER_ERR;

    //get driver 
    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_TEMP);
    for (i = 0; i < sizeof(temp_drv_map)/sizeof(temp_drv_map[0]); i++)
    {
	    it = &temp_drv_map[i];
	    if(strcmp((const char*)driver_type, (const char*)it->name) == 0)
	    {
		    rc = it->driver_init((void *)&temp_driver);
			break;
	    }
    }

	if (DRIVER_OK == rc) {
        bd = clx_driver_get_platform_bd();
		memcpy(temp_driver->sensor_map_index, bd->temp.sensor_map_index, sizeof(temp_driver->sensor_map_index));
    }

    return rc;
}

void temp_if_delete_driver(void) 
{
}
