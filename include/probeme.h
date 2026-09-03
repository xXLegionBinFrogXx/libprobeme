
#ifndef PROBEME_H
#define PROBEME_H

/* probeme: system information snapshot ABI for Linux.
 * Providers only write their own sections and OR their bits into
 * valid/truncated; the caller resets valid/truncated before each refresh
 * cycle, so several providers can fill one snapshot in any order. */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define PME_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define PME_EXPORT __attribute__((visibility("default")))
#else
#define PME_EXPORT
#endif

#define PME_ABI_MAJOR 1
#define PME_ABI_MINOR 0

PME_EXPORT uint32_t pme_abi_version(void);

#define PME_CAP_CPU        (1u << 0)
#define PME_CAP_MEMORY     (1u << 1)
#define PME_CAP_LOADAVG    (1u << 2)
#define PME_CAP_UPTIME     (1u << 3)
#define PME_CAP_DISK_IO    (1u << 4)
#define PME_CAP_FILESYSTEM (1u << 5)
#define PME_CAP_NETDEV     (1u << 6)
#define PME_CAP_THERMAL    (1u << 7)
#define PME_CAP_GPU        (1u << 8)

#define PME_OK       0
#define PME_ENOTSUP -1
#define PME_EIO     -2
#define PME_EINVAL  -3
#define PME_ENOINIT -4

#define PME_CFG_FS_INCLUDE_REMOTE (1ull << 0)

struct pme_config {
    uint32_t size;
    uint32_t reserved;
    uint64_t flags;
};

#define PME_MOUNT_RO      (1u << 0)
#define PME_MOUNT_SKIPPED (1u << 1)

struct pme_cpu_core {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
};

struct pme_cpu {
    uint32_t n;
    uint32_t pad;
    uint64_t read_at_ns;
    struct pme_cpu_core cpu[256];
};

struct pme_memory {
    uint64_t read_at_ns;
    uint64_t total;
    uint64_t free;
    uint64_t available;
    uint64_t buffers;
    uint64_t cached;
    uint64_t swap_total;
    uint64_t swap_free;
};

struct pme_loadavg {
    uint64_t read_at_ns;
    double load1;
    double load5;
    double load15;
    uint32_t running;
    uint32_t total;
};

struct pme_uptime {
    uint64_t read_at_ns;
    uint64_t uptime_s;
    uint64_t boot_time_unix_s;
};

struct pme_disk {
    char name[32];
    uint64_t reads;
    uint64_t read_bytes;
    uint64_t read_time_ms;
    uint64_t writes;
    uint64_t write_bytes;
    uint64_t write_time_ms;
    uint64_t io_in_progress;
    uint64_t io_time_ms;
};

struct pme_disk_io {
    uint32_t n;
    uint32_t pad;
    uint64_t read_at_ns;
    struct pme_disk disks[64];
};

struct pme_mount {
    char device[64];
    char mountpoint[256];
    char fstype[32];
    uint32_t flags;
    uint32_t pad;
    uint64_t size_bytes;
    uint64_t free_bytes;
    uint64_t avail_bytes;
    uint64_t files;
    uint64_t files_free;
};

struct pme_filesystem {
    uint32_t n;
    uint32_t pad;
    uint64_t read_at_ns;
    struct pme_mount mounts[64];
};

struct pme_iface {
    char name[16];
    uint64_t rx_bytes;
    uint64_t rx_packets;
    uint64_t rx_errs;
    uint64_t rx_drop;
    uint64_t tx_bytes;
    uint64_t tx_packets;
    uint64_t tx_errs;
    uint64_t tx_drop;
};

struct pme_netdev {
    uint32_t n;
    uint32_t pad;
    uint64_t read_at_ns;
    struct pme_iface ifaces[64];
};

struct pme_zone {
    char type[32];
    int64_t temp_mc;
};

struct pme_thermal {
    uint32_t n;
    uint32_t pad;
    uint64_t read_at_ns;
    struct pme_zone zones[32];
};

struct pme_gpu_dev {
    char uuid[48];
    char name[64];
    uint32_t temp_c;
    uint32_t power_mw;
    uint32_t sm_clock_mhz;
    uint32_t util_pct;
    uint32_t pstate;
    uint32_t pad;
};

struct pme_gpu {
    uint32_t n;
    uint32_t pad;
    uint64_t read_at_ns;
    struct pme_gpu_dev gpus[8];
};

struct pme_snapshot {
    uint32_t size;
    uint32_t abi_version;
    uint64_t valid;
    uint64_t truncated;
    struct pme_cpu        cpu;
    struct pme_memory     memory;
    struct pme_loadavg    loadavg;
    struct pme_uptime     uptime;
    struct pme_disk_io    disk_io;
    struct pme_filesystem filesystem;
    struct pme_netdev     netdev;
    struct pme_thermal    thermal;
    struct pme_gpu        gpu;
};

struct pme_provider {
    uint32_t size;
    uint32_t abi_version;
    const char *name;
    uint64_t capabilities;

    int (*init)(const struct pme_config *);

    int (*collect_all)(struct pme_snapshot *);

    void (*destroy)(void);
};

PME_EXPORT const struct pme_provider *pme_provider_get(void);

#ifdef __cplusplus
}
#endif

#endif
