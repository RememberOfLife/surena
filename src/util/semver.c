#include <stdbool.h>
#include <stdint.h>

#include "surena/util/semver.h"

#ifdef __cplusplus
extern "C" {
#endif

bool SEMVER_equal(semver l, semver r)
{
    return l.major == r.major && l.minor == r.minor && l.patch == r.patch;
}

bool SEMVER_satisfies(semver required, semver provided)
{
    return provided.major == required.major && (provided.minor > required.minor || (provided.minor == required.minor && provided.patch >= required.patch));
}

#ifdef __cplusplus
}
#endif
