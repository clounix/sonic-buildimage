#include <linux/module.h>

#include "clx_driver.h"
#include "clx_platform_interface.h"
#include "bd_info_ds410h.h"

extern int clx_driver_common_init(char *hw_platform);

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

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_syseeprom_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->syse2p.name;
}

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_xcvr_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->xcvr.name;
}

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_fan_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->fan.name;
}

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_watchdog_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->watchdog.name;
}

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_sysled_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->sysled.name;
}

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_psu_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->psu.name;
}

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_temp_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->temp.name;
}

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_curr_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->curr.name;
}

// 7xx/fpga/type "DRIVERA"
static char *clx_driver_vol_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->vol.name;
}
// 7xx/fpga/type "LPC"
static char *clx_driver_lpc_get_type(char *platform)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    return bd->lpc.name;
}

char *clx_driver_identify(driver_types_t driver_type)
{
    char *driver;

    char *platform = clx_driver_get_platform();
    switch (driver_type)
    {
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
    default:
        break;
    }
    return driver;
}

EXPORT_SYMBOL_GPL(clx_driver_identify);

static int flks_driver_ds410h_board(void)
{
    struct board_info *bd = clx_driver_get_platform_bd();
    char syse2p_name[] = "drv_syseeprom";
    char cpld_name[] = "drv_cpld_lattice";
    char fpga_name[] = "drv_fpga_xilinx";
    char i2c_master_name[] = "drv_i2c_master_xilinx";
    char xcvr_name[] = "drv_xcvr_fpga";
    char fan_name[] = "drv_fan_clx";
    char watchdog_name[] = "drv_wdt_cpld";
    char sysled_name[] = "drv_sysled_fpga";
    char psu_name[] = "drv_psu_clx";
    char temp_name[] = "drv_temp_sensor";
    char curr_name[] = "drv_curr_sensor";
    char vol_name[] = "drv_vol_sensor";

    // syseeprom info
    memcpy(bd->syse2p.name, syse2p_name, sizeof(syse2p_name));
    bd->syse2p.bus = SYSEEPROM_BUS_DS410H;
    bd->syse2p.addr = SYSEEPROM_ADDR_DS410H;
    bd->syse2p.size = SYSEEPROM_SIZE_DS410H;
    bd->syse2p.mux_addr = MUX_ADDR_DS410H;
    bd->syse2p.mux_channel = MUX_CH_SEL_DS410H;

    // CPLD info
    memcpy(bd->cpld.name, cpld_name, sizeof(cpld_name));
    // FPGA info
    memcpy(bd->fpga.name, fpga_name, sizeof(fpga_name));
    bd->fpga.reboot_eeprom_bus = CLX_DS410H_REBOOT_EEPROM_BUS;
    bd->fpga.reboot_eeprom_addr = CLX_DS410H_REBOOT_EEPROM_ADDR;
    // i2c master info
    memcpy(bd->i2c_master.name, i2c_master_name, sizeof(i2c_master_name));
    // transceiver info
    memcpy(bd->xcvr.name, xcvr_name, sizeof(xcvr_name));
    bd->xcvr.port_max = PORT_MAX_DS410H;
    bd->xcvr.port_platform_type = PORT_PLATFORM_DS410H;
    bd->xcvr.clk_div = PORT_CLK_DIV_DS410H;
    // fan info
    memcpy(bd->fan.name, fan_name, sizeof(fan_name));
    bd->fan.fan_num = FAN_MAX_DS410H;
    bd->fan.fan_max_speed = FAN_MAX_SPEED_DS410H;
    bd->fan.motor_per_fan = MOTOR_NUM_PER_FAN_DS410H;
    bd->fan.bus = FAN_BUS_DS410H;
    bd->fan.addr = FAN_ADDR_DS410H;
    // watchdog info
    memcpy(bd->watchdog.name, watchdog_name, sizeof(watchdog_name));
    // sysled info
    memcpy(bd->sysled.name, sysled_name, sizeof(sysled_name));
    // psu info
    memcpy(bd->psu.name, psu_name, sizeof(psu_name));
    // temp info
    memcpy(bd->temp.name, temp_name, sizeof(temp_name));
    memcpy(bd->temp.sensor_map_index, ds410h_sensor_map_index, sizeof(ds410h_sensor_map_index));
    // curr info
    memcpy(bd->curr.name, curr_name, sizeof(curr_name));
    bd->curr.total_sensor_num = DS410H_CURR_TOTAL_SENSOR_NUM;
    memcpy(bd->curr.curr_sensor_map, ds410h_curr_sensor_map, sizeof(ds410h_curr_sensor_map));
    memcpy(bd->curr.curr_index_range_map, ds410h_curr_index_range_map, sizeof(ds410h_curr_index_range_map));
    // vol info
    memcpy(bd->vol.name, vol_name, sizeof(vol_name));
    bd->vol.total_sensor_num = DS410H_VOL_TOTAL_SENSOR_NUM;
    memcpy(bd->vol.vol_sensor_map, ds410h_vol_sensor_map, sizeof(ds410h_vol_sensor_map));
    memcpy(bd->vol.vol_index_range_map, ds410h_vol_index_range_map, sizeof(ds410h_vol_index_range_map));

    LOG_INFO(CLX_DRIVER_TYPES_PLT, "syseeprom_if_create_driver\n");

    return DRIVER_OK;
}

static struct hw_platform_map platform_map[] = {
    {"x86_64-flines_ds410h_48y8c-r0", flks_driver_ds410h_board},
};
int clx_platform_check(char *platform)
{
    uint8_t i;
    int platform_valid = FALSE;
    struct hw_platform_map *it;

    for (i = 0; i < sizeof(platform_map) / sizeof(platform_map[0]); i++)
    {
        it = &platform_map[i];
        if (strcmp((const char *)platform, (const char *)it->name) == 0)
        {
            platform_valid = TRUE;
            break;
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

    // if (clx_driver_common_init((char *)hw_platform) != DRIVER_OK)
    // {
    //     hw_platform_valid = clx_platform_check((char *)hw_platform);
    // }

    // if (strcmp((const char *)platform, (const char *)hw_platform) != 0)
    // {
    //     // LOG_ERR(CLX_DRIVER_TYPES_PLT, "platform is not matched between %s and %s\n", platform, hw_platform);
    // }

    // if (platform_valid)
    // {
    //     memcpy(product, platform, strlen(platform));
    // }
    // else if (hw_platform_valid)
    // {
    //     memcpy(product, hw_platform, sizeof(product));
    // }
    // else
    // {
    //     // LOG_ERR(CLX_DRIVER_TYPES_PLT, "Both %s and %s is not valid\n", platform, hw_platform);
    //     // return DRIVER_ERR;
    // }

    strcpy(product, "x86_64-flines_ds410h_48y8c-r0");

    for (i = 0; i < sizeof(platform_map) / sizeof(platform_map[0]); i++)
    {
        it = &platform_map[i];
        LOG_DBG(CLX_DRIVER_TYPES_PLT, "Both %s and %s is valid\n", product, it->name);
        if (strcmp((const char *)product, (const char *)it->name) == 0)
        {
            ret = it->platform_init();
            LOG_DBG(CLX_DRIVER_TYPES_PLT, "platform_init OK\n");
            break;
        }
    }

    return ret;
}
EXPORT_SYMBOL_GPL(clx_driver_init);
