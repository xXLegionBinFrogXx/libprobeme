#ifndef PROBEME_COMMON_EXPORT_H
#define PROBEME_COMMON_EXPORT_H

/* PME_EXPORT marks the only symbols that leave a provider .so:
 * pme_abi_version() and pme_provider_get(). Everything else is hidden
 * via -fvisibility=hidden. */
#if defined(_WIN32)
#define PME_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define PME_EXPORT __attribute__((visibility("default")))
#else
#define PME_EXPORT
#endif

#endif /* PROBEME_COMMON_EXPORT_H */
