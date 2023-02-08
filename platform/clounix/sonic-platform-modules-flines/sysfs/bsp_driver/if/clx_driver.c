#include <linux/module.h>

#include "clx_driver.h"
#include "clx_platform_interface.h"
#include "bd_info_clx8000.h"
#include "bd_info_clx12800.h"

extern int clx_driver_common_init(char *hw_platform);
//extern clx_driver_initcall_t __start_drv_initcalls;
//extern clx_driver_initcall_t __stop_drv_initcalls;
struct board_info prod_bd;

struct board_info *clx_driver_get_platform_bd(void)
{
    return &prod_bd;
}
EXPORT_SYMBOL_GPL(clx_driver_get_platform_bd);

static char *clx_driver_get_platform(void)
{
    return "C48D8";
}

static char *clx_driver_cpld_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->cpld.name;
}

static char *clx_driver_i2c_master_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->i2c_master.name;
}

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
static char *clx_driver_reboot_eeprom_get_type(char *platform)
{
	struct board_info *bd = clx_driver_get_platform_bd();
	return bd->reboote2p.name;
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
//7xx/fpga/type "LPC"
static char *clx_driver_lpc_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->lpc.name;
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
        case CLX_DRIVER_TYPES_I2C_MASTER:
            driver = clx_driver_i2c_master_get_type(platform);
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
        case CLX_DRIVER_TYPES_LPC:
            driver = clx_driver_lpc_get_type(platform);
            break;
		case CLX_DRIVER_TYPES_REBOOT_EEPROM:
			driver = clx_driver_reboot_eeprom_get_type(platform);
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
    char syse2p_name[] = "drv_syseeprom";
	char reboote2p_name[] = "reboot_eeprom_clx8000";
    char cpld_name[] = "drv_cpld_lattice";
    char fpga_name[] = "drv_fpga_xilinx";
    char i2c_master_name[] = "drv_i2c_xilinx";
    char xcvr_name[] = "drv_xcvr_fpga";
    char fan_name[] = "drv_fan_clx";
    char watchdog_name[] = "drv_wdt_cpld";
    char sysled_name[] = "drv_sysled_fpga";
    char psu_name[] = "drv_psu_clx";
    char temp_name[] = "drv_temp_sensor";
    char curr_name[] = "drv_curr_sensor";
    char vol_name[] = "drv_vol_sensor";

    //syseeprom info
    memcpy(bd->syse2p.name, syse2p_name, sizeof(syse2p_name));
    bd->syse2p.bus = SYSEEPROM_BUS_CLX8000;
    bd->syse2p.addr = SYSEEPROM_ADDR_CLX8000;
    bd->syse2p.size = SYSEEPROM_SIZE_CLX8000;
    bd->syse2p.mux_addr = MUX_ADDR_CLX8000;
    bd->syse2p.mux_channel = MUX_CH_SEL_CLX8000;
    //reboot eeprom info
	memcpy(bd->reboote2p.name, reboote2p_name, sizeof(reboote2p_name));

    //CPLD info
    memcpy(bd->cpld.name, cpld_name, sizeof(cpld_name));
    //FPGA info
    memcpy(bd->fpga.name, fpga_name, sizeof(fpga_name));
    //i2c master info
    memcpy(bd->i2c_master.name, i2c_master_name, sizeof(i2c_master_name));
    //transceiver info
    memcpy(bd->xcvr.name, xcvr_name, sizeof(xcvr_name));
    bd->xcvr.port_max = PORT_MAX_CLX8000;
    bd->xcvr.clk_div = PORT_CLK_DIV_CLX8000;
    //fan info
    memcpy(bd->fan.name, fan_name, sizeof(fan_name));
    bd->fan.fan_num = FAN_MAX_CLX8000;
    bd->fan.motor_per_fan = MOTOR_NUM_PER_FAN_CLX8000;
    bd->fan.bus = FAN_BUS_CLX8000;
    bd->fan.addr = FAN_ADDR_CLX8000;
    //watchdog info
    memcpy(bd->watchdog.name, watchdog_name, sizeof(watchdog_name));    
    //sysled info
    memcpy(bd->sysled.name, sysled_name, sizeof(sysled_name));
    //psu info
    memcpy(bd->psu.name, psu_name, sizeof(psu_name));
    //temp info
    memcpy(bd->temp.name, temp_name, sizeof(temp_name));
    memcpy(bd->temp.sensor_map_index, clx8000_sensor_map_index, sizeof(clx8000_sensor_map_index));
    //cuff info
    memcpy(bd->curr.name, curr_name, sizeof(curr_name));
    //sysled info
    memcpy(bd->vol.name, vol_name, sizeof(vol_name));
    LOG_INFO(CLX_DRIVER_TYPES_PLT, "syseeprom_if_create_driver\n");

    return DRIVER_OK;
}

