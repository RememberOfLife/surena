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
    [ERR_SYNC_COUNTER_IMPOSSIBLE_REORDER] = "sync counter impossible reorder",
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

void game_init_create_serialized(game_init* init_info, blob b)
{
    init_info->source_type = GAME_INIT_SOURCE_TYPE_SERIALIZED;
    blob_create(&init_info->source.serialized.b, b.len);
    if (b.len > 0) {
        memcpy(init_info->source.serialized.b.data, b.data, b.len);
    }
}

const serialization_layout sl_game_init_info_standard[] = {
    {SL_TYPE_STRING, offsetof(game_init_standard, opts)},
    {SL_TYPE_STRING, offsetof(game_init_standard, legacy)},
    {SL_TYPE_STRING, offsetof(game_init_standard, state)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_game_init_info_serialized[] = {
    {SL_TYPE_STRING, offsetof(game_init_serialized, b)},
    {SL_TYPE_STOP},
};

const serialization_layout* sl_game_init_info_map[GAME_INIT_SOURCE_TYPE_COUNT] = {
    [GAME_INIT_SOURCE_TYPE_DEFAULT] = NULL,
    [GAME_INIT_SOURCE_TYPE_STANDARD] = sl_game_init_info_standard,
    [GAME_INIT_SOURCE_TYPE_SERIALIZED] = sl_game_init_info_serialized,
};

const serialization_layout sl_game_init_info[] = {
    {
        SL_TYPE_UNION_EXTERNALLY_TAGGED,
        offsetof(game_init, source),
        .ext.un = {
            .tag_size = sizeof(GAME_INIT_SOURCE_TYPE),
            .tag_offset = offsetof(game_init, source_type),
            .tag_max = GAME_INIT_SOURCE_TYPE_COUNT,
            .tag_map = sl_game_init_info_map,
        },
    },
    {SL_TYPE_STOP},
};

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

error_code game_export_options(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(ret_str);
    assert(player != PLAYER_RAND);
    return self->methods->export_options(self, player, ret_size, ret_str);
}

error_code player_count(game* self, uint8_t* ret_count)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    return self->methods->player_count(self, ret_count);
}

error_code game_export_state(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(ret_str);
    assert(player != PLAYER_RAND);
    return self->methods->export_state(self, player, ret_size, ret_str);
}

error_code game_import_state(game* self, const char* str)
{
    assert(self);
    assert(self->methods);
    assert(str);
    return self->methods->import_state(self, str);
}

error_code game_serialize(game* self, player_id player, const blob** ret_blob)
{
    assert(self);
    assert(self->methods);
    assert(ret_blob);
    assert(player != PLAYER_RAND);
    return self->methods->serialize(self, player, ret_blob);
}

error_code game_players_to_move(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_players);
    return self->methods->players_to_move(self, ret_count, ret_players);
}

error_code game_get_concrete_moves(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_moves);
    assert(player != PLAYER_NONE);
    return self->methods->get_concrete_moves(self, player, ret_count, ret_moves);
}

error_code game_get_concrete_move_probabilities(game* self, player_id player, uint32_t* ret_count, const float** ret_move_probabilities)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_move_probabilities);
    assert(player != PLAYER_NONE);
    return self->methods->get_concrete_move_probabilities(self, player, ret_count, ret_move_probabilities);
}

error_code game_get_concrete_moves_ordered(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_moves);
    assert(player != PLAYER_NONE);
    return self->methods->get_concrete_moves_ordered(self, player, ret_count, ret_moves);
}

error_code game_get_actions(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_moves);
    assert(player != PLAYER_NONE);
    return self->methods->get_actions(self, player, ret_count, ret_moves);
}

