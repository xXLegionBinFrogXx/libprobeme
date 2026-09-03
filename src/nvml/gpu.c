#include "nvml_state.h"

#include <string.h>

#include "common/clock.h"

static void copy_str(char *dst, size_t cap, const char *src)
{
    size_t n = strlen(src);

    if (n >= cap) {
        n = cap - 1u;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

int pme_collect_gpu_section(struct pme_gpu *out)
{
    unsigned int count = pme_nvml_dev_count();
    unsigned int i;
    uint32_t n = 0;

    for (i = 0u; i < count && n < (uint32_t)PME_NVML_MAX_DEVS; i++) {
        nvmlDevice_t dev = pme_nvml_dev_handle(i);
        struct pme_gpu_dev *g = &out->gpus[n];
        char buf[NVML_DEVICE_UUID_V2_BUFFER_SIZE];
        unsigned int v;
        nvmlUtilization_t util;
        nvmlPstates_t ps;

        memset(g, 0, sizeof(*g));

        if (nvmlDeviceGetUUID(dev, buf, (unsigned int)sizeof(buf)) ==
            NVML_SUCCESS) {
            copy_str(g->uuid, sizeof(g->uuid), buf);
        }
        if (nvmlDeviceGetName(dev, buf, (unsigned int)sizeof(buf)) ==
            NVML_SUCCESS) {
            copy_str(g->name, sizeof(g->name), buf);
        }
        if (nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, &v) ==
            NVML_SUCCESS) {
            g->temp_c = (uint32_t)v;
        }
        if (nvmlDeviceGetPowerUsage(dev, &v) == NVML_SUCCESS) {
            g->power_mw = (uint32_t)v;
        }
        if (nvmlDeviceGetClockInfo(dev, NVML_CLOCK_SM, &v) == NVML_SUCCESS) {
            g->sm_clock_mhz = (uint32_t)v;
        }
        if (nvmlDeviceGetUtilizationRates(dev, &util) == NVML_SUCCESS) {
            g->util_pct = (uint32_t)util.gpu;
        }
        if (nvmlDeviceGetPerformanceState(dev, &ps) == NVML_SUCCESS) {
            g->pstate = (uint32_t)ps;
        }
        n++;
    }

    out->n = n;
    out->read_at_ns = pme_now_ns();
    return PME_OK;
}
