
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* dump [provider.so ...] - load probeme providers, fill one snapshot,
 * print it. Default: libprobeme_linux.so.1 libprobeme_nvml.so.1 */

#include "probeme.h"

typedef uint32_t (*abi_fn)(void);
typedef const struct pme_provider *(*get_fn)(void);

static void dump_section(uint64_t valid, uint64_t cap, const char *name)
{
    printf("%s%s%s ", (valid & cap) ? "" : "[", name, (valid & cap) ? "" : "]");
}

int main(int argc, char **argv)
{
    static struct pme_snapshot snap;
    struct pme_config cfg;
    const char *providers[8];
    int nproviders = 0;
    int i;

    if (argc > 1) {
        for (i = 1; i < argc && nproviders < 8; i++) {
            providers[nproviders++] = argv[i];
        }
    } else {
        providers[nproviders++] = "libprobeme_linux.so.1";
        providers[nproviders++] = "libprobeme_nvml.so.1";
    }

    memset(&snap, 0, sizeof(snap));
    snap.size = (uint32_t)sizeof(snap);
    memset(&cfg, 0, sizeof(cfg));
    cfg.size = (uint32_t)sizeof(cfg);

    for (i = 0; i < nproviders; i++) {
        void *so;
        abi_fn abi;
        get_fn get;
        const struct pme_provider *p;
        int rc;

        so = dlopen(providers[i], RTLD_NOW | RTLD_LOCAL);
        if (so == NULL) {
            fprintf(stderr, "dlopen(%s): %s\n", providers[i], dlerror());
            continue;
        }
        abi = (abi_fn)dlsym(so, "pme_abi_version");
        get = (get_fn)dlsym(so, "pme_provider_get");
        if (abi == NULL || get == NULL) {
            fprintf(stderr, "%s: missing exported symbols\n", providers[i]);
            dlclose(so);
            continue;
        }
        p = get();
        if (p == NULL || p->abi_version >> 16 != abi() >> 16) {
            fprintf(stderr, "%s: inconsistent provider\n", providers[i]);
            dlclose(so);
            continue;
        }

        printf("provider '%s' abi %u.%u caps=0x%llx\n", p->name,
               p->abi_version >> 16, p->abi_version & 0xffffu,
               (unsigned long long)p->capabilities);

        rc = p->init(&cfg);
        if (rc != PME_OK) {
            fprintf(stderr, "%s init: %d\n", p->name, rc);
            dlclose(so);
            continue;
        }
        rc = p->collect_all(&snap);
        p->destroy();
        dlclose(so);
        if (rc != PME_OK) {
            fprintf(stderr, "%s collect_all: %d\n", p->name, rc);
        }
    }

    if (snap.valid == 0) {
        fprintf(stderr, "no section collected\n");
        return 1;
    }

    printf("valid=0x%llx truncated=0x%llx\n",
           (unsigned long long)snap.valid, (unsigned long long)snap.truncated);
    printf("sections:");
    dump_section(snap.valid, PME_CAP_CPU, "cpu");
    dump_section(snap.valid, PME_CAP_MEMORY, "memory");
    dump_section(snap.valid, PME_CAP_LOADAVG, "loadavg");
    dump_section(snap.valid, PME_CAP_UPTIME, "uptime");
    dump_section(snap.valid, PME_CAP_DISK_IO, "disk_io");
    dump_section(snap.valid, PME_CAP_FILESYSTEM, "filesystem");
    dump_section(snap.valid, PME_CAP_NETDEV, "netdev");
    dump_section(snap.valid, PME_CAP_THERMAL, "thermal");
    dump_section(snap.valid, PME_CAP_GPU, "gpu");
    printf("\n");

    if (snap.valid & PME_CAP_CPU) {
        printf("cpu: n=%u\n", snap.cpu.n);
    }
    if (snap.valid & PME_CAP_MEMORY) {
        printf("memory: total=%llu free=%llu avail=%llu swap=%llu\n",
               (unsigned long long)snap.memory.total,
               (unsigned long long)snap.memory.free,
               (unsigned long long)snap.memory.available,
               (unsigned long long)snap.memory.swap_total);
    }
    if (snap.valid & PME_CAP_LOADAVG) {
        printf("loadavg: %.2f %.2f %.2f (%u/%u)\n", snap.loadavg.load1,
               snap.loadavg.load5, snap.loadavg.load15, snap.loadavg.running,
               snap.loadavg.total);
    }
    if (snap.valid & PME_CAP_UPTIME) {
        printf("uptime: %llus boot=%llu\n",
               (unsigned long long)snap.uptime.uptime_s,
               (unsigned long long)snap.uptime.boot_time_unix_s);
    }
    if (snap.valid & PME_CAP_DISK_IO) {
        uint32_t j;
        printf("disk_io: n=%u\n", snap.disk_io.n);
        for (j = 0; j < snap.disk_io.n; j++) {
            printf("  %s r=%llu (%llub) w=%llu (%llub)\n",
                   snap.disk_io.disks[j].name,
                   (unsigned long long)snap.disk_io.disks[j].reads,
                   (unsigned long long)snap.disk_io.disks[j].read_bytes,
                   (unsigned long long)snap.disk_io.disks[j].writes,
                   (unsigned long long)snap.disk_io.disks[j].write_bytes);
        }
    }
    if (snap.valid & PME_CAP_FILESYSTEM) {
        uint32_t j;
        printf("filesystem: n=%u\n", snap.filesystem.n);
        for (j = 0; j < snap.filesystem.n; j++) {
            struct pme_mount *m = &snap.filesystem.mounts[j];
            if (m->flags & PME_MOUNT_SKIPPED) {
                printf("  %-10s %-24s %-8s skipped%s\n", m->device,
                       m->mountpoint, m->fstype,
                       (m->flags & PME_MOUNT_RO) ? " ro" : "");
            } else {
                printf("  %-10s %-24s %-8s %llu/%llu free%s\n", m->device,
                       m->mountpoint, m->fstype,
                       (unsigned long long)m->free_bytes,
                       (unsigned long long)m->size_bytes,
                       (m->flags & PME_MOUNT_RO) ? " ro" : "");
            }
        }
    }
    if (snap.valid & PME_CAP_NETDEV) {
        uint32_t j;
        printf("netdev: n=%u\n", snap.netdev.n);
        for (j = 0; j < snap.netdev.n; j++) {
            printf("  %-12s rx=%llu tx=%llu\n", snap.netdev.ifaces[j].name,
                   (unsigned long long)snap.netdev.ifaces[j].rx_bytes,
                   (unsigned long long)snap.netdev.ifaces[j].tx_bytes);
        }
    }
    if (snap.valid & PME_CAP_THERMAL) {
        uint32_t j;
        printf("thermal: n=%u\n", snap.thermal.n);
        for (j = 0; j < snap.thermal.n; j++) {
            printf("  %-24s %lld mC\n", snap.thermal.zones[j].type,
                   (long long)snap.thermal.zones[j].temp_mc);
        }
    }
    if (snap.valid & PME_CAP_GPU) {
        uint32_t j;
        printf("gpu: n=%u\n", snap.gpu.n);
        for (j = 0; j < snap.gpu.n; j++) {
            printf("  %s %s temp=%uc power=%llumw sm=%llumhz util=%u%% p%u\n",
                   snap.gpu.gpus[j].uuid, snap.gpu.gpus[j].name,
                   snap.gpu.gpus[j].temp_c,
                   (unsigned long long)snap.gpu.gpus[j].power_mw,
                   (unsigned long long)snap.gpu.gpus[j].sm_clock_mhz,
                   snap.gpu.gpus[j].util_pct, snap.gpu.gpus[j].pstate);
        }
    }
    return 0;
}
