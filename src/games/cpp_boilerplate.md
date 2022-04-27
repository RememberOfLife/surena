### boilerplate code for games

//TODO
how to represent game specific enums?
e.g. tictactoe player and result enums

//TODO general position for data_state_repr and a function to get a ref to it?

thegame.h
```cpp
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "surena/game.h"

typedef struct thegame_options {
    uint32_t var;
} thegame_options;

typedef struct thegame_internal_methods {

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
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/thegame.h"

namespace surena {
    
    // game data state representation and general getter
    
    typedef thegame_options opts_repr;
    opts_repr& _get_opts(game* self)
    {
        return *((opts_repr*)(self->options));
    }

    typedef struct data_repr {
        uint32_t state;
    } data_repr;

    data_repr& _get_repr(game* self)
    {
        return *((data_repr*)(self->data));
    }

    static const char* _get_error_string(error_code err);
    static error_code _import_options_bin(game* self, void* options_struct);
    static error_code _import_options_str(game* self, const char* str);
    static error_code _export_options_str(game* self, size_t* ret_size, char* str);
    static error_code _create(game* self);
    static error_code _destroy(game* self);
    static error_code _clone(game* self, game** ret_clone);
    static error_code _copy_from(game* self, game* other);
    static error_code _compare(game* self, game* other, bool* ret_equal);
    static error_code _import_state(game* self, const char* str);
    static error_code _export_state(game* self, size_t* ret_size, char* str);
    static error_code _get_player_count(game* self, uint8_t* ret_count);
    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players);
    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    static error_code _get_concrete_move_probabilities(game* self, player_id player, uint32_t* ret_count, float* move_probabilities);
    static error_code _get_concrete_moves_ordered(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    static error_code _get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    static error_code _is_legal_move(game* self, player_id player, move_code move, uint32_t sync_ctr);
    static error_code _move_to_action(game* self, move_code move, move_code* ret_action);
    static error_code _is_action(game* self, move_code move, bool* ret_is_action);
    static error_code _make_move(game* self, player_id player, move_code move);
    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players);
    static error_code _id(game* self, uint64_t* ret_id);
    static error_code _eval(game* self, player_id player, float* ret_eval);
    static error_code _discretize(game* self, uint64_t seed);
    static error_code _playout(game* self, uint64_t seed);
    static error_code _redact_keep_state(game* self, uint8_t count, player_id* players);
    static error_code _export_sync_data_gf_t(game* self, sync_data** sync_data_start, sync_data** sync_data_end);
    static error_code _import_sync_data_gf_t(game* self, void* data_start, void* data_end);
    static error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    static error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf);

    /* same for internals */

    // implementation

    static const char* _get_error_string(error_code err)
    {
        //TODO
    }

    static error_code _import_options_bin(game* self, void* options_struct)
    {
        //TODO
    }

    static error_code _import_options_str(game* self, const char* str)
    {
        //TODO
    }
    
    static error_code _export_options_str(game* self, size_t* ret_size, char* str)
    {
        //TODO
    }
    
    static error_code _create(game* self)
    {
        //TODO
    }

    static error_code _destroy(game* self)
    {
        //TODO
    }

    static error_code _clone(game* self, game** ret_clone)
    {
        //TODO
    }
    
    static error_code _copy_from(game* self, game* other)
    {
        //TODO
    }

    static error_code _compare(game* self, game* other, bool* ret_equal)
    {
        //TODO
    }

    static error_code _import_state(game* self, const char* str)
    {
        //TODO
    }

    static error_code _export_state(game* self, size_t* ret_size, char* str)
    {
        //TODO
    }

    static error_code _get_player_count(game* self, uint8_t* ret_count)
    {
        //TODO
    }

    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        //TODO
    }

    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
    }

    static error_code _get_concrete_move_probabilities(game* self, player_id player, uint32_t* ret_count, float* move_probabilities)
    {
        //TODO
    }

    static error_code _get_concrete_moves_ordered(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
    }

    static error_code _get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
    }

    static error_code _is_legal_move(game* self, player_id player, move_code move, uint32_t sync_ctr)
    {
        //TODO
    }

    static error_code _move_to_action(game* self, move_code move, move_code* ret_action)
    {
        //TODO
    }

    static error_code _is_action(game* self, move_code move, bool* ret_is_action)
    {
        //TODO
    }

    static error_code _make_move(game* self, player_id player, move_code move)
    {
        //TODO
    }

    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        //TODO
    }

    static error_code _id(game* self, uint64_t* ret_id)
    {
        //TODO
    }

    static error_code _eval(game* self, player_id player, float* ret_eval)
    {
        //TODO
    }

    static error_code _discretize(game* self, uint64_t seed)
    {
        //TODO
    }

    static error_code _playout(game* self, uint64_t seed)
    {
        //TODO
    }

    static error_code _redact_keep_state(game* self, uint8_t count, player_id* players)
    {
        //TODO
    }

    static error_code _export_sync_data_gf_t(game* self, sync_data** sync_data_start, sync_data** sync_data_end)
    {
        //TODO
    }

    static error_code _import_sync_data_gf_t(game* self, void* data_start, void* data_end)
    {
        //TODO
    }

    static error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        //TODO
    }

    static error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf)
    {
        //TODO
    }

    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf)
    {
        //TODO
    }

    //=====
    // game internal methods

    // static error_code _internal_call(game* self, int x)
    // {
    //     //TODO
    // }

}

// static const thegame_internal_methods thegame_gbe_internal_methods{
//     .internal_call = surena::_internal_call,
// };

const game_methods tictactoe_gbe{

    .game_name = "TheGame",
    .variant_name = "TheOne",
    .impl_name = "This",
    .version = semver{1, 0, 0},
    .features = game_feature_flags{
        .random_moves = true,
        .hidden_information = false,
        .simultaneous_moves = false,
    },
    .internal_methods = NULL, // (void*)&thegame_gbe_internal_methods,
    
    #include "surena/game_impl.h"
    
};
```

#### currently not in use

```cpp
class TheGame : public Game {
        
    public:

        TheGame();

        //#####
        // internal methods
        
        error_code internal_call(int x);

};
```

**G**ame wrapper internal functions
```cpp
error_code TheGame::internal_call(int x)
{
    return ((thegame_internal_methods*)gbe.methods->internal_methods)->get_cell(&gbe, x)
    {
        //TODO
    }
}
```