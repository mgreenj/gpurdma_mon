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

#include <linux/pci.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include "gpurdma_mon.h"

static int gpurdma_validate_pci(struct pci);
static u32 calculate_bandwidth(u16 lnksta)
{
    
    // I discovered that I can use the API func
    // pcie_capability_read_word for controls
    // defined in drivers/pci/access.c inside of
    // pcie_capability_reg_implemented function.
    // If the device has control of `pos`, the
    // word from register  at the pci device
    // capability offset, which is device during 
    // initialization. A read is performed at: 
    // capability offset, plus offset of the control
    // (i.e., `pos`). IIf that fails, `val` is 0
    // else, value. val == linksta. The inline
    // parameter tail call to pci_pcie_cap()
    // reduces search in PCI config space
    u32 speed = lnksta & PCI_EXP_LINKSTA_CLS;
    u32 width = (lnksta & PCI_EXP_LNKSTA_NLW) >> 4;
    u32 gbps_per_lane;

    switch (speed) {
    case 1: gbps_per_lane = 25;  break;
    case 2: gbps_per_lane = 50;  break;
    case 3: gbps_per_lane = 100; break;
    case 4: gbps_per_lane = 160; break;
    case 5: gbps_per_lane = 320; break;
    default: return 0;
    }

    return (gbps_per_lane * width) / 10;

}

static void gpurdma_bw_poll_work(struct work_struct *work)
{
    u16 lnksta;
    u32 bw_mbps;

    if (!gpurdma.gpu_pdev)
        goto reschedule;

    pcie_capability_read_word(gpurdma.gpu_pdev, PCI_EXP_LNKSTA, &lnksta);
    bw_mbps = calculate_bandwidth(lnksta);

    if (bw_mbps > gpurdma.bw_threshold)
        gpurdma_generate_event(GPURDMA_EVENT_BW_HIGH, bw_mbps);

reschedule:
    queue_delayed_work(gpurdma.wq, &gpurdma.bw_poll_work,
                       msecs_to_jiffies(100));
}

void gpurdma_check_numa(void)
{
    int gpu_numa, nic_numa;

    if (!gpurdma.gpu_pdev || !gpurdma.nic_pdev)
        return;

    gpu_numa = gpurdma.gpu_pdev->dev.numa_node;
    nic_numa = gpurdma.nic_pdev->dev.numa_node;

    if (gpu_numa != nic_numa)
        gpurdma_generate_event(GPURDMA_EVENT_NUMA_WARN,
                               ((u64)gpu_numa << 32) | nic_numa);
}

int gpurdma_telemetry_read(struct gpurdma_telemetry *out)
{
    u16 lnksta;

    memset(out, 0, sizeof(*out));

    if (!gpurdma.gpu_pdev || !gpurdma.nic_pdev)
        return -ENODEV;

    pcie_capability_read_word(gpurdma.gpu_pdev, PCI_EXP_LNKSTA, &lnksta);
    out->pcie_bandwidth_mbps   = calculate_bandwidth(lnksta);
    out->gpu_numa_node         = gpurdma.gpu_pdev->dev.numa_node;
    out->nic_numa_node         = gpurdma.nic_pdev->dev.numa_node;
    out->bar1_utilization_pct  = 0;

    return 0;
}

int gpurdma_telemetry_init(void)
{
    gpurdma.wq = create_singlethread_workqueue(DRIVER_NAME);
    if (!gpurdma.wq)
        return -ENOMEM;

    INIT_DELAYED_WORK(&gpurdma.bw_poll_work, gpurdma_bw_poll_work);
    queue_delayed_work(gpurdma.wq, &gpurdma.bw_poll_work,
                       msecs_to_jiffies(100));
    return 0;
}

void gpurdma_telemetry_fini(void)
{
    cancel_delayed_work_sync(&gpurdma.bw_poll_work);
    destroy_workqueue(gpurdma.wq);
}