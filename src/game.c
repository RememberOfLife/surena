#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "surena/util/serialization.h"

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
    [ERR_MISSING_HIDDEN_STATE] = "missing hidden state",
    [ERR_INVALID_INPUT] = "invalid input",
    [ERR_INVALID_OPTIONS] = "invalid options",
    [ERR_UNSTABLE_POSITION] = "unstable position",
    [ERR_SYNC_COUNTER_MISMATCH] = "sync ctr mismatch",
    [ERR_RETRY] = "retry",
    [ERR_CUSTOM_ANY] = "custom any",
    [ERR_ENUM_DEFAULT_OFFSET] = NULL,
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
    if (*pbuf != NULL) {
        free(*pbuf);
        *pbuf = NULL;
    }
    if (fmt != NULL) {
        va_list args;
        va_start(args, fmt);
        size_t len = vsnprintf(NULL, 0, fmt, args) + 1;
        va_end(args);
        *pbuf = (char*)malloc(len);
        if (*pbuf == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        va_start(args, fmt);
        vsnprintf(*pbuf, len, fmt, args);
        va_end(args);
    }
    return ec;
}

void game_init_create_standard(game_init* init_info, const char* opts, const char* legacy, const char* state)
{
    *init_info = (game_init){
        .source_type = GAME_INIT_SOURCE_TYPE_STANDARD,
        .source = {
            .standard = {
                .opts = (opts == NULL ? NULL : strdup(opts)),
                .legacy = (legacy == NULL ? NULL : strdup(legacy)),
                .state = (state == NULL ? NULL : strdup(state)),
            },
        },
    };
}

void game_init_create_serialized(game_init* init_info, void* buf_begin, void* buf_end)
{
    init_info->source_type = GAME_INIT_SOURCE_TYPE_SERIALIZED;
    size_t buf_size = ptrdiff(buf_end, buf_begin);
    assert(buf_size > 0);
    void* new_buf = malloc(buf_size);
    init_info->source.serialized.buf_begin = new_buf;
    init_info->source.serialized.buf_end = ptradd(new_buf, buf_size);
    memcpy(new_buf, buf_begin, buf_size);
}

//TODO check again that this works as intended
size_t sl_game_init_info_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    // flatten the unions, this encodes more data than required, but keeps complexity down

    typedef struct flat_game_init_s {
        uint32_t source_type;
        const char* opts;
        const char* legacy;
        const char* state;
        blob serialized;
    } flat_game_init;

    const serialization_layout sl_flat_game_init[] = {
        {SL_TYPE_U32, offsetof(flat_game_init, source_type)},
        {SL_TYPE_STRING, offsetof(flat_game_init, opts)},
        {SL_TYPE_STRING, offsetof(flat_game_init, legacy)},
        {SL_TYPE_STRING, offsetof(flat_game_init, state)},
        {SL_TYPE_BLOB, offsetof(flat_game_init, serialized)},
        {SL_TYPE_STOP},
    };

    flat_game_init fgi_in;
    flat_game_init fgi_out;

    game_init* cin_p = (game_init*)obj_in;
    game_init* cout_p = (game_init*)obj_out;

    // size, copy, serialize, destroy: obj_in -> flat in
    if (itype == GSIT_SIZE || itype == GSIT_COPY || itype == GSIT_SERIALIZE || itype == GSIT_DESTROY) {
        fgi_in.source_type = cin_p->source_type;
        if (cin_p->source_type == GAME_INIT_SOURCE_TYPE_STANDARD) {
            fgi_in.opts = cin_p->source.standard.opts;
            fgi_in.legacy = cin_p->source.standard.legacy;
            fgi_in.state = cin_p->source.standard.state;
        } else {
            fgi_in.opts = NULL;
            fgi_in.legacy = NULL;
            fgi_in.state = NULL;
        }
        if (cin_p->source_type == GAME_INIT_SOURCE_TYPE_SERIALIZED) {
            fgi_in.serialized = (blob){
                .data = cin_p->source.serialized.buf_begin,
                .len = ptrdiff(cin_p->source.serialized.buf_end, cin_p->source.serialized.buf_begin),
            };
        } else {
            fgi_in.serialized = (blob){
                .data = NULL,
                .len = 0,
            };
        }
    }

    //serialize
    size_t rsize = layout_serializer(itype, sl_flat_game_init, &fgi_in, &fgi_out, buf, buf_end);
    if (rsize == LS_ERR) {
        return LS_ERR;
    }

    // initzero: flat_in -> obj_in
    if (itype == GSIT_INITZERO) {
        fgi_out = fgi_in;
        obj_out = obj_in;
    }

    // deserialize, copy: flat_out -> obj_out
    if (itype == GSIT_DESERIALIZE || itype == GSIT_COPY) {
        cout_p->source_type = fgi_out.source_type;
        if (fgi_out.source_type == GAME_INIT_SOURCE_TYPE_STANDARD) {
            cout_p->source.standard.opts = fgi_out.opts;
            cout_p->source.standard.legacy = fgi_out.legacy;
            cout_p->source.standard.state = fgi_out.state;
        }
        if (fgi_out.source_type == GAME_INIT_SOURCE_TYPE_SERIALIZED) {
            cout_p->source.serialized.buf_begin = fgi_out.serialized.data;
            cout_p->source.serialized.buf_end = ptradd(fgi_out.serialized.data, fgi_out.serialized.len);
        }
    }

    return rsize;
}

#ifdef __cplusplus
}
#endif
