#ifndef PROBEME_LINUX_PROCFS_H
#define PROBEME_LINUX_PROCFS_H

#include <stddef.h>
#include <stdint.h>

#include "probeme.h"

/*
 * Pure /proc parsers: (const char *buf, size_t len, struct out *) -> int.
 * No I/O, no allocation, no stdio; never read past (buf + len).
 * Return PME_OK if at least one usable record was parsed, PME_EIO if none.
 * Malformed lines/fields are skipped, never read out of bounds.
 * Parsers never touch read_at_ns; the provider stamps it.
 */

/* /proc/stat: "cpu" aggregate goes to cpu[0], "cpuN" lines follow.
 * n == 1 + logical CPU count, capped at the array size. Fields missing on
 * older kernels parse as 0; guest/guest_nice are ignored. */
int pme_parse_proc_stat(const char *buf, size_t len, struct pme_cpu *out);

/* /proc/meminfo: only the seven keys of struct pme_memory are read,
 * kB converted to bytes. Absent keys (e.g. MemAvailable) stay 0. */
int pme_parse_meminfo(const char *buf, size_t len, struct pme_memory *out);

/* /proc/loadavg (first line): "<l1> <l5> <l15> <run>/<total> <lastpid>". */
int pme_parse_loadavg(const char *buf, size_t len, struct pme_loadavg *out);

/* /proc/uptime (first line): "<uptime> <idle>". Only uptime is taken. */
int pme_parse_uptime(const char *buf, size_t len, struct pme_uptime *out);

/* /proc/diskstats: 14/18/20-column kernels all share the first 10 fields
 * after the device name. All device lines are returned (including
 * partitions); the provider filters against /sys/block. */
int pme_parse_diskstats(const char *buf, size_t len, struct pme_disk_io *out);

/* /proc/net/dev: two header lines skipped, then "<name>: <16 counters>".
 * First 8 counters map to rx_*, counters 9..12 to tx_*. */
int pme_parse_net_dev(const char *buf, size_t len, struct pme_netdev *out);

/* /proc/self/mounts: "<device> <mountpoint> <fstype> <opts...>".
 * Sets PME_MOUNT_RO for the "ro" option and PME_MOUNT_SKIPPED for
 * remote/userspace filesystem types. Size fields are left untouched. */
int pme_parse_self_mounts(const char *buf, size_t len, struct pme_filesystem *out);

/* Shared token scanners (also used by thermal.c). Return 0 on success,
 * -1 if no number is present at *pp or the value overflows. */
int pme_parse_u64(const char **pp, const char *end, uint64_t *out);
int pme_parse_i64(const char **pp, const char *end, int64_t *out);

#endif /* PROBEME_LINUX_PROCFS_H */
