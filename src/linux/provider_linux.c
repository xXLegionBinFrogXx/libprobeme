#include <fcntl.h>
#include <string.h>
#include <unistd.h>

/* Proc files are opened once in init() and re-read per collect_all(). */

#include "common/clock.h"
#include "common/provider_impl.h"
#include "sections.h"

enum {
    FD_STAT,
    FD_MEMINFO,
    FD_LOADAVG,
    FD_UPTIME,
    FD_DISKSTATS,
    FD_NETDEV,
    FD_MOUNTS,
    FD_COUNT
};

static const char *const g_paths[FD_COUNT] = {
    "/proc/stat",
    "/proc/meminfo",
    "/proc/loadavg",
    "/proc/uptime",
    "/proc/diskstats",
    "/proc/net/dev",
    "/proc/self/mounts",
};

static struct {
    int fd[FD_COUNT];
    uint64_t flags;
    int inited;
} g = { { -1, -1, -1, -1, -1, -1, -1 }, 0u, 0 };

static void linux_destroy(void);

static int linux_init(const struct pme_config *cfg)
{
    int i;

    if (cfg == NULL || cfg->size < (uint32_t)sizeof(struct pme_config) ||
        cfg->reserved != 0u) {
        return PME_EINVAL;
    }
    if (g.inited) {
        linux_destroy();
    }
    g.flags = cfg->flags;
    for (i = 0; i < FD_COUNT; i++) {
        g.fd[i] = open(g_paths[i], O_RDONLY);
    }
    g.inited = 1;
    return PME_OK;
}

static void linux_destroy(void)
{
    int i;

    for (i = 0; i < FD_COUNT; i++) {
        if (g.fd[i] >= 0) {
            close(g.fd[i]);
        }
        g.fd[i] = -1;
    }
    g.inited = 0;
}

static int linux_collect_all(struct pme_snapshot *sn){
    int ok = 0;

    if (pme_snapshot_prepare(sn) != PME_OK) {
        return PME_EINVAL;
    }
    if (!g.inited) {
        return PME_ENOINIT;
    }

    memset(&sn->cpu, 0, sizeof(sn->cpu));
    if (pme_collect_cpu_section(g.fd[FD_STAT], &sn->cpu) == PME_OK) {
        sn->valid |= PME_CAP_CPU;
        if (sn->cpu.n == 256u) {
            sn->truncated |= PME_CAP_CPU;
        }
        ok++;
    }

    memset(&sn->memory, 0, sizeof(sn->memory));
    if (pme_collect_memory_section(g.fd[FD_MEMINFO], &sn->memory) == PME_OK) {
        sn->valid |= PME_CAP_MEMORY;
        ok++;
    }

    memset(&sn->loadavg, 0, sizeof(sn->loadavg));
    if (pme_collect_loadavg_section(g.fd[FD_LOADAVG], &sn->loadavg) == PME_OK) {
        sn->valid |= PME_CAP_LOADAVG;
        ok++;
    }

    memset(&sn->uptime, 0, sizeof(sn->uptime));
    if (pme_collect_uptime_section(g.fd[FD_UPTIME], &sn->uptime) == PME_OK) {
        sn->valid |= PME_CAP_UPTIME;
        ok++;
    }

    memset(&sn->disk_io, 0, sizeof(sn->disk_io));
    if (pme_collect_disk_io_section(g.fd[FD_DISKSTATS], &sn->disk_io) == PME_OK) {
        sn->valid |= PME_CAP_DISK_IO;
        if (sn->disk_io.n == 64u) {
            sn->truncated |= PME_CAP_DISK_IO;
        }
        ok++;
    }

    memset(&sn->filesystem, 0, sizeof(sn->filesystem));
    if (pme_collect_filesystem_section(g.fd[FD_MOUNTS], &sn->filesystem,
                                       g.flags) == PME_OK) {
        sn->valid |= PME_CAP_FILESYSTEM;
        if (sn->filesystem.n == 64u) {
            sn->truncated |= PME_CAP_FILESYSTEM;
        }
        ok++;
    }

    memset(&sn->netdev, 0, sizeof(sn->netdev));
    if (pme_collect_netdev_section(g.fd[FD_NETDEV], &sn->netdev) == PME_OK) {
        sn->valid |= PME_CAP_NETDEV;
        if (sn->netdev.n == 64u) {
            sn->truncated |= PME_CAP_NETDEV;
        }
        ok++;
    }

    memset(&sn->thermal, 0, sizeof(sn->thermal));
    if (pme_collect_thermal_section(&sn->thermal) == PME_OK) {
        sn->valid |= PME_CAP_THERMAL;
        if (sn->thermal.n == 32u) {
            sn->truncated |= PME_CAP_THERMAL;
        }
        ok++;
    }

    return (ok > 0) ? PME_OK : PME_EIO;
}

const struct pme_provider pme_provider_impl = {
    sizeof(struct pme_provider),
    ((uint32_t)PME_ABI_MAJOR << 16) | (uint32_t)PME_ABI_MINOR,
    "linux",
    (uint64_t)PME_CAP_CPU | (uint64_t)PME_CAP_MEMORY |
        (uint64_t)PME_CAP_LOADAVG | (uint64_t)PME_CAP_UPTIME |
        (uint64_t)PME_CAP_DISK_IO | (uint64_t)PME_CAP_FILESYSTEM |
        (uint64_t)PME_CAP_NETDEV | (uint64_t)PME_CAP_THERMAL,
    linux_init,
    linux_collect_all,
    linux_destroy,
};
