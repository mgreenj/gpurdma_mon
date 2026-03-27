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


#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "gpurdma_mon.h"

static int      gpurdma_open(struct inode *inode, struct file *filp);
static int      gpurdma_release(struct inode *inode, struct file *filp);
static ssize_t  gpurdma_read(struct file *filp, char __user *buf,
                            size_t count, loff_t *ppos);
static __poll_t gpurdma_poll(struct file *filp, poll_table *wait);
static long     gpurdma_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg);


const struct file_operations gpurdma_fops = {
    .owner          = THIS_MODULE,
    .open           = gpurdma_open,
    .release        = gpurdma_release,
    .read           = gpurdma_read,
    .poll           = gpurdma_poll,
    .unlocked_ioctl = gpurdma_ioctl,
};

void gpurdma_free_callback(void *data)
{
    gpurdma_generate_event(GPURDMA_EVENT_PRESSURE, 0);
}

static int gpurdma_open(struct inode *inode, struct file *filp)
{
    struct gpurdma_client *client;

    client = kzalloc(sizeof(*client), GFP_KERNEL);
    if (!client)
        return -ENOMEM;
    
    init_waitqueue_head(&client->wq);
    atomic_set(&client->event_count, 0);
    client->pid = current->pid;

    INIT_LIST_HEAD(&client->list);

    GPURDMA_LIST_ADD(client);

    // this stores in fd making the client accessible in other fops
    // initialized in gpurdma_ops
    filp->private_data = client;
    return 0;
}

static int gpurdma_release(struct inode *inode, struct file *filp)
{
    struct gpurdma_client *client = filp->private_data;

    GPURDMA_LIST_DEL(client);

    kfree(client);
    filp->private_data = NULL;
    return 0;
}

static ssize_t gpurdma_read(struct file *filp, char __user *buf,
                            size_t count, loff_t *ppos)
{
    struct gpurdma_client *client = filp->private_data;
    struct gpurdma_event event;

    if (count < sizeof(event))
        return -EINVAL;

    if (wait_event_interruptible(client->wq,
                                atomic_read(&client->event_count) > 0))
        return -ERESTARTSYS;
    
    // the above will block until a defined event is available
    // in the queue. When NVIDIA driver calls nvidida_p2p_get_pages()
    // a callback can be used if hardware-connected. There will also 
    // be a poll to check for events.

    memset(&event, 0, sizeof(event));

    if (copy_to_user(buf, &event, sizeof(event))
        return -EFAULT;

    atomic_dec(&client->event_count);
    return sizeof(event);
}

static __poll_t gpurdma_poll(struct file *filp, poll_table *wait)
{
    struct gpurdma_client *client = filp->private_data;

    poll_wait(filp, &client->wq, wait);

    if (atomic_read(&client->event_count) > 0)
        return EPOLLIN | EPOLLRDNORM;
    
    return 0;
}

static long gpurdma_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg)
{
    struct gpurdma_telemetry telem;

    switch (cmd) {
    case GPURDMA_IOC_GET_TELEMETRY:
        gpurdma_telemetry_read(&telem);
        if (copy_to_user((void __user *)arg, &telem, sizeof(telem)))
            return -EFAULT;
        return 0;
    
    case GPURDMA_IOC_SET_THROTTLE:
        if (get_user(gpurdma.bw_threshold, (__u32 __user *)arg))
            return -EFAULT;
        return 0;
    
    default:
        return -ENOTTY;
    }
}

int gpurdma_monitor_init(void){return 0;}

/* wait queues cleaned up per-client by gpurdma_release() */
void gpurdma_monitor_fini(void) {}