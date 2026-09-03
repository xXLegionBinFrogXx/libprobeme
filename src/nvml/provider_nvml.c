#include <string.h>

/* PME_ENOTSUP here means "no usable NVIDIA driver"; without VRAM series
 * because unified-memory parts report meaningless framebuffer numbers. */

#include "nvml_state.h"

#include "common/clock.h"
#include "common/provider_impl.h"

static struct {
    int inited;
    unsigned int count;
    nvmlDevice_t handles[PME_NVML_MAX_DEVS];
} g;

unsigned int pme_nvml_dev_count(void)
{
    return g.count;
}

nvmlDevice_t pme_nvml_dev_handle(unsigned int index)
{
    return g.handles[index];
}

static void nvml_backend_destroy(void);

static int nvml_backend_init(const struct pme_config *cfg)
{
    unsigned int i;
    unsigned int n = 0;

    if (cfg == NULL || cfg->size < (uint32_t)sizeof(struct pme_config) ||
        cfg->reserved != 0u) {
        return PME_EINVAL;
    }
    if (g.inited) {
        nvml_backend_destroy();
    }
    if (nvmlInit_v2() != NVML_SUCCESS) {
        return PME_ENOTSUP;
    }
    if (nvmlDeviceGetCount_v2(&n) != NVML_SUCCESS) {
        nvmlShutdown();
        return PME_ENOTSUP;
    }
    g.count = 0u;
    for (i = 0u; i < n && g.count < (unsigned int)PME_NVML_MAX_DEVS; i++) {
        nvmlDevice_t h;
        if (nvmlDeviceGetHandleByIndex_v2(i, &h) == NVML_SUCCESS) {
            g.handles[g.count++] = h;
        }
    }
    g.inited = 1;
    return PME_OK;
}

static void nvml_backend_destroy(void)
{
    if (g.inited) {
        nvmlShutdown();
    }
    g.inited = 0;
    g.count = 0u;
}

static int nvml_backend_collect_all(struct pme_snapshot *sn)
{
    int rc;

    if (pme_snapshot_prepare(sn) != PME_OK) {
        return PME_EINVAL;
    }
    if (!g.inited) {
        return PME_ENOINIT;
    }

    memset(&sn->gpu, 0, sizeof(sn->gpu));
    rc = pme_collect_gpu_section(&sn->gpu);
    if (rc != PME_OK) {
        return rc;
    }
    sn->valid |= PME_CAP_GPU;
    if (sn->gpu.n == (uint32_t)PME_NVML_MAX_DEVS) {
        sn->truncated |= PME_CAP_GPU;
    }
    return PME_OK;
}

const struct pme_provider pme_provider_impl = {
    sizeof(struct pme_provider),
    ((uint32_t)PME_ABI_MAJOR << 16) | (uint32_t)PME_ABI_MINOR,
    "nvml",
    (uint64_t)PME_CAP_GPU,
    nvml_backend_init,
    nvml_backend_collect_all,
    nvml_backend_destroy,
};
