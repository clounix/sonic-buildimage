#ifndef __KNET_FAULT_EVENT_H__
#define __KNET_FAULT_EVENT_H__

#include "knet_types.h"

#define KNET_FAULT_EVENT_MSG_SIZE 256

#define KNET_FAULT_EVENT_MGR_ENABLE (0x1UL << 0)
#define KNET_FAULT_EVENT_MGR_TEST (0x1UL << 1)

typedef enum {
    KNET_FAULT_EVENT_KENT_DMA_ALLOC_FAIL = 0,
    KNET_FAULT_EVENT_KENT_NO_AVAILABLE_DESC,
    KNET_FAULT_EVENT_KENT_DMA_MAP_SINGLE_FAIL,
    KNET_FAULT_EVENT_KENT_PCI_PROBE_FAIL,
} knet_fault_event_type_t;

typedef struct {
    knet_fault_event_type_t type;
    const char *name;
    const char *description;
    uint32_t fault_class;
    uint32_t fault_event;
} knet_fault_event_info_t;

const knet_fault_event_info_t *knet_fault_event_get_info(knet_fault_event_type_t type);
void knet_fault_event_report(knet_fault_event_type_t type);
void knet_fault_event_report_with_format(knet_fault_event_type_t type, const char *format, ...);
void knet_fault_event_report_custom(uint32_t fault_class, uint32_t fault_event, const char *format, ...);
#endif
