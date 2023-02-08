#ifndef _LPC_SYSCPLD_INTERFACE_H_
#define _LPC_SYSCPLD_INTERFACE_H_


#include "device_driver_common.h"
#include "clx_driver.h"

//interface for lpcsyscpld 

struct lpc_syscpld_fn_if {
    void *data;
};

int lpc_syscpld_if_create_driver(void);
void lpc_syscpld_if_delete_driver(void);
#endif //
