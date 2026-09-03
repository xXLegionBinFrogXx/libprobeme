
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* monitor [interval_s] [provider.so ...] - refresh system stats every
 * interval (default 5). Rates are computed here from the raw counters. */

#include "probeme.h"

typedef const struct pme_provider *(*get_fn)(void);

#define FRAME_DT(sn) ((double)((sn)->cpu.read_at_ns) / 1e9)

static int load_provider(const char *path, struct pme_snapshot *snap,
                         const struct pme_config *cfg)
{
    void *so = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    get_fn get;
    const struct pme_provider *p;

    if (so == NULL) {
        fprintf(stderr, "skip %s: %s\n", path, dlerror());
        return -1;
    }
    get = (get_fn)dlsym(so, "pme_provider_get");
    p = (get != NULL) ? get() : NULL;
    if (p == NULL || p->init(cfg) != PME_OK) {
        fprintf(stderr, "skip %s: not usable\n", path);
        dlclose(so);
        return -1;
    }
    if (p->collect_all(snap) != PME_OK) {
        p->destroy();
        dlclose(so);
        return -1;
    }
    p->destroy();
    dlclose(so);
    return 0;
}

static void human(char *out, size_t cap, double bytes)
{
    const char *u[] = { "B", "K", "M", "G", "T" };
    int i = 0;

    while (bytes >= 1024.0 && i < 4) {
        bytes /= 1024.0;
        i++;
    }
    snprintf(out, cap, "%.1f%s", bytes, u[i]);
}

static double clamp_pct(double v)
{
    if (v < 0.0) {
        return 0.0;
    }
    return (v > 100.0) ? 100.0 : v;
}

