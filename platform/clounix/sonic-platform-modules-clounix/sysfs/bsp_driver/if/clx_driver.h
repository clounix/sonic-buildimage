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
	CLX_DRIVER_TYPES_XCVR,
	CLX_DRIVER_TYPES_SYSEEPROM,
	CLX_DRIVER_TYPES_FAN,
	CLX_DRIVER_TYPES_PSU,
	CLX_DRIVER_TYPES_TEMP,
	CLX_DRIVER_TYPES_CURR,
	CLX_DRIVER_TYPES_VOL,
	CLX_DRIVER_TYPES_WATCHDOG,
	CLX_DRIVER_TYPES_SYSLED,
	CLX_DRIVER_TYPES_MAX,
}driver_types_t;

#define dev_log(module_name, switch, level, fmt, args...) \
    do { \
        if((level) & (switch))  \
            printk(KERN_ERR"DEV %s: "fmt, module_name, ##args); \
    } while(0);

struct driver_map {
	char *name;
        //void (*driver_init)(void * driver);
        void (*driver_init)(void **driver);
};

char *clx_driver_identify(driver_types_t driver_type);
int clx_driver_init(void);

//define module ERROR CODE
#define DRIVER_OK 0
#define DRIVER_ERR (-1)
#define ERR_MODULLE_CPLD 0x1
#define ERR_MODULE_FPGA 0x2
#define ERR_MODULLE_XCVR 0x3

#define BOARD_NAME_LEN 32
struct bd_common {
	char name[BOARD_NAME_LEN];
};

struct bd_syseeprom {
	char name[BOARD_NAME_LEN];
};

struct bd_fan {
	char name[BOARD_NAME_LEN];
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
};

struct bd_temp {
	char name[BOARD_NAME_LEN];
};

struct bd_vol {
	char name[BOARD_NAME_LEN];
};

struct bd_curr {
	char name[BOARD_NAME_LEN];
};

struct bd_fpga {
	char name[BOARD_NAME_LEN];
};

struct bd_watchdog {
	char name[BOARD_NAME_LEN];
};

struct board_info {
	struct bd_common common;
	struct bd_syseeprom syse2p;
	struct bd_fan fan;
	struct bd_cpld cpld;
	struct bd_sysled sysled;
	struct bd_psu psu;
	struct bd_xcvr xcvr;
	struct bd_temp temp;
	struct bd_vol vol;
	struct bd_curr curr;
	struct bd_fpga fpga;
	struct bd_watchdog watchdog;
};

#endif //_CLX_DRIVER_H_

