#ifndef PROBEME_COMMON_PROVIDER_IMPL_H
#define PROBEME_COMMON_PROVIDER_IMPL_H

#include "probeme.h"

/* Each provider translation unit defines exactly this one object;
 * common/provider.c implements the two exported ABI symbols on top of it. */
extern const struct pme_provider pme_provider_impl;

/* Validate the caller's snapshot, stamp abi_version, clear valid/truncated.
 * Returns PME_OK or PME_EINVAL. */
int pme_snapshot_prepare(struct pme_snapshot *sn);

#endif /* PROBEME_COMMON_PROVIDER_IMPL_H */
