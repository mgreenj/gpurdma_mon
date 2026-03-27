/*
 * Copyright (C) 2026 Maurice Green
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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