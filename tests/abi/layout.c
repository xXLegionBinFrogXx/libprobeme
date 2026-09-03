/* ABI layout lock for probeme.h.
 *
 * Every field offset and struct size is pinned. If this file fails to
 * compile, the header changed incompatibly - bump PME_ABI_MINOR for
 * append-only changes and update the numbers here deliberately.
 *
 * Verified on: x86_64 (gcc, clang), aarch64 (gcc, clang). All snapshot
 * structs are pointer-free, so LP64 layout is identical everywhere. */
#include "probeme.h"

#include <stddef.h>

#define CHECK(expr) _Static_assert(expr, #expr);
#define SZ(t) _Static_assert(sizeof(struct t) == SZ_##t, #t);
#define OFF(t, f) _Static_assert(offsetof(struct t, f) == OFF_##t##_##f, #t);

/* pme_cpu_core: 8 x uint64_t */
#define SZ_pme_cpu_core 64
#define OFF_pme_cpu_core_user 0
#define OFF_pme_cpu_core_nice 8
#define OFF_pme_cpu_core_system 16
#define OFF_pme_cpu_core_idle 24
#define OFF_pme_cpu_core_iowait 32
#define OFF_pme_cpu_core_irq 40
#define OFF_pme_cpu_core_softirq 48
#define OFF_pme_cpu_core_steal 56
CHECK(sizeof(struct pme_cpu_core) == 8 * sizeof(uint64_t));
OFF(pme_cpu_core, user)
OFF(pme_cpu_core, nice)
OFF(pme_cpu_core, system)
OFF(pme_cpu_core, idle)
OFF(pme_cpu_core, iowait)
OFF(pme_cpu_core, irq)
OFF(pme_cpu_core, softirq)
OFF(pme_cpu_core, steal)

/* pme_cpu */
#define SZ_pme_cpu 16400
#define OFF_pme_cpu_n 0
#define OFF_pme_cpu_pad 4
#define OFF_pme_cpu_read_at_ns 8
#define OFF_pme_cpu_cpu 16
SZ(pme_cpu)
OFF(pme_cpu, n)
OFF(pme_cpu, pad)
OFF(pme_cpu, read_at_ns)
OFF(pme_cpu, cpu)
CHECK(sizeof(struct pme_cpu) == 16 + 256 * sizeof(struct pme_cpu_core));

/* pme_memory: 8 x uint64_t */
#define SZ_pme_memory 64
#define OFF_pme_memory_read_at_ns 0
#define OFF_pme_memory_total 8
#define OFF_pme_memory_free 16
#define OFF_pme_memory_available 24
#define OFF_pme_memory_buffers 32
#define OFF_pme_memory_cached 40
#define OFF_pme_memory_swap_total 48
#define OFF_pme_memory_swap_free 56
SZ(pme_memory)
OFF(pme_memory, read_at_ns)
OFF(pme_memory, total)
OFF(pme_memory, free)
OFF(pme_memory, available)
OFF(pme_memory, buffers)
OFF(pme_memory, cached)
OFF(pme_memory, swap_total)
OFF(pme_memory, swap_free)

/* pme_loadavg */
#define SZ_pme_loadavg 40
#define OFF_pme_loadavg_read_at_ns 0
#define OFF_pme_loadavg_load1 8
#define OFF_pme_loadavg_load5 16
#define OFF_pme_loadavg_load15 24
#define OFF_pme_loadavg_running 32
#define OFF_pme_loadavg_total 36
SZ(pme_loadavg)
OFF(pme_loadavg, read_at_ns)
OFF(pme_loadavg, load1)
OFF(pme_loadavg, load5)
OFF(pme_loadavg, load15)
OFF(pme_loadavg, running)
OFF(pme_loadavg, total)

/* pme_uptime: 3 x uint64_t */
#define SZ_pme_uptime 24
#define OFF_pme_uptime_read_at_ns 0
#define OFF_pme_uptime_uptime_s 8
#define OFF_pme_uptime_boot_time_unix_s 16
SZ(pme_uptime)
OFF(pme_uptime, read_at_ns)
OFF(pme_uptime, uptime_s)
OFF(pme_uptime, boot_time_unix_s)