static int clx_driver_clx12800_board(void)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    char syse2p_name[] = "drv_syseeprom";
    char cpld_name[] = "drv_cpld_lattice";
    char fpga_name[] = "drv_fpga_anlogic";
    char i2c_master_name[] = "drv_i2c_anlogic";
    char xcvr_name[] = "drv_xcvr_fpga";
    char fan_name[] = "drv_fan_clx";
    char watchdog_name[] = "drv_wdt_cpld";
    char sysled_name[] = "drv_sysled_fpga";
    char psu_name[] = "drv_psu_clx";
    char temp_name[] = "drv_temp_sensor";
    char curr_name[] = "drv_curr_sensor";
    char vol_name[] = "drv_vol_sensor";
    char lpc_name[] = "drv_lpc_cpld";

    //syseeprom info
    memcpy(bd->syse2p.name, syse2p_name, sizeof(syse2p_name));
    bd->syse2p.bus = SYSEEPROM_BUS_CLX12800;
    bd->syse2p.addr = SYSEEPROM_ADDR_CLX12800;
    bd->syse2p.size = SYSEEPROM_SIZE_CLX12800;
    bd->syse2p.mux_addr = MUX_ADDR_CLX12800;
    bd->syse2p.mux_channel = MUX_CH_SEL_CLX12800;
    //CPLD info
    memcpy(bd->cpld.name, cpld_name, sizeof(cpld_name));
    //FPGA info
    memcpy(bd->fpga.name, fpga_name, sizeof(fpga_name));
    //i2c master info
    memcpy(bd->i2c_master.name, i2c_master_name, sizeof(i2c_master_name));
    //transceiver info
    memcpy(bd->xcvr.name, xcvr_name, sizeof(xcvr_name));
    bd->xcvr.port_max = PORT_MAX_CLX128000;
    bd->xcvr.clk_div = PORT_CLK_DIV_CLX128000;
    //fan info
    memcpy(bd->fan.name, fan_name, sizeof(fan_name));
    bd->fan.fan_num = FAN_MAX_CLX128000;
    bd->fan.motor_per_fan = MOTOR_NUM_PER_FAN_CLX128000;
    bd->fan.bus = FAN_BUS_CLX128000;
    bd->fan.addr = FAN_ADDR_CLX128000;
    //watchdog info
    memcpy(bd->watchdog.name, watchdog_name, sizeof(watchdog_name));
    //sysled info
    memcpy(bd->sysled.name, sysled_name, sizeof(sysled_name));
    //psu info
    memcpy(bd->psu.name, psu_name, sizeof(psu_name));
    //temp info
    memcpy(bd->temp.name, temp_name, sizeof(temp_name));
    memcpy(bd->temp.sensor_map_index, clx12800_sensor_map_index, sizeof(clx12800_sensor_map_index));
    //sysled info
    memcpy(bd->curr.name, curr_name, sizeof(curr_name));
    //sysled info
    memcpy(bd->vol.name, vol_name, sizeof(vol_name));
    //lpc info
    memcpy(bd->lpc.name, lpc_name, sizeof(lpc_name));
    LOG_INFO(CLX_DRIVER_TYPES_PLT, "syseeprom_if_create_driver\n");

    return DRIVER_OK;
}

static struct hw_platform_map platform_map[] = {
    {"x86_64-clounix_clx8000_48c8d-r0", clx_driver_clx8000_board},
    {"clounix_clx12800_32d-r0", clx_driver_clx12800_board},
    {"x86_64-flines_ds610_48c8d-r0", clx_driver_clx8000_board},
};
int clx_platform_check(char *platform)
{
    uint8_t i;
    int platform_valid = FALSE;
    struct hw_platform_map *it;

    for (i = 0; i < sizeof(platform_map)/sizeof(platform_map[0]); i++) {
        it = &platform_map[i];
        if(strcmp((const char*)platform, (const char*)it->name) == 0)
        {
            platform_valid = TRUE;
        }
    }
    return platform_valid;
}

int clx_driver_init(char *platform)
{
    uint8_t i;
    int hw_platform_valid = FALSE;
    int platform_valid = FALSE;
    int ret = DRIVER_ERR;
    char hw_platform[PRODUCT_NAME_LEN_MAX];
    char product[PRODUCT_NAME_LEN_MAX];
    struct hw_platform_map *it;

    LOG_INFO(CLX_DRIVER_TYPES_PLT, "platform init %s\n", platform);
    platform_valid = clx_platform_check(platform);
    memset(hw_platform, 0, sizeof(hw_platform));
    memset(product, 0, sizeof(product));

    /*
    if (clx_driver_common_init((char *)hw_platform) != DRIVER_OK) {
        hw_platform_valid = clx_platform_check((char *)hw_platform);
    }
    */

    if (strcmp((const char*)platform, (const char*)hw_platform) != 0) {
        LOG_ERR(CLX_DRIVER_TYPES_PLT, "platform is not matched between %s and %s\n", platform, hw_platform);
    }

    if (platform_valid) {
        memcpy(product, platform, strlen(platform));
    }
    else if (hw_platform_valid) {
        memcpy(product, hw_platform, sizeof(product));
    }
    else {
        LOG_ERR(CLX_DRIVER_TYPES_PLT, "Both %s and %s is not valid\n", platform, hw_platform);
        return DRIVER_ERR;
    }

    for (i = 0; i < sizeof(platform_map)/sizeof(platform_map[0]); i++) {
        it = &platform_map[i];
        if(strcmp((const char*)product, (const char*)it->name) == 0)
        {
            ret = it->platform_init();
            break;
        }
    }

    return ret;
}
EXPORT_SYMBOL_GPL(clx_driver_init);

void clx_driver_invoke_initcalls(void)
{
//  clx_driver_initcall_t *ic;

    //for (ic = &__start_drv_initcalls; ic < &__stop_drv_initcalls; ic++)
//  for (ic = &_drv_initcalls_start; ic < &_drv_initcalls_end; ic++)
//      (*ic)();
}

/* dummy driver initcall to make sure section exists */
#if 0
static void dummy_initcall(void)
{
    printk(KERN_INFO "dummy_initcall");
}
clx_driver_define_initcall(dummy_initcall);
#endif
