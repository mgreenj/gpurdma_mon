
# VectorFlow-GX Monitoring & Telemetry Driver

## Driver Compilation Unit
drivers/
    gpurdma_mon/
        uapi/
            gpurdma_mon.h    :userspace-safe: ioctl cmds, structs, event types
        core.c               :shared internals, data structures, locking
        debugfs.c            :debugfs interface, debug knobs
        gpurdma_mon.h        :kernel-internal: shared state, function prototypes,
        main.c               :kernel-only macros, struct definitions
        Makefile             :Makefile
        monitor.c            :poll/epoll, memory pressure notifications, free callback
        README               :This file
        telemetry.c          :PCIe bandwidth, BAR1 utilization, sysfs attributes

## Notes

Author: Maurice Green
Current Version: 1.0

Last Modification Date: 03/22/2026