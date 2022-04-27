#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "surena/game.h"

const char* general_error_strings[] = {
    [ERR_OK] = "OK",
    [ERR_STATE_UNRECOVERABLE] = "state unrecoverable",
    [ERR_STATE_CORRUPTED] = "state corrupted",
    [ERR_OUT_OF_MEMORY] = "out of memory",
    [ERR_FEATURE_UNSUPPORTED] = "feature unsupported",
    [ERR_STATE_UNINITIALIZED] = "state uninitialized",
    [ERR_INVALID_INPUT] = "invalid input",
    [ERR_INVALID_OPTIONS] = "invalid options",
    [ERR_UNSTABLE_POSITION] = "unstable position",
    [ERR_SYNC_CTR_MISMATCH] = "sync ctr mismatch",
};

const char* get_general_error_string(error_code err)
{
    if (err < ERR_ENUM_DEFAULT_OFFSET) {
        return general_error_strings[err];
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif
