#pragma once

// provides very limited subset of semver
//TODO maybe replace this by a proper solution?

#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct semver {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} semver;

bool SEMVER_equal(semver l, semver r);

bool SEMVER_satisfies(semver required, semver provided);

#ifdef __cplusplus
}
#endif
