#include "gpurdma_mon.h"

struct gpurdma_state gpurdma = {
    .bw_threshold = 80,
};

void gpurdma_generate_event(u32 event_type, u64 details)
{
    struct gpurdma_client *client;
    struct gpurdma_event event = {
        .timestamp_ns = ktime_get_ns();
        .event_type = event_type,
        .details = details,
    };

    spin_lock(&gpurdma.lock);
    list_for_each_entry(client, &gpurdma.clients, list) {
        atomic_inc(&client->event_count);
        wake_up_interruptible(&client->wq);
    }
    spin_unlock(&gpurdma.lock);
}

int gpurdma_core_init(void)
{
    spin_lock_init(&gpurdma.lock);
    INIT_LIST_HEAD(&gpurdma.clients);
    return 0;
}

void gpurdma_core_fini(void) {}