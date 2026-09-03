/*
 * probeme - system information snapshot API for Linux
 *
 * The header is a sysinfo ABI: no Prometheus vocabulary, no dynamic
 * allocation, no threads. Consumers allocate one struct pme_snapshot
 * (~56 KB) and call collect_all(); the library fills raw kernel counters,
 * never rates or accumulated values.
 *
 * ABI rules:
 *   - Only pme_abi_version() and pme_provider_get() are exported by a
 *     provider shared object.
 *   - Structs are append-only across minor versions; `size` is the first
 *     member of versioned structs.
 *   - Fixed-width stdint.h types only; no bool, no enum fields.
 *   - Strings are fixed char[N], NUL-terminated, silently truncated.
 *   - uint64_t for nanosecond timestamps and flag bitmasks.
 */
#ifndef PROBEME_H
#define PROBEME_H

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

/* (major << 16) | minor */
PME_EXPORT uint32_t pme_abi_version(void);

/* ------------------------------------------------------------------ */
/* Capability / valid / truncated bits                                 */
/* ------------------------------------------------------------------ */

#define PME_CAP_CPU        (1u << 0)
#define PME_CAP_MEMORY     (1u << 1)
#define PME_CAP_LOADAVG    (1u << 2)
#define PME_CAP_UPTIME     (1u << 3)
#define PME_CAP_DISK_IO    (1u << 4)
#define PME_CAP_FILESYSTEM (1u << 5)
#define PME_CAP_NETDEV     (1u << 6)
#define PME_CAP_THERMAL    (1u << 7)
#define PME_CAP_GPU        (1u << 8)

/* ------------------------------------------------------------------ */
/* Return codes                                                        */
/* ------------------------------------------------------------------ */

#define PME_OK       0
#define PME_ENOTSUP -1
#define PME_EIO     -2
#define PME_EINVAL  -3
#define PME_ENOINIT -4

/* ------------------------------------------------------------------ */
/* Configuration                                                       */
/* ------------------------------------------------------------------ */

/* Library is unfiltered by default except: remote/userspace filesystem
 * statvfs() calls are skipped (entry is still reported, flagged with
 * PME_MOUNT_SKIPPED). Set PME_CFG_FS_INCLUDE_REMOTE to opt in. */
#define PME_CFG_FS_INCLUDE_REMOTE (1ull << 0)

struct pme_config {
    uint32_t size;      /* caller: sizeof(struct pme_config) */
    uint32_t reserved;
    uint64_t flags;     /* PME_CFG_* */
};

/* Mount entry flags */
#define PME_MOUNT_RO      (1u << 0)
#define PME_MOUNT_SKIPPED (1u << 1) /* remote/userspace FS not statvfs'd */

/* ------------------------------------------------------------------ */
/* Section structs                                                     */
/* ------------------------------------------------------------------ */

/* Raw per-CPU jiffies since boot, as printed by /proc/stat. The library
 * never computes deltas. */
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

/* cpu[0] is the kernel-wide aggregate ("cpu" line); cpu[1..n) are the
 * individual CPUs ("cpu0", "cpu1", ...) so n == 1 + logical CPU count. */
struct pme_cpu {
    uint32_t n;
    uint32_t pad;
    uint64_t read_at_ns;
    struct pme_cpu_core cpu[256];
};

/* Values in bytes, from /proc/meminfo (kB fields * 1024).
 * available is 0 on kernels without MemAvailable. */
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

/* 1/5/15-minute load as printed by /proc/loadavg (kernel fixed-point,
 * two decimals). running/total: runnable / total scheduling entities. */
struct pme_loadavg {
    uint64_t read_at_ns;
    double load1;
    double load5;
    double load15;
    uint32_t running;
    uint32_t total;
};

/* uptime_s from /proc/uptime (truncated to seconds);
 * boot_time_unix_s = current wall clock - uptime_s. */
struct pme_uptime {
    uint64_t read_at_ns;
    uint64_t uptime_s;
    uint64_t boot_time_unix_s;
};

/* Raw totals since boot from /proc/diskstats. Bytes derived from the
 * sector counts (512 bytes per sector). */
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

/* One entry per /proc/self/mounts line. Remote/userspace filesystems are
 * present but flagged PME_MOUNT_SKIPPED with size fields 0 unless
 * PME_CFG_FS_INCLUDE_REMOTE is set. Mountpoint paths keep the kernel's
 * escaping (e.g. \040 for space). */
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

/* Raw totals since boot from /proc/net/dev. */
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

/* Temperature in milli-degrees Celsius from /sys/class/thermal.
 * `type` is the zone type string (e.g. "cpu_thermal", "acpitz"). */
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

/* Instantaneous GPU state. No VRAM series (unified-memory architectures
 * report meaningless numbers). power_mw is milliwatts. pstate is the
 * numeric performance state (0 = P0, highest). */
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

/* ------------------------------------------------------------------ */
/* Snapshot                                                            */
/* ------------------------------------------------------------------ */

/* Caller allocates once (static or heap, ~56 KB), zeroes it, sets size =
 * sizeof(struct pme_snapshot), passes to collect_all(). Before each refresh
 * cycle the caller resets valid/truncated to 0. The library fills only
 * sections listed in provider capabilities; on success it sets those
 * sections' bits in `valid` and never writes other sections, so two
 * providers (e.g. linux + nvml) can fill one snapshot in any order.
 *
 * `truncated` mirrors `valid`: a bit is set when the corresponding array
 * cap was hit (then n == cap). */
struct pme_snapshot {
    uint32_t size;        /* caller: sizeof(struct pme_snapshot) */
    uint32_t abi_version; /* provider stamps on collect */
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

/* ------------------------------------------------------------------ */
/* Provider                                                            */
/* ------------------------------------------------------------------ */

struct pme_provider {
    uint32_t size;
    uint32_t abi_version;
    const char *name;
    uint64_t capabilities; /* PME_CAP_* bitmask */

    /* init: check config, open resources. Returns PME_OK, PME_EINVAL
     * (size < sizeof or reserved != 0), or PME_ENOTSUP (backend not
     * usable on this machine). Must be called before collect_all. */
    int (*init)(const struct pme_config *);

    /* collect_all: fills only capability sections, stamps abi_version,
     * assembles `valid`/`truncated`. Returns PME_OK if at least one
     * section succeeded, PME_EIO if none did, PME_EINVAL on bad size,
     * PME_ENOINIT if init was not called. Per-section failure leaves the
     * section untouched and its `valid` bit clear. Not reentrant; the
     * caller serializes. */
    int (*collect_all)(struct pme_snapshot *);

    void (*destroy)(void);
};

/* The only exported symbols of a provider .so are pme_abi_version() and
 * this function. */
PME_EXPORT const struct pme_provider *pme_provider_get(void);

#ifdef __cplusplus
}
#endif

#endif /* PROBEME_H */
