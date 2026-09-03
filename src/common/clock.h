#ifndef PROBEME_COMMON_CLOCK_H
#define PROBEME_COMMON_CLOCK_H

#include <stdint.h>

/* Monotonic nanoseconds, CLOCK_MONOTONIC. Never goes backwards within a
 * boot; not related to wall time. */
uint64_t pme_now_ns(void);

#endif /* PROBEME_COMMON_CLOCK_H */
