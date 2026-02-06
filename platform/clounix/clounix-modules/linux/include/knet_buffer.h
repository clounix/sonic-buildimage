#ifndef __CLX_BUFFER_H__
#define __CLX_BUFFER_H__
#include "knet_types.h"

extern struct list_head dma_buffer_list;
extern struct mutex dma_list_mutex;

int
clx_ioctl_dma_free(uint32_t unit, unsigned long arg);
int
clx_ioctl_dma_alloc(uint32_t unit, unsigned long arg);
int
cleanup_usr_dma_buffer(void);
#endif // __CLX_BUFFER_H__