error_code game_is_legal_move(game* self, player_id player, move_data_sync move)
{
    assert(self);
    assert(self->methods);
    assert(player != PLAYER_NONE);
    if (self->sync_ctr != move.sync_ctr && game_ff(self).simultaneous_moves == false) {
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
    if (self->sync_ctr != move.sync_ctr && game_ff(self).simultaneous_moves == false) {
        return ERR_SYNC_COUNTER_MISMATCH;
    }
    return self->methods->move_to_action(self, player, move, ret_action);
}

error_code game_is_action(game* self, player_id player, move_data_sync move, bool* ret_is_action)
{
    assert(self);
    assert(self->methods);
    assert(ret_is_action);
    if (self->sync_ctr != move.sync_ctr && game_ff(self).simultaneous_moves == false) {
        return ERR_SYNC_COUNTER_MISMATCH;
    }
    return self->methods->is_action(self, player, move, ret_is_action);
}

error_code game_make_move(game* self, player_id player, move_data_sync move)
{
    assert(self);
    assert(self->methods);
    assert(player != PLAYER_NONE);
    error_code ec = game_is_legal_move(self, player, move);
    if (ec != ERR_OK) {
        return ec;
    }
    move_data_sync action;
    ec = game_move_to_action(self, player, move, &action);
    if (ec != ERR_OK) {
        return ec;
    }
    bool action_dropped =
        (self->methods->features.big_moves == true && blob_is_null(&action.md.big)) ||
        (self->methods->features.big_moves == false && action.md.code == MOVE_NONE);
    ec = self->methods->make_move(self, player, move);
    if (action_dropped == false) {
        self->sync_ctr++;
    }
    return ec;
}

error_code game_get_results(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_players);
    return self->methods->get_results(self, ret_count, ret_players);
}

error_code game_export_legacy(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(ret_str);
    assert(player != PLAYER_RAND);
    return self->methods->export_legacy(self, player, ret_size, ret_str);
}

error_code game_get_scores(game* self, size_t* ret_count, player_id* players, const int32_t** ret_scores)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(players);
    assert(ret_scores);
    return self->methods->get_scores(self, ret_count, players, ret_scores);
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
    assert(player != PLAYER_NONE && player != PLAYER_RAND);
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

error_code game_redact_keep_state(game* self, uint8_t count, const player_id* players)
{
    assert(self);
    assert(self->methods);
    assert(players);
    return self->methods->redact_keep_state(self, count, players);
}

error_code game_export_sync_data(game* self, uint32_t* ret_count, const sync_data** ret_sync_data)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_sync_data);
    return self->methods->export_sync_data(self, ret_count, ret_sync_data);
}

error_code game_import_sync_data(game* self, blob b)
{
    assert(self);
    assert(self->methods);
    assert(!blob_is_null(&b));
    return self->methods->import_sync_data(self, b);
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

error_code game_get_move_str(game* self, player_id player, move_data_sync move, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(ret_str);
    assert(player != PLAYER_NONE);
    if (self->sync_ctr != move.sync_ctr && game_ff(self).simultaneous_moves == false) {
        return ERR_SYNC_COUNTER_MISMATCH;
    }
    return self->methods->get_move_str(self, player, move, ret_size, ret_str);
}

error_code game_print(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(ret_str);
    assert(player != PLAYER_NONE && player != PLAYER_RAND);
    return self->methods->print(self, player, ret_size, ret_str);
}

bool game_e_move_is_none(game* self, move_data move)
{
    assert(self);
    assert(self->methods);
    if (game_ff(self).big_moves == false) {
        return move.code == MOVE_NONE;
    } else {
        return blob_is_null(&move.big);
    }
}

move_data game_e_move_copy(game* self, move_data move)
{
    assert(self);
    assert(self->methods);
    if (game_ff(self).big_moves == false) {
        return move;
    }
    move_data ret;
    ls_primitive_blob_serializer(GSIT_COPY, &move.big, &ret.big, NULL, NULL);
    return ret;
}

move_data_sync game_e_move_sync_copy(game* self, move_data_sync move)
{
    assert(self);
    assert(self->methods);
    move_data_sync ret = move;
    ret.md = game_e_move_copy(self, move.md);
    ret.sync_ctr = self->sync_ctr;
    return ret;
}

move_data_sync game_e_move_make_sync(game* self, move_data move)
{
    assert(self);
    assert(self->methods);
    return (move_data_sync){.md = move, .sync_ctr = self->sync_ctr};
}

#ifdef __cplusplus
}
#endif
