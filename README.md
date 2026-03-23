# gpurdma_mon Kernel Driver

This is a Linux Character Device driver that provides telemetry to monitor GPU memory pressure, and PCIe Peer-to-Peer bandwidth, used by `VectorFlow-GX`, experimental project exploring high-performance network packet processing on the GPU using NVIDIA GPUDirect RDMA. [You can read about and follow my progress on my blog](https://blog.mauricegreen.me/blogs/nvidia-gpu-rdma-packet-perf-1/). The GPU uses NVIDIA GPUDirect RDMA technology and DPDK for the first version. The second version replaces DPDK with NVIDIAs DOCA library; mainly GPUNetIO.

This is a work in progress. Currently three events are monitored, with plans to expand. I'm particularly interested in NUMA topology, and I'm exploring use of Linux's[NUMA memory policy API](https://docs.kernel.org/admin-guide/mm/numa_memory_policy.html) to improve GPU-NIC affinity in real-time.

