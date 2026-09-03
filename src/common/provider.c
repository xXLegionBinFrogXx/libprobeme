#include "provider_impl.h"

#include "export.h"

#include <stddef.h>

PME_EXPORT uint32_t pme_abi_version(void)
{
    return ((uint32_t)PME_ABI_MAJOR << 16) | (uint32_t)PME_ABI_MINOR;
}

PME_EXPORT const struct pme_provider *pme_provider_get(void)
{
    return &pme_provider_impl;
}

int pme_snapshot_prepare(struct pme_snapshot *sn)
{
    if (sn == NULL || sn->size < (uint32_t)sizeof(struct pme_snapshot)) {
        return PME_EINVAL;
    }

    /* valid/truncated belong to the caller; providers only OR bits in. */
    sn->abi_version = pme_abi_version();
    return PME_OK;
}
