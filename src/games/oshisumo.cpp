#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/oshisumo.h"

namespace {
    
    // general purpose helpers for opts, data
    
    typedef oshisumo_options opts_repr;

    struct data_repr {
        opts_repr opts;

        player_id result_player;
        int8_t push_cell;
        uint8_t player_tokens[2];
        uint8_t sm_acc_buf[2];
    };

    opts_repr& get_opts(game* self)
    {
        return ((data_repr*)(self->data1))->opts;
    }

    data_repr& get_repr(game* self)
    {
        return *((data_repr*)(self->data1));
    }

    // forward declare everything to allow for inlining at least in this unit
    const char* get_last_error(game* self);
    error_code create_with_opts_str(game* self, const char* str);
    error_code create_with_opts_bin(game* self, void* options_struct);
    GF_UNUSED(create_deserialize);
    error_code create_default(game* self);
    error_code export_options_str(game* self, size_t* ret_size, char* str);
    GF_UNUSED(get_options_bin_ref);
    error_code destroy(game* self);
    error_code clone(game* self, game* clone_target);
    error_code copy_from(game* self, game* other);
    error_code compare(game* self, game* other, bool* ret_equal);
    error_code import_state(game* self, const char* str);
    error_code export_state(game* self, size_t* ret_size, char* str);
    GF_UNUSED(serialize);
    error_code players_to_move(game* self, uint8_t* ret_count, player_id* players);
    error_code get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    GF_UNUSED(get_concrete_move_probabilities);
    GF_UNUSED(get_concrete_moves_ordered);
    error_code get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    error_code is_legal_move(game* self, player_id player, move_code move, sync_counter sync);
    error_code move_to_action(game* self, move_code move, move_code* ret_action);
    error_code is_action(game* self, move_code move, bool* ret_is_action);
    error_code make_move(game* self, player_id player, move_code move);
    error_code get_results(game* self, uint8_t* ret_count, player_id* players);
    GF_UNUSED(get_sync_counter);
    error_code id(game* self, uint64_t* ret_id);
    error_code eval(game* self, player_id player, float* ret_eval);
    GF_UNUSED(discretize);
    error_code playout(game* self, uint64_t seed);
    error_code redact_keep_state(game* self, uint8_t count, player_id* players);
    error_code export_sync_data(game* self, sync_data** sync_data_start, sync_data** sync_data_end);
    error_code release_sync_data(game* self, sync_data* sync_data_start, sync_data* sync_data_end);
    error_code import_sync_data(game* self, void* data_start, void* data_end);
    error_code get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    error_code get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    error_code debug_print(game* self, size_t* ret_size, char* str_buf);

    error_code get_tokens(game* self, player_id p, uint8_t* t);
    error_code set_tokens(game* self, player_id p, uint8_t t);
    error_code get_cell(game* self, uint8_t* c);
    error_code set_cell(game* self, uint8_t c);
    error_code get_sm_tokens(game* self, player_id p, uint8_t* t);

    // implementation

    //TODO //BUG use new buffer sizer api

    const char* get_last_error(game* self)
    {
        return (char*)self->data2; // in this scheme opts are saved together with the state in data1, and data2 is the last error string
    }

    error_code create_with_opts_str(game* self, const char* str)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        self->data2 = NULL;

        opts_repr& opts = get_opts(self);
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

    error_code create_with_opts_bin(game* self, void* options_struct)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        self->data2 = NULL;

        opts_repr& opts = get_opts(self);
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

    error_code create_default(game* self)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        self->data2 = NULL;

        opts_repr& opts = get_opts(self);
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

    error_code export_options_str(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = get_opts(self);
        *ret_size = sprintf(str, "%hhu-%hhu", opts.size, opts.tokens);
        return ERR_OK;
    }

    error_code destroy(game* self)
    {
        free(self->data1);
        self->data1 = NULL;
        free(self->data2);
        self->data2 = NULL;
        return ERR_OK;
    }

    error_code clone(game* self, game* clone_target)
    {
        if (clone_target == NULL) {
            return ERR_INVALID_INPUT;
        }
        clone_target->methods = self->methods;
        opts_repr& opts = get_opts(self);
        error_code ec = clone_target->methods->create_with_opts_bin(clone_target, &opts);
        if (ec != ERR_OK) {
            return ec;
        }
        memcpy(clone_target->data1, self->data1, sizeof(data_repr));
        return ERR_OK;
    }
    
    error_code copy_from(game* self, game* other)
    {
        memcpy(self->data1, other->data1, sizeof(data_repr));
        return ERR_OK;
    }

    error_code compare(game* self, game* other, bool* ret_equal)
    {
        *ret_equal = (memcmp(self->data1, other->data1, sizeof(data_repr)) == 0);
        return ERR_OK;
    }

    error_code import_state(game* self, const char* str)
    {
        data_repr& data = get_repr(self);
        if (str == NULL) {
            opts_repr& opts = get_opts(self);
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

    error_code export_state(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
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

    error_code players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
        uint8_t pc = 0;
        while (pc < 2) {
            if (data.sm_acc_buf[pc++] == OSHISUMO_NONE) {
                continue;
            }
            players[pc - 1] = pc;
        }
        return ERR_OK;
    }

    error_code get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        data_repr& data = get_repr(self);
        for (int i = 0; i <= data.player_tokens[player - 1]; i++) {
            moves[i] = i;
        }
        *ret_count = data.player_tokens[player - 1] + 1;
        return ERR_OK;
    }

    error_code get_actions(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code is_legal_move(game* self, player_id player, move_code move, sync_counter sync)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code move_to_action(game* self, move_code move, move_code* ret_action)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code is_action(game* self, move_code move, bool* ret_is_action)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code make_move(game* self, player_id player, move_code move)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code id(game* self, uint64_t* ret_id)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code eval(game* self, player_id player, float* ret_eval)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code playout(game* self, uint64_t seed)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code redact_keep_state(game* self, uint8_t count, player_id* players)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code export_sync_data(game* self, sync_data** sync_data_start, sync_data** sync_data_end)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code release_sync_data(game* self, sync_data* sync_data_start, sync_data* sync_data_end)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code import_sync_data(game* self, void* data_start, void* data_end)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf)
    {
        //TODO
        return ERR_STATE_UNINITIALIZED;
    }

    error_code debug_print(game* self, size_t* ret_size, char* str_buf)
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
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
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

    error_code get_tokens(game* self, player_id p, uint8_t* t)
    {
        data_repr& data = get_repr(self);
        *t = data.player_tokens[p - 1];
        return ERR_OK;
    }

    error_code set_tokens(game* self, player_id p, uint8_t t)
    {
        data_repr& data = get_repr(self);
        data.player_tokens[p - 1] = t;
        return ERR_OK;
    }
    
    error_code get_cell(game* self, int8_t* c)
    {
        data_repr& data = get_repr(self);
        *c = data.push_cell;
        return ERR_OK;
    }
    
    error_code set_cell(game* self, int8_t c)
    {
        data_repr& data = get_repr(self);
        data.push_cell = c;
        return ERR_OK;
    }

    error_code get_sm_tokens(game* self, player_id p, uint8_t* t)
    {
        data_repr& data = get_repr(self);
        *t = data.sm_acc_buf[p - 1];
        return ERR_OK;
    }

}

static const oshisumo_internal_methods oshisumo_gbe_internal_methods{
    .get_tokens = get_tokens,
    .set_tokens = set_tokens,
    .get_cell = get_cell,
    .set_cell = set_cell,
    .get_sm_tokens = get_sm_tokens,
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
        .serializable = false,
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
