### boilerplate code for games

//TODO general position for data_state_repr and a function to get a ref to it?

thegame.h
```cpp
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "surena/game.h"

typedef struct thegame_options_s {
    uint32_t var;
} thegame_options;

typedef struct thegame_internal_methods_s {

    // docs for internal_call
    error_code (*internal_call)(game* self, int x);

    /* other exposed internals */

} thegame_internal_methods;

extern const game_methods thegame_gbe;

#ifdef __cplusplus
}
#endif
```

//TODO namespace isn't *really* necessary, but makes accessing surena internals easier
//TODO find some way to remove the forward declaration boilerplate
use `GF_UNUSED(game_function_name);` to mark a feature function as not supported, for both true features and feature flags
thegame.cpp
```cpp
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/thegame.h"

namespace {
    
    // general purpose helpers for opts, data, errors

    error_code _rerrorf(game* self, error_code ec, const char* fmt, ...)
    {
        if (self->data2 == NULL) {
            self->data2 = malloc(1024); //TODO correct size from where?
        }
        va_list args;
        va_start(args, fmt);
        vsprintf((char*)self->data2, fmt, args);
        va_end(args);
        return ec;
    }
    
    typedef thegame_options opts_repr;

    struct data_repr {
        opts_repr opts;

        uint32_t state;
    };

    opts_repr& _get_opts(game* self)
    {
        return ((data_repr*)(self->data1))->opts;
    }

    data_repr& _get_repr(game* self)
    {
        return *((data_repr*)(self->data1));
    }

    const char* _get_last_error(game* self);
    error_code _create_with_opts_str(game* self, const char* str);
    error_code _create_with_opts_bin(game* self, void* options_struct);
    error_code _create_default(game* self);
    error_code _export_options_str(game* self, size_t* ret_size, char* str);
    error_code _get_options_bin_ref(game* self, void** ret_bin_ref);
    error_code _destroy(game* self);
    error_code _clone(game* self, game* clone_target);
    error_code _copy_from(game* self, game* other);
    error_code _compare(game* self, game* other, bool* ret_equal);
    error_code _import_state(game* self, const char* str);
    error_code _export_state(game* self, size_t* ret_size, char* str);
    error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players);
    error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    error_code _get_concrete_move_probabilities(game* self, player_id player, uint32_t* ret_count, float* move_probabilities);
    error_code _get_concrete_moves_ordered(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    error_code _get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    error_code _is_legal_move(game* self, player_id player, move_code move, sync_counter sync);
    error_code _move_to_action(game* self, move_code move, move_code* ret_action);
    error_code _is_action(game* self, move_code move, bool* ret_is_action);
    error_code _make_move(game* self, player_id player, move_code move);
    error_code _get_results(game* self, uint8_t* ret_count, player_id* players);
    error_code _get_sync_counter(game* self, sync_counter* ret_sync);
    error_code _id(game* self, uint64_t* ret_id);
    error_code _eval(game* self, player_id player, float* ret_eval);
    error_code _discretize(game* self, uint64_t seed);
    error_code _playout(game* self, uint64_t seed);
    error_code _redact_keep_state(game* self, uint8_t count, player_id* players);
    error_code _export_sync_data(game* self, sync_data** sync_data_start, sync_data** sync_data_end);
    error_code _release_sync_data(game* self, sync_data* sync_data_start, sync_data* sync_data_end);
    error_code _import_sync_data(game* self, void* data_start, void* data_end);
    error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    error_code _debug_print(game* self, size_t* ret_size, char* str_buf);

    /* same for internals */

    // implementation

    const char* _get_last_error(game* self)
    {
        return (char*)self->data2; // in this scheme opts are saved together with the state in data1, and data2 is the last error string
    }

    error_code _create_with_opts_str(game* self, const char* str)
    {
        //TODO
    }

    error_code _create_with_opts_bin(game* self, void* options_struct)
    {
        //TODO
    }

    error_code _create_default(game* self)
    {
        //TODO
    }

    error_code _export_options_str(game* self, size_t* ret_size, char* str)
    {
        //TODO
    }

    error_code _get_options_bin_ref(game* self, void** ret_bin_ref)
    {
        //TODO
    }

    error_code _destroy(game* self)
    {
        //TODO
    }

    error_code _clone(game* self, game* clone_target)
    {
        //TODO
    }
    
    error_code _copy_from(game* self, game* other)
    {
        //TODO
    }

    error_code _compare(game* self, game* other, bool* ret_equal)
    {
        //TODO
    }

    error_code _import_state(game* self, const char* str)
    {
        //TODO
    }

    error_code _export_state(game* self, size_t* ret_size, char* str)
    {
        //TODO
    }

    error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        //TODO
    }

    error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
    }

    error_code _get_concrete_move_probabilities(game* self, player_id player, uint32_t* ret_count, float* move_probabilities)
    {
        //TODO
    }

    error_code _get_concrete_moves_ordered(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
    }

    error_code _get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
    }

    error_code _is_legal_move(game* self, player_id player, move_code move, sync_counter sync)
    {
        //TODO
    }

    error_code _move_to_action(game* self, move_code move, move_code* ret_action)
    {
        //TODO
    }

    error_code _is_action(game* self, move_code move, bool* ret_is_action)
    {
        //TODO
    }

    error_code _make_move(game* self, player_id player, move_code move)
    {
        //TODO
    }

    error_code _get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        //TODO
    }

    error_code get_sync_counter(game* self, sync_counter* ret_sync)
    {
        //TODO
    }

    error_code _id(game* self, uint64_t* ret_id)
    {
        //TODO
    }

    error_code _eval(game* self, player_id player, float* ret_eval)
    {
        //TODO
    }

    error_code _discretize(game* self, uint64_t seed)
    {
        //TODO
    }

    error_code _playout(game* self, uint64_t seed)
    {
        //TODO
    }

    error_code _redact_keep_state(game* self, uint8_t count, player_id* players)
    {
        //TODO
    }

    error_code _export_sync_data(game* self, sync_data** sync_data_start, sync_data** sync_data_end)
    {
        //TODO
    }

    error_code _release_sync_data(game* self, sync_data* sync_data_start, sync_data* sync_data_end)
    {
        //TODO
    }

    error_code _import_sync_data(game* self, void* data_start, void* data_end)
    {
        //TODO
    }

    error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        //TODO
    }

    error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf)
    {
        //TODO
    }

    error_code _debug_print(game* self, size_t* ret_size, char* str_buf)
    {
        //TODO
    }

    //=====
    // game internal methods

    // error_code _internal_call(game* self, int x)
    // {
    //     //TODO
    // }

}

// static const thegame_internal_methods thegame_gbe_internal_methods{
//     .internal_call = _internal_call,
// };

const game_methods tictactoe_gbe{

    .game_name = "TheGame",
    .variant_name = "TheOne",
    .impl_name = "This",
    .version = semver{1, 0, 0},
    .features = game_feature_flags{
        .options = true,
        .options_bin = true,
        .options_bin_ref = true,
        .random_moves = true,
        .hidden_information = true,
        .simultaneous_moves = true,
        .sync_counter = true,
        .move_ordering = true,
        .id = true,
        .eval = true,
        .playout = true,
        .print = true,
    },
    .internal_methods = NULL, // (void*)&thegame_gbe_internal_methods,
    
    #include "surena/game_impl.h"
    
};
```
