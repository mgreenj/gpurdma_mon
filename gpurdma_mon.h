
#ifndef __GPURDMA_MON_H__
#define __GPURDMA_MON_H__

#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include "uapi/gpurdma_mon.h"


#define GPURDMA_STRINGIFY(s)           #s
#define GPURDMA_TOSTRING(s)            GPURDMA_STRINGIFY(s)

#define GPURDMA_MAJOR_VERSION_SHIFT    16

#define GPURDMA_MAJOR_VERSION    1
#define GPURDMA_MINOR_VERSION    0
#define GPURDMA_VERSION          ((GPURDMA_MAJOR_VERSION << GPURDMA_MAJOR_VERSION_SHIFT) | GPURDMA_MINOR_VERSION)
#define GPURDMA_VERSION_STRING   GPURDMA_TOSTRING(GPURDMA_MAJOR_VERSION) "." GPURDMA_TOSTRING(GPURDMA_MINOR_VERSION)

#define CHECK_RC(call, label, fmt, ...) \
    do { \
        res = (call); \
        if (res) { \
            fprintf(stderr, "%s: " fmt, __func__, ##__VA_ARGS__); \
            goto label; \
        } \
    } while (0)



/**
 * The gpurdma_client struct is used to manage state on a per-fd
 * basis. Each process that opens the gpurdma_mon drivers device
 * file (/dev/gpurdma_mon) will have a unique file descriptor and
 * each fd should correspond to a unique state. Otherwise, every
 * calling process would share state, which is not what we want.
 */
struct gpurdma_client {
    wait_queue_head_t   wq;
    atomic_t            event_count;
    u32                 throttle_pol;
    pid_t               pid;
};

struct gpurdma_state {
    struct pci_dev          *gpu_pdev;
    struct pci_dev          *nic_pdev;
    struct workqueue_struct *wq;
    struct delayed_work      bw_poll_work;
    u32                      bw_threshold;
    spinlock_t               lock;

    // list of clients to `fanout` events to open fd's.
    struct list_head         clients;
};

extern struct gpurdma_state gpurdma;
extern const struct file_operations gpurdma_fops;

#define GPURDMA_LIST_ADD(client)        \
do {                                    \
    spin_lock(&gpurdma.lock);           \
    list_add(&(client)->list,           \
             &gpurdma.clients);         \
    spin_unlock(&gpurdma.lock);         \
} while (0)

#define GPURDMA_LIST_DEL(client)        \
do {                                    \
    spin_lock(&gpurdma.lock);           \
    list_del(&(client)->list);          \
    spin_unlock(&gpurdma.lock);         \
} while (0)


int  gpurdma_core_init(void);
void gpurdma_core_fini(void);
void gpurdma_generate_event(u32 event_type, u64 details);

#ifdef CONFIG_DEBUG_FS
int  gpurdma_debugfs_init(void);
void gpurdma_debugfs_fini(void);
#else
static inline int  gpurdma_debugfs_init(void) { return 0; }
static inline void gpurdma_debugfs_fini(void) {}
#endif

#endif /* __GPURDMA_MON_H__ */