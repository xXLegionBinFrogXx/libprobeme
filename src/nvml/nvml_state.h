#ifndef PROBEME_NVML_STATE_H
#define PROBEME_NVML_STATE_H

#include <nvml.h>

#include "probeme.h"

#define PME_NVML_MAX_DEVS 8

unsigned int pme_nvml_dev_count(void);
nvmlDevice_t pme_nvml_dev_handle(unsigned int index);

int pme_collect_gpu_section(struct pme_gpu *out);

#endif
