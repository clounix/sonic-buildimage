#include <linux/module.h>
#include "clx_driver.h"

//extern clx_driver_initcall_t __start_drv_initcalls;
//extern clx_driver_initcall_t __stop_drv_initcalls;
struct board_info prod_bd;

int g_dev_loglevel[CLX_DRIVER_TYPES_MAX] = {0, 0, 0, 0, 0, 0, 0 ,0, 0, 0};
EXPORT_SYMBOL_GPL(g_dev_loglevel);

struct board_info *clx_driver_get_platform_bd(void)
{
	return &prod_bd;
}
static char *clx_driver_get_platform(void)
{
	return "C48D8";
}

//to be updated for database
//7xx/cpld/type "DRIVERA"
static char *clx_driver_cpld_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->cpld.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_fpga_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->fpga.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_syseeprom_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->syse2p.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_xcvr_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->xcvr.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_fan_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->fan.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_watchdog_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->watchdog.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_sysled_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->sysled.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_psu_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->psu.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_temp_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->temp.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_curr_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->curr.name;
}

//7xx/fpga/type "DRIVERA"
static char *clx_driver_vol_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->vol.name;
}

char *clx_driver_identify(driver_types_t driver_type)
{
	char *driver;

	char *platform = clx_driver_get_platform();
	switch(driver_type) {
		case CLX_DRIVER_TYPES_CPLD:
			driver = clx_driver_cpld_get_type(platform);
			break;
		case CLX_DRIVER_TYPES_FPGA:
			driver = clx_driver_fpga_get_type(platform);		
			break;
		case CLX_DRIVER_TYPES_XCVR:
			driver = clx_driver_xcvr_get_type(platform);		
			break;
		case CLX_DRIVER_TYPES_SYSEEPROM:
			driver = clx_driver_syseeprom_get_type(platform);		
			break;
		case CLX_DRIVER_TYPES_FAN:
			driver = clx_driver_fan_get_type(platform);		
			break;
		case CLX_DRIVER_TYPES_WATCHDOG:
			driver = clx_driver_watchdog_get_type(platform);
			break;
		case CLX_DRIVER_TYPES_SYSLED:
			driver = clx_driver_sysled_get_type(platform);
			break;
		case CLX_DRIVER_TYPES_TEMP:
			driver = clx_driver_temp_get_type(platform);
			break;
		case CLX_DRIVER_TYPES_CURR:
			driver = clx_driver_curr_get_type(platform);
			break;
		case CLX_DRIVER_TYPES_VOL:
			driver = clx_driver_vol_get_type(platform);
			break;
		case CLX_DRIVER_TYPES_PSU:
			driver = clx_driver_psu_get_type(platform);
			break;																							
		default:
			break;
	}
	return driver;
}
EXPORT_SYMBOL_GPL(clx_driver_identify);

static int clx_driver_clx8000_board(void)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	char syse2p_name[] = "syseeprom_clx8000";	
	char cpld_name[] = "cpld_clx8000";
	char fpga_name[] = "fpga_clx8000";
	char xcvr_name[] = "xcvr_clx8000";
	char fan_name[] = "fan_clx8000";
	char watchdog_name[] = "watchdog_clx8000";
	char sysled_name[] = "sysled_clx8000";
	char psu_name[] = "psu_clx8000";
	char temp_name[] = "temp_clx8000";
	char curr_name[] = "current_clx8000";
	char vol_name[] = "voltage_clx8000";		

	//syseeprom info
	memcpy(bd->syse2p.name, syse2p_name, sizeof(syse2p_name));
	//CPLD info
	memcpy(bd->cpld.name, cpld_name, sizeof(cpld_name));
	//FPGA info
	memcpy(bd->fpga.name, fpga_name, sizeof(fpga_name));
	//transceiver info
	memcpy(bd->xcvr.name, xcvr_name, sizeof(xcvr_name));
	//fan info
	memcpy(bd->fan.name, fan_name, sizeof(fan_name));
	//watchdog info
	memcpy(bd->watchdog.name, watchdog_name, sizeof(watchdog_name));	
	//sysled info
	memcpy(bd->sysled.name, sysled_name, sizeof(sysled_name));
	//psu info
	memcpy(bd->psu.name, psu_name, sizeof(psu_name));
	//temp info
	memcpy(bd->temp.name, temp_name, sizeof(temp_name));
	//sysled info
	memcpy(bd->curr.name, curr_name, sizeof(curr_name));
	//sysled info
	memcpy(bd->vol.name, vol_name, sizeof(vol_name));
	printk(KERN_INFO "syseeprom_if_create_driver\n");

	return 0;
}

int clx_driver_init(void)
{
	int ret = DRIVER_OK;

	//get product information from eeprom?
	//initialization the const board info
	ret = clx_driver_clx8000_board();

	return ret;
}
EXPORT_SYMBOL_GPL(clx_driver_init);

void clx_driver_invoke_initcalls(void)
{
//	clx_driver_initcall_t *ic;

	//for (ic = &__start_drv_initcalls; ic < &__stop_drv_initcalls; ic++)
//	for (ic = &_drv_initcalls_start; ic < &_drv_initcalls_end; ic++)
//		(*ic)();
}

/* dummy driver initcall to make sure section exists */
#if 0
static void dummy_initcall(void)
{
	printk(KERN_INFO "dummy_initcall");
}
clx_driver_define_initcall(dummy_initcall);
#endif



