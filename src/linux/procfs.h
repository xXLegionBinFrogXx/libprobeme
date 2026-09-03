#ifndef PROBEME_LINUX_PROCFS_H
#define PROBEME_LINUX_PROCFS_H

/* Pure /proc parsers: malformed lines are skipped, input is never read
 * past (buf + len), read_at_ns is left to the caller. Semantics per
 * section are in docs/ABI.md. */

#include <stddef.h>
#include <stdint.h>

#include "probeme.h"

int pme_parse_proc_stat(const char *buf, size_t len, struct pme_cpu *out);

int pme_parse_meminfo(const char *buf, size_t len, struct pme_memory *out);

int pme_parse_loadavg(const char *buf, size_t len, struct pme_loadavg *out);

int pme_parse_uptime(const char *buf, size_t len, struct pme_uptime *out);

int pme_parse_diskstats(const char *buf, size_t len, struct pme_disk_io *out);

int pme_parse_net_dev(const char *buf, size_t len, struct pme_netdev *out);

int pme_parse_self_mounts(const char *buf, size_t len, struct pme_filesystem *out);

int pme_parse_u64(const char **pp, const char *end, uint64_t *out);
int pme_parse_i64(const char **pp, const char *end, int64_t *out);

#endif
