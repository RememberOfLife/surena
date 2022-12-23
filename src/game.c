#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rosalia/serialization.h"

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
    [ERR_MISSING_HIDDEN_STATE] = "missing hidden state",
    [ERR_INVALID_INPUT] = "invalid input",
    [ERR_INVALID_PLAYER] = "invalid player",
    [ERR_INVALID_MOVE] = "invalid move",
    [ERR_INVALID_OPTIONS] = "invalid options",
    [ERR_INVALID_LEGACY] = "invalid legacy",
    [ERR_INVALID_STATE] = "invalid state",
    [ERR_UNSTABLE_POSITION] = "unstable position",
    [ERR_SYNC_COUNTER_MISMATCH] = "sync counter mismatch",
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

//TODO sl labout
const serialization_layout sl_game_init_info[] = {
    {SL_TYPE_STOP},
};

//TODO check again that this works as intended
size_t sl_game_init_info_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    // flatten the unions, this encodes more data than required, but keeps complexity down

    typedef struct flat_game_init_s {
        uint8_t source_type;
        const char* opts;
        const char* legacy;
        const char* state;
        blob serialized;
    } flat_game_init;

    const serialization_layout sl_flat_game_init[] = {
        {SL_TYPE_U8, offsetof(flat_game_init, source_type)},
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

const char* game_gname(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->game_name;
}

const char* game_vname(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->variant_name;
}

const char* game_iname(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->impl_name;
}

const semver game_version(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->version;
}

const game_feature_flags game_ff(game* self)
{
    assert(self);
    return self->methods->features;
}

const char* game_get_last_error(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->get_last_error(self);
}

error_code game_create(game* self, game_init* init_info)
{
    assert(self);
    assert(self->methods);
    assert(init_info);
    return self->methods->create(self, init_info);
}

error_code game_destroy(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->destroy(self);
}

error_code game_clone(game* self, game* clone_target)
{
    assert(self);
    assert(self->methods);
    assert(clone_target);
    return self->methods->clone(self, clone_target);
}

error_code game_copy_from(game* self, game* other)
{
    assert(self);
    assert(self->methods);
    assert(other);
    return self->methods->copy_from(self, other);
}

error_code game_compare(game* self, game* other, bool* ret_equal)
{
    assert(self);
    assert(self->methods);
    assert(other);
    assert(ret_equal);
    return self->methods->compare(self, other, ret_equal);
}

error_code game_export_options(game* self, size_t* ret_size, char* str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(str);
    return self->methods->export_options(self, ret_size, str);
}

error_code game_export_state(game* self, size_t* ret_size, char* str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(str);
    return self->methods->export_state(self, ret_size, str);
}

error_code game_import_state(game* self, const char* str)
{
    assert(self);
    assert(self->methods);
    assert(str);
    return self->methods->import_state(self, str);
}

error_code game_serialize(game* self, size_t* ret_size, char* buf)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(buf);
    return self->methods->serialize(self, ret_size, buf);
}

error_code game_players_to_move(game* self, uint8_t* ret_count, player_id* players)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(players);
    return self->methods->players_to_move(self, ret_count, players);
}

error_code game_get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_data* moves)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(moves);
    assert(player != PLAYER_NONE);
    return self->methods->get_concrete_moves(self, player, ret_count, moves);
}

error_code game_get_concrete_move_probabilities(game* self, player_id player, uint32_t* ret_count, float* move_probabilities)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(move_probabilities);
    assert(player != PLAYER_NONE);
    return self->methods->get_concrete_move_probabilities(self, player, ret_count, move_probabilities);
}

error_code game_get_concrete_moves_ordered(game* self, player_id player, uint32_t* ret_count, move_data* moves)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(moves);
    assert(player != PLAYER_NONE);
    return self->methods->get_concrete_moves_ordered(self, player, ret_count, moves);
}

error_code game_get_actions(game* self, player_id player, uint32_t* ret_count, move_data* moves)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(moves);
    assert(player != PLAYER_NONE);
    return self->methods->get_actions(self, player, ret_count, moves);
}

error_code game_is_legal_move(game* self, player_id player, move_data_sync move)
{
    assert(self);
    assert(self->methods);
    assert(player != PLAYER_NONE);
    assert(move.sync_ctr == self->sync_ctr);
    if (self->methods->features.simultaneous_moves == false && move.sync_ctr != self->sync_ctr) {
        return ERR_SYNC_COUNTER_MISMATCH;
    }
    return self->methods->is_legal_move(self, player, move);
}

error_code game_move_to_action(game* self, player_id player, move_data_sync move, move_data_sync* ret_action)
{
    assert(self);
    assert(self->methods);
    assert(ret_action);
    assert(player != PLAYER_NONE);
    assert(move.sync_ctr == self->sync_ctr);
    return self->methods->move_to_action(self, player, move, ret_action);
}

