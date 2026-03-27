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