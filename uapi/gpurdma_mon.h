#ifndef _UAPI_GPURDMA_MON_H
#define _UAPI_GPURDMA_MON_H

#include <linux/ioctl.h>
#include <linux/types.h>

/* Define Events to monitor */
#define GPURDMA_EVENT_PRESSURE    1
#define GPURDMA_EVENT_NUMA_WARN   2
#define GPURDMA_EVENT_BW_HIGH     3

struct gpurdma_event {
    __u64 timestamp_ns;
    __u32 event_type;
    __u32 severity;
    __u64 details;
};

#define GPURDMA_IOC_MAGIC           'G'
#define GPURDMA_IOC_GET_TELEMETRY   _IOR(GPURDMA_IOC_MAGIC, 1, struct gpurdma_telemetry)
#define GPURDMA_IOC_SET_THROTTLE    _IOW(GPURDMA_IOC_MAGIC, 2, __u32)

#endif /* _UAPI_GPURDMA_MON_H */