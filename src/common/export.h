#ifndef PROBEME_COMMON_EXPORT_H
#define PROBEME_COMMON_EXPORT_H

#if defined(_WIN32)
#define PME_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define PME_EXPORT __attribute__((visibility("default")))
#else
#define PME_EXPORT
#endif

#endif
