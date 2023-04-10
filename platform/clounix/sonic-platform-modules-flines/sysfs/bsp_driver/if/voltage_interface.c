#include "voltage_interface.h"
#include "clx_driver.h"
// #include <linux/compiler.h>

extern int drv_sensor_voltage_init(void **voltage_driver);

struct voltage_fn_if *voltage_driver;

static struct driver_map voltage_drv_map[] = {
	{"drv_vol_sensor", drv_sensor_voltage_init, NULL},
};

struct voltage_fn_if *get_voltage(void)
{
	return voltage_driver;
}

int voltage_if_create_driver(void)
{
	char *driver_type = NULL;
	struct driver_map *it;
	struct board_info *bd;
	int i;
	int rc = DRIVER_ERR;

	printk(KERN_INFO "voltage_if_create_driver\n");
	// get driver
	driver_type = clx_driver_identify(CLX_DRIVER_TYPES_VOL);
	for (i = 0; i < sizeof(voltage_drv_map) / sizeof(voltage_drv_map[0]); i++)
	{
		it = &voltage_drv_map[i];
		if (strcmp((const char *)driver_type, (const char *)it->name) == 0)
		{
			rc = it->driver_init((void *)&voltage_driver);
		}
	}
    /**/

	if (DRIVER_OK == rc)
	{
		bd = clx_driver_get_platform_bd();
		voltage_driver->total_sensor_num = bd->vol.total_sensor_num;
		memcpy(voltage_driver->psensor_map,bd->vol.vol_sensor_map,sizeof(bd->vol.vol_sensor_map));
		memcpy(voltage_driver->pvol_index_range_map,bd->vol.vol_index_range_map, \
		sizeof(bd->vol.vol_index_range_map));		
	}

	return rc;
}

void voltage_if_delete_driver(void)
{
}
