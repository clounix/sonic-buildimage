#include "knet_fault_event.h"
#include "knet_dev.h"

#ifdef CLX_KNET_UKFEF
#include <linux/fault_event.h>
#else
enum FAULT_CLASS {
        SLIGHT_FAULT,
        NORMAL_FAULT,
        FATAL_FAULT,
        FAULT_CLASSS_MAX
};
enum FAULT_EVENT {
        FE_SOFTLOCKUP,
        FE_RCUSTALL,
        FE_HUNGTASK,
        FE_OOM_GLOBAL,
        FE_OOM_CGROUP,
        FE_ALLOCFAIL,
        FE_LIST_CORRUPT,
        FE_MM_STATE,
        FE_IO_ERR,
        FE_EXT4_ERR,
        FE_MCE,
        FE_SIGNAL,
        FE_WARN,
        FE_PANIC,
        FE_MAX
};
#endif

extern struct device_attribute dev_attr_test_fault_event;
uint32_t fault_event_mgr = KNET_FAULT_EVENT_MGR_ENABLE;

static const knet_fault_event_info_t knet_fault_event_info_table[] = {
    {
        .type = KNET_FAULT_EVENT_KENT_DMA_ALLOC_FAIL,
        .name = "KENT_DMA_ALLOC_FAIL",
        .description = "DMA packet allocation failed",
        .fault_class = NORMAL_FAULT,
        .fault_event = FE_WARN,
    },
    {
        .type = KNET_FAULT_EVENT_KENT_NO_AVAILABLE_DESC,
        .name = "KENT_TX_NO_AVAILABLE_DESCRIPTOR",
        .description = "No available descriptor",
        .fault_class = SLIGHT_FAULT,
        .fault_event = FE_WARN,
    },
    {
        .type = KNET_FAULT_EVENT_KENT_DMA_MAP_SINGLE_FAIL,
        .name = "KENT_DMA_MAP_SINGLE_FAIL",
        .description = "DMA map single failed",
        .fault_class = NORMAL_FAULT,
        .fault_event = FE_WARN,
    },
    {
        .type = KNET_FAULT_EVENT_KENT_PCI_PROBE_FAIL,
        .name = "KENT_PCI_PROBE_FAIL",
        .description = "PCI probe failed",
        .fault_class = FATAL_FAULT,
        .fault_event = FE_WARN,
    },
};

#define KNET_FAULT_EVENT_INFO_TABLE_SIZE \
    (sizeof(knet_fault_event_info_table) / sizeof(knet_fault_event_info_table[0]))

const knet_fault_event_info_t *knet_fault_event_get_info(knet_fault_event_type_t type)
{
    uint32_t i;
    for (i = 0; i < KNET_FAULT_EVENT_INFO_TABLE_SIZE; i++) {
        if (knet_fault_event_info_table[i].type == type) {
            return &knet_fault_event_info_table[i];
        }
    }
    return NULL;
}

void knet_fault_event_report(knet_fault_event_type_t type)
{
    knet_fault_event_report_with_format(type, NULL);
}

void knet_fault_event_report_with_format(knet_fault_event_type_t type, const char *format, ...)
{
    const knet_fault_event_info_t *info = NULL;
    char msg[KNET_FAULT_EVENT_MSG_SIZE];
    va_list args;

    if (!(fault_event_mgr & KNET_FAULT_EVENT_MGR_ENABLE)) {
        return;
    }

    info = knet_fault_event_get_info(type);
    if (info == NULL) {
        return;
    }
    memset(msg, 0, sizeof(msg));
    if (format && format[0] != '\0') {
        va_start(args, format);
        vsnprintf(msg, sizeof(msg), format, args);
        va_end(args);
    } else {
        strncpy(msg, info->description, sizeof(msg) - 1);
        msg[sizeof(msg) - 1] = '\0';
    }

#ifdef CLX_KNET_UKFEF
    report_fault_event(raw_smp_processor_id(), current, info->fault_class, info->fault_event, info->description);
#else
    dbg_print(DBG_CRIT, "fault_class: %d, fault_event: %d, description: %s\n",
              info->fault_class, info->fault_event, msg);
#endif
}

void knet_fault_event_report_custom(uint32_t fault_class, uint32_t fault_event, const char *format, ...)
{
    char msg[512];
    va_list args;

    if (!(fault_event_mgr & KNET_FAULT_EVENT_MGR_ENABLE)) {
        return;
    }

    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);
#ifdef CLX_KNET_UKFEF
    report_fault_event(raw_smp_processor_id(), current, fault_class, fault_event, msg);
#else
    dbg_print(DBG_CRIT, "Custom fault - class: %d, event: %d, msg: %s\n",
              fault_class, fault_event, msg);
#endif
}
