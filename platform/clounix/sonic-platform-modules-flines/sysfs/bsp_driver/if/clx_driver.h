#ifndef _CLX_DRIVER_H_
#define _CLX_DRIVER_H_

#include <linux/module.h>
#include "device_driver_common.h"

//#include "cpld_interface.h"
/* driver init call prototype */
//typedef void (*clx_driver_initcall_t)(void);
#if 0
typedef void (*clx_driver_initcall_t)(struct cpld_fn_if *);
#else
typedef void (*clx_driver_initcall_t)(void *);
#endif


/* define driver init call */
#define clx_driver_define_initcall(fn) \
	static clx_driver_initcall_t __initcall_##fn \
		__attribute__ ((used)) \
		__attribute__((__section__("drv_initcalls"))) = fn;

typedef enum {
	CLX_DRIVER_TYPES_CPLD,
	CLX_DRIVER_TYPES_FPGA,
	CLX_DRIVER_TYPES_I2C_MASTER,
	CLX_DRIVER_TYPES_SLOT,
	CLX_DRIVER_TYPES_XCVR,
	CLX_DRIVER_TYPES_SYSEEPROM,
	CLX_DRIVER_TYPES_FAN,
	CLX_DRIVER_TYPES_PSU,
	CLX_DRIVER_TYPES_TEMP,
	CLX_DRIVER_TYPES_CURR,
	CLX_DRIVER_TYPES_VOL,
	CLX_DRIVER_TYPES_WATCHDOG,
	CLX_DRIVER_TYPES_SYSLED,
	CLX_DRIVER_TYPES_PLT,
	CLX_DRIVER_TYPES_LPC,
	CLX_DRIVER_TYPES_REBOOT_EEPROM,
	CLX_DRIVER_TYPES_MAX
}driver_types_t;

extern int g_dev_loglevel[CLX_DRIVER_TYPES_MAX];

#define dev_log(switch, level, fmt, args...) \
    do { \
        if((level) & (switch)) { \
            printk(KERN_ERR "%s[%d] " fmt, __func__, __LINE__, ##args); \
        } \
    } while(0);

#define LOG_INFO(idx, fmt, args...) dev_log(g_dev_loglevel[idx], INFO, fmt, ##args)
#define LOG_ERR(idx, fmt, args...) dev_log(g_dev_loglevel[idx], ERR, fmt, ##args)
#define LOG_DBG(idx, fmt, args...) dev_log(g_dev_loglevel[idx], DBG, fmt, ##args)

struct driver_map {
	char *name;
    int (*driver_init)(void **driver);
    void (*driver_exit)(void **driver);
};

char *clx_driver_identify(driver_types_t driver_type);
int clx_driver_init(char *platform);
struct board_info *clx_driver_get_platform_bd(void);

//define module ERROR CODE
#define DRIVER_OK 0
#define DRIVER_ERR (-1)
#define ERR_MODULLE_CPLD 0x1
#define ERR_MODULE_FPGA 0x2
#define ERR_MODULLE_XCVR 0x3

#define BOARD_NAME_LEN 32
#define SENSOR_MAP_INDEX_MAX 64

struct sensor_descript {
    unsigned char *adap_name;
    unsigned char addr;
    unsigned char *alias;
};

struct bd_common {
	char name[BOARD_NAME_LEN];
};

struct bd_syseeprom {
	char name[BOARD_NAME_LEN];
    unsigned char bus;
    unsigned char addr;
	unsigned int size;
    unsigned char mux_addr;
    unsigned char mux_channel;
};
struct bd_reboot_eeprom {
	char name[BOARD_NAME_LEN];
};
struct bd_fan {
	char name[BOARD_NAME_LEN];
	unsigned int fan_num;
        unsigned int motor_per_fan;
        unsigned char bus;
        unsigned char addr;
};

struct bd_cpld {
	char name[BOARD_NAME_LEN];
};

struct bd_sysled {
	char name[BOARD_NAME_LEN];
};

struct bd_psu {
	char name[BOARD_NAME_LEN];
};

struct bd_xcvr {
	char name[BOARD_NAME_LEN];
	unsigned int port_max;
    unsigned char clk_div;
};

struct bd_temp {
	char name[BOARD_NAME_LEN];
	struct sensor_descript sensor_map_index[SENSOR_MAP_INDEX_MAX];
};

struct bd_vol {
	char name[BOARD_NAME_LEN];
};

struct bd_curr {
	char name[BOARD_NAME_LEN];
};

struct bd_i2c_master {
	char name[BOARD_NAME_LEN];
};

struct bd_fpga {
	char name[BOARD_NAME_LEN];
};

struct bd_watchdog {
	char name[BOARD_NAME_LEN];
};

struct bd_lpc {
	char name[BOARD_NAME_LEN];
};

struct board_info {
	struct bd_common common;
	struct bd_syseeprom syse2p;
	struct bd_reboot_eeprom reboote2p;
	struct bd_fan fan;
	struct bd_cpld cpld;
	struct bd_sysled sysled;
	struct bd_psu psu;
	struct bd_xcvr xcvr;
	struct bd_temp temp;
	struct bd_vol vol;
	struct bd_curr curr;
	struct bd_fpga fpga;
	struct bd_i2c_master i2c_master;
	struct bd_watchdog watchdog;
	struct bd_lpc lpc;
};

struct hw_platform_map {
    char *name;
    int (*platform_init)(void);
};
#define PRODUCT_NAME_LEN_MAX 64

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define PRINT_LOGLEVEL(loglevel, buf, count) \
{ \
    return sprintf(buf, "the loglevel is :0x%x\n" \
            "    ERR   (0x01) is %s\n" \
            "    WARNING   (0x02) is %s\n" \
            "    INFO   (0x04) is %s\n" \
            "    DBG   (0x08) is %s\n" \
            "please use Dec or Hex data\n", \
            loglevel, \
            (loglevel & ERR)?"on":"off", \
            (loglevel & WARNING)?"on":"off", \
            (loglevel & INFO)?"on":"off", \
            (loglevel & DBG)?"on":"off"); \
}

#endif //_CLX_DRIVER_H_

