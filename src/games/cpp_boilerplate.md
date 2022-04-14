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
thegame.cpp
```cpp
#include "surena/util/semver.h"
#include "surena/game.h"

#include "surena/games/thegame.h"

namespace surena {
    
    // game data state representation and general getter

    typedef struct data_repr {
        uint32_t state;
    } data_repr;

    data_repr& _get_repr(game* self)
    {
        return *((data_repr*)(self->data));
    }

    // forward declare everything to allow for inlining at least in this unit

    static const char* _get_error_string(error_code err);
    static error_code _create(game* self);
    static error_code _destroy(game* self);
    static error_code _clone(game* self, game** ret_clone);
    static error_code _copy_from(game* self, game* other);
    static error_code _compare(game* self, game* other, bool* ret_equal);
    static error_code _import_state(game* self, const char* str);
    static error_code _export_state(game* self, size_t* ret_size, char* str);
    static error_code _get_player_count(game* self, uint8_t* ret_count);
    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players);
    static error_code _get_concrete_moves(game* self, uint32_t* ret_count, move_code* moves, player_id player);
    static error_code _get_concrete_move_probabilities(game* self, uint32_t* ret_count, float* move_probabilities, player_id player);
    static error_code _get_concrete_moves_ordered(game* self, uint32_t* ret_count, move_code* moves, player_id player);
    static error_code _get_actions(game* self, uint32_t* ret_count, move_code* moves, player_id player);
    static error_code _is_legal_move(game* self, player_id player, move_code move, uint32_t sync_ctr);
    static error_code _move_to_action(game* self, move_code* ret_action, move_code move);
    static error_code _is_action(game* self, bool* ret_is_action, move_code move);
    static error_code _make_move(game* self, player_id player, move_code move);
    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players);
    static error_code _id(game* self, uint64_t* ret_id);
    static error_code _eval(game* self, float* ret_eval, player_id player);
    static error_code _discretize(game* self, uint64_t seed);
    static error_code _playout(game* self, uint64_t seed);
    static error_code _redact_keep_state(game* self, player_id* players, uint32_t count);
    static error_code _get_move_code(game* self, move_code* ret_move, const char* str);
    static error_code _get_move_str(game* self, size_t* ret_size, char* str_buf, move_code move);
    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf);

    /* same for internals */

    // implementation

    static const char* _get_error_string(error_code err)
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

    static error_code _get_concrete_moves(game* self, uint32_t* ret_count, move_code* moves, player_id player)
    {
        //TODO
    }

    static error_code _get_concrete_move_probabilities(game* self, uint32_t* ret_count, float* move_probabilities, player_id player)
    {
        //TODO
    }

    static error_code _get_concrete_moves_ordered(game* self, uint32_t* ret_count, move_code* moves, player_id player)
    {
        //TODO
    }

    static error_code _get_actions(game* self, uint32_t* ret_count, move_code* moves, player_id player)
    {
        //TODO
    }

    static error_code _is_legal_move(game* self, player_id player, move_code move, uint32_t sync_ctr)
    {
        //TODO
    }

    static error_code _move_to_action(game* self, move_code* ret_action, move_code move)
    {
        //TODO
    }

    static error_code _is_action(game* self, bool* ret_is_action, move_code move)
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

    static error_code _eval(game* self, float* ret_eval, player_id player)
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

    static error_code _redact_keep_state(game* self, player_id* players, uint32_t count)
    {
        //TODO
    }

    static error_code _get_move_code(game* self, move_code* ret_move, const char* str)
    {
        //TODO
    }

    static error_code _get_move_str(game* self, size_t* ret_size, char* str_buf, move_code move)
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
    
    .get_error_string = surena::_get_error_string,
    .create = surena::_create,
    .destroy = surena::_destroy,
    .clone = surena::_clone,
    .copy_from = surena::_copy_from,
    .compare = surena::_compare,
    .import_state = surena::_import_state,
    .export_state = surena::_export_state,
    .get_player_count = surena::_get_player_count,
    .players_to_move = surena::_players_to_move,
    .get_concrete_moves = surena::_get_concrete_moves,
    .get_concrete_move_probabilities = surena::_get_concrete_move_probabilities,
    .get_concrete_moves_ordered = surena::_get_concrete_moves_ordered,
    .get_actions = surena::_get_actions,
    .is_legal_move = surena::_is_legal_move,
    .move_to_action = surena::_move_to_action,
    .is_action = surena::_is_action,
    .make_move = surena::_make_move,
    .get_results = surena::_get_results,
    .id = surena::_id,
    .eval = surena::_eval,
    .discretize = surena::_discretize,
    .playout = surena::_playout,
    .redact_keep_state = surena::_redact_keep_state,
    .get_move_code = surena::_get_move_code,
    .get_move_str = surena::_get_move_str,
    .debug_print = surena::_debug_print,
    
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