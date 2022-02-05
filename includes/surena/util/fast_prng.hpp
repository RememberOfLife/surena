#ifndef FAST_PRNG_HPP
#define FAST_PRNG_HPP

#include <cstdint>

// using PCG32 minimal seeded via splitmix64
struct fast_prng {
    uint64_t state;
    uint64_t inc;
    void srand(uint64_t seed);
    uint32_t rand();

    fast_prng(uint64_t seed);
};

#endif
