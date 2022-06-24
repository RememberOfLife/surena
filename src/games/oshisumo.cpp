#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/oshisumo.h"

namespace surena {
    
    // general purpose helpers for opts, data, errors

    static error_code _rerrorf(game* self, error_code ec, const char* fmt, ...)
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
    
    typedef oshisumo_options opts_repr;

    typedef struct data_repr {
        opts_repr opts;

        player_id result_player;
        int8_t push_cell;
        uint8_t player_tokens[2];
        uint8_t sm_acc_buf[2];
    } data_repr;

    static opts_repr& _get_opts(game* self)
    {
        return ((data_repr*)(self->data1))->opts;
    }

    static data_repr& _get_repr(game* self)
    {
        return *((data_repr*)(self->data1));
    }

    // forward declare everything to allow for inlining at least in this unit
    static const char* _get_last_error(game* self);
    static error_code _create_with_opts_str(game* self, const char* str);
    static error_code _create_with_opts_bin(game* self, void* options_struct);
    static error_code _create_default(game* self);
    static error_code _export_options_str(game* self, size_t* ret_size, char* str);
    GF_UNUSED(get_options_bin_ref);
    static error_code _destroy(game* self);
    static error_code _clone(game* self, game* clone_target);
    static error_code _copy_from(game* self, game* other);
    static error_code _compare(game* self, game* other, bool* ret_equal);
    static error_code _import_state(game* self, const char* str);
    static error_code _export_state(game* self, size_t* ret_size, char* str);
    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players);
    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    GF_UNUSED(get_concrete_move_probabilities);
    GF_UNUSED(get_concrete_moves_ordered);
    static error_code _get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    static error_code _is_legal_move(game* self, player_id player, move_code move, sync_counter sync);
    static error_code _move_to_action(game* self, move_code move, move_code* ret_action);
    static error_code _is_action(game* self, move_code move, bool* ret_is_action);
    static error_code _make_move(game* self, player_id player, move_code move);
    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players);
    GF_UNUSED(get_sync_counter);
    static error_code _id(game* self, uint64_t* ret_id);
    static error_code _eval(game* self, player_id player, float* ret_eval);
    GF_UNUSED(discretize);
    static error_code _playout(game* self, uint64_t seed);
    static error_code _redact_keep_state(game* self, uint8_t count, player_id* players);
    static error_code _export_sync_data(game* self, sync_data** sync_data_start, sync_data** sync_data_end);
    static error_code _release_sync_data(game* self, sync_data* sync_data_start, sync_data* sync_data_end);
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

    //TODO //BUG use new buffer sizer api

    static const char* _get_last_error(game* self)
    {
        return (char*)self->data2; // in this scheme opts are saved together with the state in data1, and data2 is the last error string
    }

    static error_code _create_with_opts_str(game* self, const char* str)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        opts_repr& opts = _get_opts(self);
        opts.size = 5;
        opts.tokens = 50;
        if (str != NULL) {
            int ec = sscanf(str, "%hhu-%hhu", &opts.size, &opts.tokens);
            if (ec != 2) {
                free(self->data1);
                return ERR_INVALID_INPUT;
            }
        }

        self->sizer = (buf_sizer){
            .options_str = 8,
            .state_str = 14,
            .player_count = 2,
            .max_players_to_move = 2,
            .max_moves = (uint32_t)(opts.tokens + 1),
            .max_results = 1,
            .move_str = 10, //TODO size correctly
            .print_str = 100, //TODO size correctly
        };
        return ERR_OK;
    }

    static error_code _create_with_opts_bin(game* self, void* options_struct)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        opts_repr& opts = _get_opts(self);
        opts.size = 5;
        opts.tokens = 50;
        if (options_struct != NULL) {
            opts = *(opts_repr*)options_struct;
        }

        self->sizer = (buf_sizer){
            .options_str = 8,
            .state_str = 14,
            .player_count = 2,
            .max_players_to_move = 2,
            .max_moves = (uint32_t)(opts.tokens + 1),
            .max_results = 1,
            .move_str = 10, //TODO size correctly
            .print_str = 100, //TODO size correctly
        };
        return ERR_OK;
    }

    static error_code _create_default(game* self)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        opts_repr& opts = _get_opts(self);
        opts.size = 5;
        opts.tokens = 50;

        self->sizer = (buf_sizer){
            .options_str = 8,
            .state_str = 14,
            .player_count = 2,
            .max_players_to_move = 2,
            .max_moves = (uint32_t)(opts.tokens + 1),
            .max_results = 1,
            .move_str = 10, //TODO size correctly
            .print_str = 100, //TODO size correctly
        };
        return ERR_OK;
    }

    static error_code _export_options_str(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        *ret_size = sprintf(str, "%hhu-%hhu", opts.size, opts.tokens);
        return ERR_OK;
    }

    static error_code _destroy(game* self)
    {
        free(self->data1);
        self->data1 = NULL;
        return ERR_OK;
    }

    static error_code _clone(game* self, game* clone_target)
    {
        if (clone_target == NULL) {
            return ERR_INVALID_INPUT;
        }
        clone_target->methods = self->methods;
        opts_repr& opts = _get_opts(self);
        error_code ec = clone_target->methods->create_with_opts_bin(clone_target, &opts);
        if (ec != ERR_OK) {
            return ec;
        }
        memcpy(clone_target->data1, self->data1, sizeof(data_repr));
        return ERR_OK;
    }
    
    static error_code _copy_from(game* self, game* other)
    {
        memcpy(self->data1, other->data1, sizeof(data_repr));
        return ERR_OK;
    }

    static error_code _compare(game* self, game* other, bool* ret_equal)
    {
        *ret_equal = (memcmp(self->data1, other->data1, sizeof(data_repr)) == 0);
        return ERR_OK;
    }

    static error_code _import_state(game* self, const char* str)
    {
        data_repr& data = _get_repr(self);
        if (str == NULL) {
            opts_repr& opts = _get_opts(self);
            data = data_repr{
                .result_player = PLAYER_NONE,
                .push_cell = 0,
                .player_tokens = {opts.tokens, opts.tokens},
                .sm_acc_buf = {OSHISUMO_NONE, OSHISUMO_NONE},
            };
            return ERR_OK;
        }
        int ec = sscanf(str, "%hhu>%hhd<%hhu %hhu", &data.player_tokens[0], &data.push_cell, &data.player_tokens[1], &data.result_player);
        if (ec < 3) {
            return ERR_INVALID_INPUT;
        }
        if (ec < 4) {
            data.result_player = PLAYER_NONE;
        }
        return ERR_OK;
    }

    static error_code _export_state(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = _get_repr(self);
        char* ostr = str;
        str += sprintf(str, "%hhu>%hhd<%hhu ", data.player_tokens[0], data.push_cell, data.player_tokens[1]);
        if (data.result_player > PLAYER_NONE) {
            str += sprintf(str, "%hhu", data.result_player);
        } else {
            str += sprintf(str, "-");
        }
        *ret_size = ostr - str;
        return ERR_OK;
    }

    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = _get_repr(self);
        uint8_t pc = 0;
        while (pc < 2) {
            if (data.sm_acc_buf[pc++] == OSHISUMO_NONE) {
                continue;
            }
            players[pc - 1] = pc;
        }
        return ERR_OK;
    }

    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        data_repr& data = _get_repr(self);
        for (int i = 0; i <= data.player_tokens[player - 1]; i++) {
            moves[i] = i;
        }
        *ret_count = data.player_tokens[player - 1] + 1;
        return ERR_OK;
    }

    static error_code _get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    static error_code _is_legal_move(game* self, player_id player, move_code move, sync_counter sync)
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

    static error_code _release_sync_data(game* self, sync_data* sync_data_start, sync_data* sync_data_end)
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
        /* pad the board and inner spacing on the tokens line, '^' just shows the center
        size 5
        (5)
        -| | |X| | |-
        45    ^    33
        */
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        char* ostr = str_buf;
        char acc_str_buf[6][2];
        for (int i = 0; i < 2; i++) {
            if (data.sm_acc_buf[i] == OSHISUMO_ANY) {
                    sprintf(acc_str_buf[i], "(#)");
            } else if (data.sm_acc_buf[i] != OSHISUMO_NONE) {
                sprintf(acc_str_buf[i], "(%hhu)", data.sm_acc_buf[i]);
            }
        }
        if (data.sm_acc_buf[0] != OSHISUMO_NONE) {
            str_buf += sprintf(str_buf, "%s", acc_str_buf[0]);
        }
        if (data.sm_acc_buf[1] != OSHISUMO_NONE) {
            if (data.sm_acc_buf[0] != OSHISUMO_NONE) {
                str_buf += sprintf(str_buf, " ");
            }
            str_buf += sprintf(str_buf, "%s", acc_str_buf[1]);
        }
        if (data.sm_acc_buf[0] != OSHISUMO_NONE || data.sm_acc_buf[1] != OSHISUMO_NONE) {
            str_buf += sprintf(str_buf, "\n");
        }
        //TODO padding
        if (opts.size % 2 == 0) {
            for (int i = -1; i <= opts.size; i++) {
                bool mid_skip = false;
                if (data.push_cell + opts.size / 2 == i && i == opts.size / 2) {
                    mid_skip = true;
                    str_buf += sprintf(str_buf, "!");
                } else if (i >= 0) {
                    str_buf += sprintf(str_buf, "|");
                }
                if (data.push_cell + (opts.size - ((data.push_cell > 0) ? 1 : 0)) / 2 == i && !mid_skip) {
                    str_buf += sprintf(str_buf, "X");
                } else {
                    if (i == -1 || i == opts.size) {
                        str_buf += sprintf(str_buf, "-");
                    } else {
                        str_buf += sprintf(str_buf, " ");
                    }
                }
            }
        } else {
            for (int i = -1; i <= opts.size; i++) {
                if (data.push_cell + opts.size / 2 == i) {
                    str_buf += sprintf(str_buf, "X");
                } else {
                    if (i == -1 || i == opts.size) {
                        str_buf += sprintf(str_buf, "-");
                    } else {
                        str_buf += sprintf(str_buf, " ");
                    }
                }
                if (i < opts.size) {
                    str_buf += sprintf(str_buf, "|");
                }
            }
        }
        str_buf += sprintf(str_buf, "\n%hhu - %hhu\n", data.player_tokens[0], data.player_tokens[1]);
        return ERR_OK;
    }

    //=====
    // game internal methods

    static error_code _get_tokens(game* self, player_id p, uint8_t* t)
    {
        data_repr& data = _get_repr(self);
        *t = data.player_tokens[p - 1];
        return ERR_OK;
    }

    static error_code _set_tokens(game* self, player_id p, uint8_t t)
    {
        data_repr& data = _get_repr(self);
        data.player_tokens[p - 1] = t;
        return ERR_OK;
    }
    
    static error_code _get_cell(game* self, int8_t* c)
    {
        data_repr& data = _get_repr(self);
        *c = data.push_cell;
        return ERR_OK;
    }
    
    static error_code _set_cell(game* self, int8_t c)
    {
        data_repr& data = _get_repr(self);
        data.push_cell = c;
        return ERR_OK;
    }

    static error_code _get_sm_tokens(game* self, player_id p, uint8_t* t)
    {
        data_repr& data = _get_repr(self);
        *t = data.sm_acc_buf[p - 1];
        return ERR_OK;
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
        .options = true,
        .options_bin = true,
        .options_bin_ref = false,
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = true,
        .sync_counter = false,
        .move_ordering = false,
        .id = true,
        .eval = true,
        .playout = true,
        .print = true,
    },
    .internal_methods = (void*)&oshisumo_gbe_internal_methods,
    
    #include "surena/game_impl.h"
    
};
