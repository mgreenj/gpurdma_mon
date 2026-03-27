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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/pci.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("maurice@mauricegreenjr.com");
MODULE_DESCRIPTION("Kernel-mode telemetry and memory pressure driver for systems using PCI P2P memory and GPUDirect RDMA");
MODULE_VERSION(GPURDMA_VERSION_STRING);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/sched/signal.h>
#endif

#include "gpurdma_mon.h"

#define PCI_VENDOR_ID_NVIDIA        (0x10de)
#define PCI_VENDOR_ID_MELLANOX      (0x15b3)

/* I'm using Misc framework for driver registration becuase device will not
   bind to any hardware device */
struct miscdevice gpurdma_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "gpurdma_mon",
    .fops = &gpurdma_fops,
};

static inline int gpurdma_mon_register(void)
{
    int ret = 0;

    CHECK_RC(gpurdma_core_init(),
             err_out, "core init failed: %d\n", rc);
    
    CHECK_RC(misc_register(&gpurdma_misc),
            err_core, "misc_register failed: %d\n", rc);

    gpurdma.gpu_pdev = pci_get_device(PCI_VENDOR_ID_NVIDIA, PCI_ANY_ID, NULL);
    if (!gpu_pdev) {
        pr_err("gpurdma_mon: NVIDIA GPU not found\n");
        goto err_misc;
    }

    gpurdma.nic_pdev = pci_get_device(PCI_VENDOR_ID_MELLANOX, PCI_ANY_ID, NULL);
    if (!nic_pdev) {
        pr_err("Aborting! Failed to find Mellanox NIC device");
        goto err_pci_dev;
    }

    CHECK_RC(gpurdma_monitor_init(),
             err_nic, "monitor init failed: %d\n", rc);

    CHECK_RC(gpurdma_telemetry_init(),
             err_monitor, "telemetry init failed: %d\n", rc);

    CHECK_RC(gpurdma_debugfs_init(),
             err_telemetry, "debugfs init failed: %d\n", rc);

err_telemetry:
    gpurdma_telemetry_fini();
err_monitor:
    gpurdma_monitor_fini();
err_nic:
    pci_dev_put(gpurdma.nic_pdev);
    gpurdma.nic_pdev = NULL;
err_gpu:
    pci_dev_put(gpurdma.gpu_pdev);
    gpurdma.gpu_pdev = NULL;
err_misc:
    misc_deregister(&gpurdma_misc);
err_core:
    gpurdma_core_fini();
err_out:
    return rc;
}


static inline gpurdma_mon_unregister(void)
{
    gpurdma_debugfs_fini();
    gpurdma_telemetry_fini();
    gpurdma_monitor_fini();
    pci_dev_put(gpurdma.nic_pdev);
    gpurdma.nic_pdev = NULL;
    pci_dev_put(gpurdma.gpu_pdev);
    gpurdma.gpu_pdev = NULL;
    misc_deregister(&gpurdma_misc);
    gpurdma_core_fini();
    pr_info("%s: unloaded\n", DRIVER_NAME);
}

static int __init gpurdma_mon_init(void)
{
    return gpurdma_mon_register();
}

static void __exit gpurdma_mon_exit(void)
{
    gpurdma_mon_unregister();
}

module_init(gpurdma_mon_init);
module_exit(gpurdma_mon_exit);


