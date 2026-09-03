/* live_linux - load the built linux provider and assert sane ranges.
 * Skips (exit 77) when the .so cannot be loaded. Run on real Linux hosts. */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "probeme.h"

typedef const struct pme_provider *(*get_fn)(void);

#define CHECK(cond, ...)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s -- ", __FILE__, __LINE__, #cond);\
            fprintf(stderr, __VA_ARGS__);                                    \
            fputc('\n', stderr);                                             \
            exit(1);                                                         \
        }                                                                    \
    } while (0)

int main(void)
{
    static struct pme_snapshot snap;
    static char path[512];
    void *so;
    get_fn get;
    const struct pme_provider *p;
    struct pme_config cfg;
    int rc;

    if (snprintf(path, sizeof(path), "%s/libprobeme_linux.so.1",
                 PME_BUILD_LIB_DIR) >= (int)sizeof(path)) {
        return 77;
    }
    so = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (so == NULL) {
        printf("skip: %s\n", dlerror());
        return 77;
    }
    get = (get_fn)dlsym(so, "pme_provider_get");
    CHECK(get != NULL, "pme_provider_get missing");
    p = get();
    CHECK(p != NULL && p->abi_version >> 16 == PME_ABI_MAJOR, "abi major");

    memset(&cfg, 0, sizeof(cfg));
    cfg.size = (uint32_t)sizeof(cfg);
    CHECK(p->init(&cfg) == PME_OK, "init");

    memset(&snap, 0, sizeof(snap));
    snap.size = (uint32_t)sizeof(snap);
    rc = p->collect_all(&snap);
    CHECK(rc == PME_OK, "collect_all rc=%d", rc);
    CHECK(snap.abi_version >> 16 == PME_ABI_MAJOR, "stamped abi");
    CHECK((snap.valid & PME_CAP_CPU) != 0u, "cpu valid=0x%llx",
          (unsigned long long)snap.valid);
    CHECK((snap.valid & PME_CAP_MEMORY) != 0u, "memory");
    CHECK((snap.valid & PME_CAP_LOADAVG) != 0u, "loadavg");
    CHECK((snap.valid & PME_CAP_UPTIME) != 0u, "uptime");
    CHECK((snap.valid & PME_CAP_DISK_IO) != 0u, "disk_io");
    CHECK((snap.valid & PME_CAP_FILESYSTEM) != 0u, "filesystem");
    CHECK((snap.valid & PME_CAP_NETDEV) != 0u, "netdev");

    CHECK(snap.cpu.n >= 2u && snap.cpu.n <= 256u, "cpu n=%u", snap.cpu.n);
    CHECK(snap.memory.total > 0u, "mem total");
    CHECK(snap.memory.free <= snap.memory.total, "mem free<=total");
    CHECK(snap.loadavg.total >= snap.loadavg.running, "loadavg run/tot");
    CHECK(snap.uptime.uptime_s > 0u, "uptime");
    CHECK(snap.uptime.boot_time_unix_s > 0u, "boot time");
    CHECK(snap.disk_io.n >= 1u && snap.disk_io.n <= 64u, "disk n=%u",
          snap.disk_io.n);
    CHECK(snap.filesystem.n >= 1u && snap.filesystem.n <= 64u, "mounts n=%u",
          snap.filesystem.n);
    CHECK(snap.netdev.n >= 1u && snap.netdev.n <= 64u, "netdev n=%u",
          snap.netdev.n);
    CHECK((snap.valid & PME_CAP_THERMAL) == 0u || snap.thermal.n <= 32u,
          "thermal n");
    CHECK(snap.gpu.n == 0u, "linux provider must not touch gpu");

    p->destroy();
    dlclose(so);
    printf("ok live_linux (valid=0x%llx)\n", (unsigned long long)snap.valid);
    return 0;
}
