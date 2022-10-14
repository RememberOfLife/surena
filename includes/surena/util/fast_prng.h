#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// using PCG32 minimal seeded via splitmix64
typedef struct fast_prng_t {
    uint64_t state;
    uint64_t inc;
} fast_prng;

void fprng_srand(fast_prng* fprng, uint64_t seed);
uint32_t fprng_rand(fast_prng* fprng);

#ifdef __cplusplus
}
#endif
