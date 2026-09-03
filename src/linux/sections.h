#ifndef PROBEME_LINUX_SECTIONS_H
#define PROBEME_LINUX_SECTIONS_H

#include <stdint.h>

#include "common/clock.h"
#include "probeme.h"

int pme_collect_cpu_section(int fd, struct pme_cpu *out);
int pme_collect_memory_section(int fd, struct pme_memory *out);
int pme_collect_loadavg_section(int fd, struct pme_loadavg *out);
int pme_collect_uptime_section(int fd, struct pme_uptime *out);
int pme_collect_disk_io_section(int fd, struct pme_disk_io *out);
int pme_collect_netdev_section(int fd, struct pme_netdev *out);

int pme_collect_filesystem_section(int fd, struct pme_filesystem *out,
                                   uint64_t cfg_flags);
int pme_collect_thermal_section(struct pme_thermal *out);

#endif
