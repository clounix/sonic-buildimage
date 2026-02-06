#ifndef __KG_DRV_H__
#define __KG_DRV_H__

#include "knet_dev.h"

int
kg_driver_init(uint32_t unit, clx_drv_cb_t *ptr_clx_drv);
int
kg_driver_deinit(uint32_t unit);
int
kg_init_dma_driver(uint32_t unit);
void
kg_cleanup_dma_driver(uint32_t unit);

#endif // __KG_DRV_H__
