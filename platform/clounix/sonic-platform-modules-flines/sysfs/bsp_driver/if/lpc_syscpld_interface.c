#include "clx_driver.h"

extern int drv_lpc_syscpld_init(void **driver);
extern void drv_lpc_syscpld_exit(void **driver);

static struct driver_map lpc_syscpld_drv_map[] = {
    {"drv_lpc_cpld", drv_lpc_syscpld_init, drv_lpc_syscpld_exit},
};

struct lpc_syscpld_fn_if *lpc_syscpld_driver;

int lpc_syscpld_if_create_driver(void)
{
    char *driver_type = NULL;
    struct driver_map *it;
    int i;

    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_LPC);

    for (i=0; i<sizeof(lpc_syscpld_drv_map)/sizeof(struct driver_map); i++) {
        it = &lpc_syscpld_drv_map[i];
        if (strcmp(driver_type, it->name) == 0) {
            return it->driver_init((void *)&lpc_syscpld_driver);
        }
    }

    return -ENODATA;
}

void lpc_syscpld_if_delete_driver(void)
{
    char *driver_type = NULL;
    struct driver_map *it;
    int i;

    driver_type = clx_driver_identify(CLX_DRIVER_TYPES_LPC);

    for (i=0; i<sizeof(lpc_syscpld_drv_map)/sizeof(struct driver_map); i++) {
        it = &lpc_syscpld_drv_map[i];
        if (strcmp(driver_type, it->name) == 0) {
            it->driver_exit((void *)&lpc_syscpld_driver);
            break;
        }
    }

    return;
}