static void frame(const struct pme_snapshot *now, const struct pme_snapshot *prev,
                  int have_prev)
{
    char tbuf[32];
    char b1[32];
    char b2[32];
    time_t wall = time(NULL);
    double dt = have_prev ? (FRAME_DT(now) - FRAME_DT(prev)) : 0.0;

    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&wall));
    printf("probeme @ %s\n", tbuf);

    if (now->valid & PME_CAP_CPU) {
        double pct = 0.0;
        if (have_prev && dt > 0.0) {
            const struct pme_cpu_core *a = &prev->cpu.cpu[0];
            const struct pme_cpu_core *b = &now->cpu.cpu[0];
            double total = (double)(b->user - a->user) +
                           (double)(b->nice - a->nice) +
                           (double)(b->system - a->system) +
                           (double)(b->idle - a->idle) +
                           (double)(b->iowait - a->iowait) +
                           (double)(b->irq - a->irq) +
                           (double)(b->softirq - a->softirq) +
                           (double)(b->steal - a->steal);
            double idle = (double)(b->idle - a->idle) +
                          (double)(b->iowait - a->iowait);
            pct = clamp_pct(100.0 * (total - idle) / total);
        }
        if (now->valid & PME_CAP_LOADAVG) {
            printf("cpu  %4.1f%% (%u cpus)  load %.2f %.2f %.2f (%u/%u)\n", pct,
                   (now->cpu.n > 0u) ? now->cpu.n - 1u : 0u, now->loadavg.load1,
                   now->loadavg.load5, now->loadavg.load15,
                   now->loadavg.running, now->loadavg.total);
        } else {
            printf("cpu  %4.1f%% (%u cpus)\n", pct,
                   (now->cpu.n > 0u) ? now->cpu.n - 1u : 0u);
        }
    }

    if (now->valid & PME_CAP_MEMORY) {
        human(b1, sizeof(b1), (double)now->memory.total);
        human(b2, sizeof(b2), (double)now->memory.available);
        printf("mem  %s total, %s avail", b1, b2);
        if (now->memory.swap_total > 0u) {
            human(b1, sizeof(b1),
                  (double)(now->memory.swap_total - now->memory.swap_free));
            human(b2, sizeof(b2), (double)now->memory.swap_total);
            printf(", swap %s/%s", b1, b2);
        }
        printf("\n");
    }

    if (now->valid & PME_CAP_UPTIME) {
        printf("up   %llu s (boot %llu unix)\n",
               (unsigned long long)now->uptime.uptime_s,
               (unsigned long long)now->uptime.boot_time_unix_s);
    }

    if ((now->valid & PME_CAP_DISK_IO) && have_prev && dt > 0.0) {
        uint32_t i;
        printf("disk");
        for (i = 0; i < now->disk_io.n; i++) {
            const struct pme_disk *d = &now->disk_io.disks[i];
            const struct pme_disk *pd = NULL;
            uint32_t j;
            double rb;
            double wb;

            for (j = 0; j < prev->disk_io.n; j++) {
                if (strcmp(prev->disk_io.disks[j].name, d->name) == 0) {
                    pd = &prev->disk_io.disks[j];
                    break;
                }
            }
            if (pd == NULL) {
                continue;
            }
            rb = (double)(d->read_bytes - pd->read_bytes) / dt;
            wb = (double)(d->write_bytes - pd->write_bytes) / dt;
            if (rb + wb < 1024.0) {
                continue;
            }
            human(b1, sizeof(b1), rb);
            human(b2, sizeof(b2), wb);
            printf("  %s r %s/s w %s/s", d->name, b1, b2);
        }
        printf("\n");
    }

    if ((now->valid & PME_CAP_NETDEV) && have_prev && dt > 0.0) {
        uint32_t i;
        printf("net ");
        for (i = 0; i < now->netdev.n; i++) {
            const struct pme_iface *f = &now->netdev.ifaces[i];
            const struct pme_iface *pf = NULL;
            uint32_t j;
            double rx;
            double tx;

            for (j = 0; j < prev->netdev.n; j++) {
                if (strcmp(prev->netdev.ifaces[j].name, f->name) == 0) {
                    pf = &prev->netdev.ifaces[j];
                    break;
                }
            }
            if (pf == NULL) {
                continue;
            }
            rx = (double)(f->rx_bytes - pf->rx_bytes) / dt;
            tx = (double)(f->tx_bytes - pf->tx_bytes) / dt;
            if (rx + tx < 1024.0) {
                continue;
            }
            human(b1, sizeof(b1), rx);
            human(b2, sizeof(b2), tx);
            printf("  %s rx %s/s tx %s/s", f->name, b1, b2);
        }
        printf("\n");
    }

    if (now->valid & PME_CAP_THERMAL) {
        uint32_t i;
        printf("temp");
        for (i = 0; i < now->thermal.n; i++) {
            printf("  %s %.1fC", now->thermal.zones[i].type,
                   (double)now->thermal.zones[i].temp_mc / 1000.0);
        }
        printf("\n");
    }

    if (now->valid & PME_CAP_GPU) {
        uint32_t i;
        for (i = 0; i < now->gpu.n; i++) {
            printf("gpu  %s %uC %u mW %u MHz %u%% P%u\n",
                   now->gpu.gpus[i].name, now->gpu.gpus[i].temp_c,
                   now->gpu.gpus[i].power_mw, now->gpu.gpus[i].sm_clock_mhz,
                   now->gpu.gpus[i].util_pct, now->gpu.gpus[i].pstate);
        }
    }
    fflush(stdout);
}

int main(int argc, char **argv)
{
    static struct pme_snapshot now;
    static struct pme_snapshot prev;
    struct pme_config cfg;
    int interval = 5;
    int have_prev = 0;
    int argi = 1;
    const char *providers[8];
    int nproviders = 0;

    memset(&cfg, 0, sizeof(cfg));
    cfg.size = (uint32_t)sizeof(cfg);

    if (argi < argc && *argv[argi] >= '0' && *argv[argi] <= '9') {
        interval = atoi(argv[argi++]);
        if (interval < 1) {
            interval = 1;
        }
    }
    for (; argi < argc && nproviders < 8; argi++) {
        providers[nproviders++] = argv[argi];
    }
    if (nproviders == 0) {
        providers[nproviders++] = "libprobeme_linux.so.1";
        providers[nproviders++] = "libprobeme_nvml.so.1";
    }

    memset(&now, 0, sizeof(now));
    now.size = (uint32_t)sizeof(now);

    fputs("\033[2J", stdout);
    for (;;) {
        int i;

        memcpy(&prev, &now, sizeof(now));
        now.valid = 0u;
        now.truncated = 0u;
        for (i = 0; i < nproviders; i++) {
            load_provider(providers[i], &now, &cfg);
        }
        if (now.valid == 0u) {
            fprintf(stderr, "no data from any provider\n");
            return 1;
        }
        fputs("\033[H", stdout);
        frame(&now, &prev, have_prev);
        have_prev = 1;
        sleep((unsigned int)interval);
    }
}
