
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* live_nvml: GPU range asserts; exit 77 (ctest skip) on non-NVIDIA hosts. */

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
    uint32_t i;

    if (snprintf(path, sizeof(path), "%s/libprobeme_nvml.so.1",
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
    rc = p->init(&cfg);
    if (rc != PME_OK) {
        printf("skip: init rc=%d (no NVIDIA driver?)\n", rc);
        p->destroy();
        dlclose(so);
        return 77;
    }

    memset(&snap, 0, sizeof(snap));
    snap.size = (uint32_t)sizeof(snap);
    rc = p->collect_all(&snap);
    p->destroy();
    dlclose(so);
    CHECK(rc == PME_OK, "collect_all rc=%d", rc);
    CHECK((snap.valid & PME_CAP_GPU) != 0u, "gpu valid=0x%llx",
          (unsigned long long)snap.valid);
    CHECK(snap.gpu.n >= 1u && snap.gpu.n <= 8u, "gpu n=%u", snap.gpu.n);

    for (i = 0; i < snap.gpu.n; i++) {
        const struct pme_gpu_dev *g = &snap.gpu.gpus[i];
        CHECK(g->uuid[0] != '\0', "uuid[%u] empty", i);
        CHECK(g->name[0] != '\0', "name[%u] empty", i);
        CHECK(g->temp_c <= 120u, "temp[%u]=%u", i, g->temp_c);
        CHECK(g->util_pct <= 100u, "util[%u]=%u", i, g->util_pct);
        CHECK(g->power_mw > 0u, "power[%u]=%u", i, g->power_mw);
    }

    printf("ok live_nvml (n=%u: %s %u mC %u mW)\n", snap.gpu.n,
           snap.gpu.gpus[0].name, snap.gpu.gpus[0].temp_c,
           snap.gpu.gpus[0].power_mw);
    return 0;
}