error_code game_is_action(game* self, move_data_sync move, bool* ret_is_action)
{
    assert(self);
    assert(self->methods);
    assert(ret_is_action);
    assert(move.sync_ctr == self->sync_ctr);
    return self->methods->is_action(self, move, ret_is_action);
}

error_code game_make_move(game* self, player_id player, move_data_sync move)
{
    assert(self);
    assert(self->methods);
    assert(player != PLAYER_NONE);
    assert(move.sync_ctr == self->sync_ctr);
    assert(game_is_legal_move(self, player, move) == ERR_OK); //TODO want this in release too?
    move_data_sync action;
    game_move_to_action(self, player, move, &action);
    bool action_dropped =
        (self->methods->features.big_moves == true && action.r.big.len == 0) ||
        (self->methods->features.big_moves == false && action.r.code == MOVE_NONE);
    error_code ec = self->methods->make_move(self, player, move);
    if (action_dropped == false) {
        self->sync_ctr++;
    }
    return ec;
}

error_code game_get_results(game* self, uint8_t* ret_count, player_id* players)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(players);
    return self->methods->get_results(self, ret_count, players);
}

error_code game_export_legacy(game* self, size_t* ret_size, char* str_buf)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(str_buf);
    return self->methods->export_legacy(self, ret_size, str_buf);
}

error_code game_get_scores(game* self, size_t* ret_count, player_id* players, int32_t* scores)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(players);
    assert(scores);
    return self->methods->get_scores(self, ret_count, players, scores);
}

error_code game_id(game* self, uint64_t* ret_id)
{
    assert(self);
    assert(self->methods);
    assert(ret_id);
    return self->methods->id(self, ret_id);
}

error_code game_eval(game* self, player_id player, float* ret_eval)
{
    assert(self);
    assert(self->methods);
    assert(ret_eval);
    assert(player != PLAYER_NONE);
    return self->methods->eval(self, player, ret_eval);
}

error_code game_discretize(game* self, uint64_t seed)
{
    assert(self);
    assert(self->methods);
    return self->methods->discretize(self, seed);
}

error_code game_playout(game* self, uint64_t seed)
{
    assert(self);
    assert(self->methods);
    assert(seed != SEED_NONE);
    return self->methods->playout(self, seed);
}

error_code game_redact_keep_state(game* self, uint8_t count, player_id* players)
{
    assert(self);
    assert(self->methods);
    assert(players);
    return self->methods->redact_keep_state(self, count, players);
}

error_code game_export_sync_data(game* self, sync_data** sync_data_start, sync_data** sync_data_end)
{
    assert(self);
    assert(self->methods);
    assert(sync_data_start);
    assert(sync_data_end);
    return self->methods->export_sync_data(self, sync_data_start, sync_data_end);
}

error_code game_import_sync_data(game* self, void* data_start, void* data_end)
{
    assert(self);
    assert(self->methods);
    assert(data_start);
    assert(data_end);
    assert(data_start < data_end);
    return self->methods->import_sync_data(self, data_start, data_end);
}

error_code game_get_move_data(game* self, player_id player, const char* str, move_data_sync* ret_move)
{
    assert(self);
    assert(self->methods);
    assert(str);
    assert(ret_move);
    assert(player != PLAYER_NONE);
    error_code ec = self->methods->get_move_data(self, player, str, ret_move);
    ret_move->sync_ctr = self->sync_ctr;
    return ec;
}

error_code game_get_move_str(game* self, player_id player, move_data_sync move, size_t* ret_size, char* str_buf)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(str_buf);
    assert(player != PLAYER_NONE);
    assert(move.sync_ctr == self->sync_ctr);
    return self->methods->get_move_str(self, player, move, ret_size, str_buf);
}

error_code game_print(game* self, player_id player, size_t* ret_size, char* str_buf)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(str_buf);
    assert(player != PLAYER_NONE);
    return self->methods->print(self, player, ret_size, str_buf);
}

move_data game_e_move_copy(game* self, move_data move)
{
    //TODO
}

move_data_sync game_e_move_sync_copy(game* self, move_data_sync move)
{
    //TODO
}

move_data_sync game_e_move_make_sync(game* self, move_data move)
{
    assert(self);
    //TODO how to convert without looking at feature flags?
    //TODO maybe truly do want to separate sync ctr from move and not deal with two move formats?
    if (self->methods->features.big_moves) {
        return (move_data_sync){.r.big = move.r.big, .sync_ctr = self->sync_ctr};
    } else {
        return (move_data_sync){.r.code = move.r.code, .sync_ctr = self->sync_ctr};
    };
}

#ifdef __cplusplus
}
#endif
