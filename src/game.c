#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    [ERR_SYNC_COUNTER_MISMATCH] = "sync ctr mismatch",
    [ERR_RETRY] = "retry",
    [ERR_CUSTOM_UNSPEC] = "custom unspec",
};

const char* get_general_error_string(error_code err)
{
    if (err < ERR_ENUM_DEFAULT_OFFSET) {
        return general_error_strings[err];
    }
    return NULL;
}

error_code rerrorf(char** pbuf, error_code ec, const char* fmt, ...)
{
    if (pbuf == NULL) {
        return ec;
    }
    if (ec == ERR_OK) {
        free(*pbuf);
        *pbuf = NULL;
        return ERR_OK;
    }
    if (*pbuf != NULL) {
        free(*pbuf);
    }
    va_list args;
    va_start(args, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);
    *pbuf = malloc(len);
    if (*pbuf == NULL) {
        return ERR_OUT_OF_MEMORY;
    }
    va_start(args, fmt);
    vsnprintf(*pbuf, len, fmt, args);
    va_end(args);
    return ec;
}

#ifdef __cplusplus
}
#endif