/* pme_disk */
#define SZ_pme_disk 96
#define OFF_pme_disk_name 0
#define OFF_pme_disk_reads 32
#define OFF_pme_disk_read_bytes 40
#define OFF_pme_disk_read_time_ms 48
#define OFF_pme_disk_writes 56
#define OFF_pme_disk_write_bytes 64
#define OFF_pme_disk_write_time_ms 72
#define OFF_pme_disk_io_in_progress 80
#define OFF_pme_disk_io_time_ms 88
SZ(pme_disk)
OFF(pme_disk, name)
OFF(pme_disk, reads)
OFF(pme_disk, read_bytes)
OFF(pme_disk, read_time_ms)
OFF(pme_disk, writes)
OFF(pme_disk, write_bytes)
OFF(pme_disk, write_time_ms)
OFF(pme_disk, io_in_progress)
OFF(pme_disk, io_time_ms)

/* pme_disk_io */
#define SZ_pme_disk_io 6160
#define OFF_pme_disk_io_n 0
#define OFF_pme_disk_io_pad 4
#define OFF_pme_disk_io_read_at_ns 8
#define OFF_pme_disk_io_disks 16
SZ(pme_disk_io)
OFF(pme_disk_io, n)
OFF(pme_disk_io, pad)
OFF(pme_disk_io, read_at_ns)
OFF(pme_disk_io, disks)
CHECK(sizeof(struct pme_disk_io) == 16 + 64 * sizeof(struct pme_disk));

/* pme_mount */
#define SZ_pme_mount 400
#define OFF_pme_mount_device 0
#define OFF_pme_mount_mountpoint 64
#define OFF_pme_mount_fstype 320
#define OFF_pme_mount_flags 352
#define OFF_pme_mount_pad 356
#define OFF_pme_mount_size_bytes 360
#define OFF_pme_mount_free_bytes 368
#define OFF_pme_mount_avail_bytes 376
#define OFF_pme_mount_files 384
#define OFF_pme_mount_files_free 392
SZ(pme_mount)
OFF(pme_mount, device)
OFF(pme_mount, mountpoint)
OFF(pme_mount, fstype)
OFF(pme_mount, flags)
OFF(pme_mount, pad)
OFF(pme_mount, size_bytes)
OFF(pme_mount, free_bytes)
OFF(pme_mount, avail_bytes)
OFF(pme_mount, files)
OFF(pme_mount, files_free)

/* pme_filesystem */
#define SZ_pme_filesystem 25616
#define OFF_pme_filesystem_n 0
#define OFF_pme_filesystem_pad 4
#define OFF_pme_filesystem_read_at_ns 8
#define OFF_pme_filesystem_mounts 16
SZ(pme_filesystem)
OFF(pme_filesystem, n)
OFF(pme_filesystem, pad)
OFF(pme_filesystem, read_at_ns)
OFF(pme_filesystem, mounts)
CHECK(sizeof(struct pme_filesystem) == 16 + 64 * sizeof(struct pme_mount));

/* pme_iface */
#define SZ_pme_iface 80
#define OFF_pme_iface_name 0
#define OFF_pme_iface_rx_bytes 16
#define OFF_pme_iface_rx_packets 24
#define OFF_pme_iface_rx_errs 32
#define OFF_pme_iface_rx_drop 40
#define OFF_pme_iface_tx_bytes 48
#define OFF_pme_iface_tx_packets 56
#define OFF_pme_iface_tx_errs 64
#define OFF_pme_iface_tx_drop 72
SZ(pme_iface)
OFF(pme_iface, name)
OFF(pme_iface, rx_bytes)
OFF(pme_iface, rx_packets)
OFF(pme_iface, rx_errs)
OFF(pme_iface, rx_drop)
OFF(pme_iface, tx_bytes)
OFF(pme_iface, tx_packets)
OFF(pme_iface, tx_errs)
OFF(pme_iface, tx_drop)

/* pme_netdev */
#define SZ_pme_netdev 5136
#define OFF_pme_netdev_n 0
#define OFF_pme_netdev_pad 4
#define OFF_pme_netdev_read_at_ns 8
#define OFF_pme_netdev_ifaces 16
SZ(pme_netdev)
OFF(pme_netdev, n)
OFF(pme_netdev, pad)
OFF(pme_netdev, read_at_ns)
OFF(pme_netdev, ifaces)
CHECK(sizeof(struct pme_netdev) == 16 + 64 * sizeof(struct pme_iface));

/* pme_zone */
#define SZ_pme_zone 40
#define OFF_pme_zone_type 0
#define OFF_pme_zone_temp_mc 32
SZ(pme_zone)
OFF(pme_zone, type)
OFF(pme_zone, temp_mc)

/* pme_thermal */
#define SZ_pme_thermal 1296
#define OFF_pme_thermal_n 0
#define OFF_pme_thermal_pad 4
#define OFF_pme_thermal_read_at_ns 8
#define OFF_pme_thermal_zones 16
SZ(pme_thermal)
OFF(pme_thermal, n)
OFF(pme_thermal, pad)
OFF(pme_thermal, read_at_ns)
OFF(pme_thermal, zones)
CHECK(sizeof(struct pme_thermal) == 16 + 32 * sizeof(struct pme_zone));

/* pme_gpu_dev */
#define SZ_pme_gpu_dev 136
#define OFF_pme_gpu_dev_uuid 0
#define OFF_pme_gpu_dev_name 48
#define OFF_pme_gpu_dev_temp_c 112
#define OFF_pme_gpu_dev_power_mw 116
#define OFF_pme_gpu_dev_sm_clock_mhz 120
#define OFF_pme_gpu_dev_util_pct 124
#define OFF_pme_gpu_dev_pstate 128
#define OFF_pme_gpu_dev_pad 132
SZ(pme_gpu_dev)
OFF(pme_gpu_dev, uuid)
OFF(pme_gpu_dev, name)
OFF(pme_gpu_dev, temp_c)
OFF(pme_gpu_dev, power_mw)
OFF(pme_gpu_dev, sm_clock_mhz)
OFF(pme_gpu_dev, util_pct)
OFF(pme_gpu_dev, pstate)
OFF(pme_gpu_dev, pad)

/* pme_gpu */
#define SZ_pme_gpu 1104
#define OFF_pme_gpu_n 0
#define OFF_pme_gpu_pad 4
#define OFF_pme_gpu_read_at_ns 8
#define OFF_pme_gpu_gpus 16
SZ(pme_gpu)
OFF(pme_gpu, n)
OFF(pme_gpu, pad)
OFF(pme_gpu, read_at_ns)
OFF(pme_gpu, gpus)
CHECK(sizeof(struct pme_gpu) == 16 + 8 * sizeof(struct pme_gpu_dev));

/* pme_snapshot */
#define SZ_pme_snapshot 55864
#define OFF_pme_snapshot_size 0
#define OFF_pme_snapshot_abi_version 4
#define OFF_pme_snapshot_valid 8
#define OFF_pme_snapshot_truncated 16
#define OFF_pme_snapshot_cpu 24
#define OFF_pme_snapshot_memory 16424
#define OFF_pme_snapshot_loadavg 16488
#define OFF_pme_snapshot_uptime 16528
#define OFF_pme_snapshot_disk_io 16552
#define OFF_pme_snapshot_filesystem 22712
#define OFF_pme_snapshot_netdev 48328
#define OFF_pme_snapshot_thermal 53464
#define OFF_pme_snapshot_gpu 54760
SZ(pme_snapshot)
OFF(pme_snapshot, size)
OFF(pme_snapshot, abi_version)
OFF(pme_snapshot, valid)
OFF(pme_snapshot, truncated)
OFF(pme_snapshot, cpu)
OFF(pme_snapshot, memory)
OFF(pme_snapshot, loadavg)
OFF(pme_snapshot, uptime)
OFF(pme_snapshot, disk_io)
OFF(pme_snapshot, filesystem)
OFF(pme_snapshot, netdev)
OFF(pme_snapshot, thermal)
OFF(pme_snapshot, gpu)

/* pme_provider: LP64 only (contains pointers). */
CHECK(sizeof(struct pme_provider) == 48);
CHECK(offsetof(struct pme_provider, name) == 8);
CHECK(offsetof(struct pme_provider, capabilities) == 16);
CHECK(offsetof(struct pme_provider, init) == 24);
CHECK(offsetof(struct pme_provider, collect_all) == 32);
CHECK(offsetof(struct pme_provider, destroy) == 40);

int main(void)
{
    return 0;
}
