#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/oshisumo.hpp"

namespace surena {
    
    // game data state representation and general getter
    
    typedef oshisumo_options opts_repr;
    opts_repr& _get_opts(game* self)
    {
        return *((opts_repr*)(self->options));
    }

    typedef struct data_repr {
        uint8_t push_cell;
        uint8_t player_tokens[2];
        uint8_t sm_acc_buf[2];
    } data_repr;

    data_repr& _get_repr(game* self)
    {
        return *((data_repr*)(self->data));
    }

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
    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    GF_UNUSED(get_concrete_move_probabilities);
    GF_UNUSED(get_concrete_moves_ordered);
    static error_code _get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    static error_code _is_legal_move(game* self, player_id player, move_code move, uint32_t sync_ctr);
    static error_code _move_to_action(game* self, move_code move, move_code* ret_action);
    static error_code _is_action(game* self, move_code move, bool* ret_is_action);
    static error_code _make_move(game* self, player_id player, move_code move);
    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players);
    static error_code _id(game* self, uint64_t* ret_id);
    static error_code _eval(game* self, player_id player, float* ret_eval);
    GF_UNUSED(discretize);
    static error_code _playout(game* self, uint64_t seed);
    static error_code _redact_keep_state(game* self, uint8_t count, player_id* players);
    static error_code _export_sync_data(game* self, sync_data** sync_data_start, sync_data** sync_data_end);
    static error_code _import_sync_data(game* self, void* data_start, void* data_end);
    static error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    static error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf);

    static error_code _get_tokens(game* self, player_id p, uint8_t* t);
    static error_code _set_tokens(game* self, player_id p, uint8_t t);
    static error_code _get_cell(game* self, uint8_t* c);
    static error_code _set_cell(game* self, uint8_t c);
    static error_code _get_sm_tokens(game* self, player_id p, uint8_t* t);

    // implementation

    static const char* _get_error_string(error_code err)
    {
        const char* gen_err_str = get_general_error_string(err);
        if (gen_err_str != NULL) {
            return gen_err_str;
        }
        return "unknown error";
    }

    static error_code _create(game* self)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _destroy(game* self)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _clone(game* self, game** ret_clone)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }
    
    static error_code _copy_from(game* self, game* other)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _compare(game* self, game* other, bool* ret_equal)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _import_state(game* self, const char* str)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _export_state(game* self, size_t* ret_size, char* str)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _get_player_count(game* self, uint8_t* ret_count)
    {
        *ret_count = 2;
        return ERR_OK;
    }

    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _is_legal_move(game* self, player_id player, move_code move, uint32_t sync_ctr)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _move_to_action(game* self, move_code move, move_code* ret_action)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _is_action(game* self, move_code move, bool* ret_is_action)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _make_move(game* self, player_id player, move_code move)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _id(game* self, uint64_t* ret_id)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _eval(game* self, player_id player, float* ret_eval)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _playout(game* self, uint64_t seed)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _redact_keep_state(game* self, uint8_t count, player_id* players)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _export_sync_data(game* self, sync_data** sync_data_start, sync_data** sync_data_end)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _import_sync_data(game* self, void* data_start, void* data_end)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    //=====
    // game internal methods

    static error_code _get_tokens(game* self, player_id p, uint8_t* t)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _set_tokens(game* self, player_id p, uint8_t t)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }
    
    static error_code _get_cell(game* self, uint8_t* c)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }
    
    static error_code _set_cell(game* self, uint8_t c)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _get_sm_tokens(game* self, player_id p, uint8_t* t)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

}

static const oshisumo_internal_methods oshisumo_gbe_internal_methods{
    .get_tokens = surena::_get_tokens,
    .set_tokens = surena::_set_tokens,
    .get_cell = surena::_get_cell,
    .set_cell = surena::_set_cell,
    .get_sm_tokens = surena::_get_sm_tokens,
};

const game_methods oshisumo_gbe{

    .game_name = "Oshisumo",
    .variant_name = "Standard",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = true,
    },
    .internal_methods = (void*)&oshisumo_gbe_internal_methods,
    
    #include "surena/game_impl.h"
    
};
