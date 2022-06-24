#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "surena/util/noise.hpp"
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/twixt_pp.h"

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
    
    typedef twixt_pp_options opts_repr;

    typedef struct data_repr {
        opts_repr opts;

        TWIXT_PP_PLAYER current_player;
        TWIXT_PP_PLAYER winning_player;
        uint16_t remaining_inner_nodes;
        std::unordered_map<uint16_t, twixt_pp_graph> graph_map;
        uint16_t next_graph_id;
        std::vector<std::vector<twixt_pp_node>> gameboard; // white plays vertical and black horizontal per default, gameboard[iy][ix]
        bool pie_swap;
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
    static error_code _get_options_bin_ref(game* self, void** ret_bin_ref);
    static error_code _destroy(game* self);
    static error_code _clone(game* self, game* clone_target);
    static error_code _copy_from(game* self, game* other);
    static error_code _compare(game* self, game* other, bool* ret_equal);
    static error_code _import_state(game* self, const char* str);
    static error_code _export_state(game* self, size_t* ret_size, char* str);
    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players);
    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    GF_UNUSED(get_concrete_move_probabilities);
    GF_UNUSED(get_concrete_moves_ordered); //TODO
    GF_UNUSED(get_actions);
    static error_code _is_legal_move(game* self, player_id player, move_code move, sync_counter sync);
    GF_UNUSED(move_to_action);
    GF_UNUSED(is_action);
    static error_code _make_move(game* self, player_id player, move_code move);
    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players);
    GF_UNUSED(get_sync_counter);
    GF_UNUSED(id); //TODO
    GF_UNUSED(eval); //TODO
    GF_UNUSED(discretize);
    static error_code _playout(game* self, uint64_t seed);
    GF_UNUSED(redact_keep_state);
    GF_UNUSED(export_sync_data);
    GF_UNUSED(release_sync_data);
    GF_UNUSED(import_sync_data);
    static error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    static error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf);

    /* same for internals */
    static error_code _get_node(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER* p);
    static error_code _set_node(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER p, bool* wins);
    static error_code _get_node_connections(game* self, uint8_t x, uint8_t y, uint8_t* conn);

    // implementation

    static const char* _get_last_error(game* self)
    {
        return (char*)self->data2; // in this scheme opts are saved together with the state in data1, and data2 is the last error string
    }

    static error_code _create_with_opts_str(game* self, const char* str)
    {
        self->data1 = new(malloc(sizeof(data_repr))) data_repr();
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        
        opts_repr& opts = _get_opts(self);
        opts.wx = 24;
        opts.wy = 24;
        opts.pie_swap = true;
        if (str != NULL) {
            // this accepts formats in:
            // "white/black+" and "square+"
            // the '+' is optional and indicates swap rule being enabled (a "plus" for black)
            uint8_t square_size = 0;
            char square_swap = '\0';
            int ec_square = sscanf(str, "%hhu%c", &square_size, &square_swap);
            char double_swap = '\0';
            int ec_double = sscanf(str, "%hhu/%hhu%c", &opts.wy, &opts.wx, &double_swap);
            //TODO check both sizes for within min/max
            if (ec_double >= 2) {
                opts.pie_swap = (double_swap == '+');
                if (opts.pie_swap == false && double_swap != '\0') {
                    free(self->data1);
                    self->data1 = NULL;
                    return ERR_INVALID_INPUT;
                }
            } else if (ec_square >= 1) {
                opts.wx = square_size;
                opts.wy = square_size;
                opts.pie_swap = (square_swap == '+');
                if (opts.pie_swap == false && square_swap != '\0') {
                    free(self->data1);
                    self->data1 = NULL;
                    return ERR_INVALID_INPUT;
                }
            } else {
                free(self->data1);
                self->data1 = NULL;
                return ERR_INVALID_INPUT;
            }
        }

        self->sizer = (buf_sizer){
            .options_str = 9,
            .state_str = (size_t)(1024), //TODO
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = (uint32_t)(opts.wx * opts.wy + (opts.wx > opts.wy ? opts.wx : opts.wy) * 2 - 4),
            .max_results = 1,
            .move_str = 6,
            .print_str = (size_t)(opts.wx * opts.wy + opts.wy + 1),
        };
        return ERR_OK;
    }

    static error_code _create_with_opts_bin(game* self, void* options_struct)
    {
        self->data1 = new(malloc(sizeof(data_repr))) data_repr();
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        
        opts_repr& opts = _get_opts(self);
        opts.wx = 24;
        opts.wy = 24;
        opts.pie_swap = true;
        if (options_struct != NULL) {
            opts = *(opts_repr*)options_struct;
        }

        self->sizer = (buf_sizer){
            .options_str = 9,
            .state_str = (size_t)(1024), //TODO
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = (uint32_t)(opts.wx * opts.wy + (opts.wx > opts.wy ? opts.wx : opts.wy) * 2 - 4),
            .max_results = 1,
            .move_str = 6,
            .print_str = (size_t)(opts.wx * opts.wy + opts.wy + 1),
        };
        return ERR_OK;
    }

    static error_code _create_default(game* self)
    {
        self->data1 = new(malloc(sizeof(data_repr))) data_repr();
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        opts_repr& opts = _get_opts(self);
        opts.wx = 24;
        opts.wy = 24;
        opts.pie_swap = true;

        self->sizer = (buf_sizer){
            .options_str = 9,
            .state_str = (size_t)(1024), //TODO
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = (uint32_t)(opts.wx * opts.wy + (opts.wx > opts.wy ? opts.wx : opts.wy) * 2 - 4),
            .max_results = 1,
            .move_str = 6,
            .print_str = (size_t)(opts.wx * opts.wy + opts.wy + 1),
        };
        return ERR_OK;
    }

    static error_code _export_options_str(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        if (opts.wx == opts.wy) {
            *ret_size = sprintf(str, "%hhu%c", opts.wx, opts.pie_swap ? '+' : '\0');
        } else {
            *ret_size = sprintf(str, "%hhu/%hhu%c", opts.wy, opts.wx, opts.pie_swap ? '+' : '\0');
        }
        return ERR_OK;
    }

    static error_code _get_options_bin_ref(game* self, void** ret_bin_ref)
    {
        *(opts_repr*)ret_bin_ref = _get_opts(self);
        return ERR_OK;
    }

    static error_code _destroy(game* self)
    {
        delete (data_repr*)self->data1;
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
        *(data_repr*)clone_target->data1 = *(data_repr*)self->data1;
        return ERR_OK;
    }
    
    static error_code _copy_from(game* self, game* other)
    {
        *(data_repr*)self->data1 = *(data_repr*)other->data1;
        return ERR_OK;
    }

    static error_code _compare(game* self, game* other, bool* ret_equal)
    {
        //TODO
        return ERR_STATE_CORRUPTED;
    }

    static error_code _import_state(game* self, const char* str)
    {
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        data.current_player = TWIXT_PP_PLAYER_WHITE;
        data.winning_player = TWIXT_PP_PLAYER_INVALID;
        data.remaining_inner_nodes = opts.wx * opts.wy;
        data.graph_map.clear();
        data.next_graph_id = 1;
        data.gameboard.clear();
        data.gameboard.reserve(opts.wy);
        for (int iy = 0; iy < opts.wy; iy++) {
            std::vector<twixt_pp_node> tile_row_vector{};
            tile_row_vector.resize(opts.wx, twixt_pp_node{TWIXT_PP_PLAYER_NONE, 0, 0 ,0});
            data.gameboard.push_back(std::move(tile_row_vector));
        }
        data.gameboard[0][0].player = TWIXT_PP_PLAYER_INVALID;
        data.gameboard[0][opts.wx - 1].player = TWIXT_PP_PLAYER_INVALID;
        data.gameboard[opts.wy - 1][0].player = TWIXT_PP_PLAYER_INVALID;
        data.gameboard[opts.wy - 1][opts.wx - 1].player = TWIXT_PP_PLAYER_INVALID;
        data.pie_swap = opts.pie_swap;
        if (str == NULL) {
            return ERR_OK;
        }
        //TODO
        return ERR_STATE_UNRECOVERABLE;
    }

    static error_code _export_state(game* self, size_t* ret_size, char* str)
    {
        //TODO
        return ERR_STATE_UNRECOVERABLE;
    }

    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = _get_repr(self);
        if (data.current_player == PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = data.current_player;
        return ERR_OK;
    }

    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        if (moves == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        uint32_t move_cnt = 0;
        for (int iy = 0; iy < opts.wy; iy++) {
            for (int ix = 0; ix < opts.wx; ix++) {
                if (data.gameboard[iy][ix].player == TWIXT_PP_PLAYER_NONE) {
                    // add the free tile to the return vector
                    moves[move_cnt++] = (ix << 8) | iy;
                }
            }
        }
        *ret_count = move_cnt;
        return ERR_OK;
    }

    static error_code _is_legal_move(game* self, player_id player, move_code move, sync_counter sync)
    {
        if (move == MOVE_NONE) {
            return ERR_INVALID_INPUT;
        }
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        if (ptm != player) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = _get_repr(self);
        int ix = (move >> 8) & 0xFF;
        int iy = move & 0xFF;
        if (data.gameboard[iy][ix].player != TWIXT_PP_PLAYER_NONE) {
            return ERR_INVALID_INPUT;
        }
        return ERR_OK;
    }

    static error_code _make_move(game* self, player_id player, move_code move)
    {
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        
        int tx = (move >> 8) & 0xFF;
        int ty = move & 0xFF;

        bool wins;
        // set node updates graph structures in the backend, and informs us if this move is winning for the current player
        _set_node(self, tx, ty, data.current_player, &wins);

        if (tx > 0 && tx < opts.wx - 1 && ty > 0 && ty < opts.wy) {
            data.remaining_inner_nodes--;
        }

        if (wins) {
            data.winning_player = data.current_player;
            data.current_player = TWIXT_PP_PLAYER_NONE;
        } else if (data.remaining_inner_nodes == 0) {
            data.winning_player = TWIXT_PP_PLAYER_NONE;
            data.current_player = TWIXT_PP_PLAYER_NONE;
        } else {
            // only really switch colors if the game is still going
            data.current_player = data.current_player == TWIXT_PP_PLAYER_WHITE ? TWIXT_PP_PLAYER_BLACK : TWIXT_PP_PLAYER_WHITE;
        }

        return ERR_OK;
    }

    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = _get_repr(self);
        if (data.current_player != TWIXT_PP_PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = data.winning_player;
        return ERR_OK;
    }

    static error_code _playout(game* self, uint64_t seed)
    {
        uint32_t ctr = 0;
        move_code* moves;
        uint32_t moves_count;
        _get_concrete_moves(self, TWIXT_PP_PLAYER_NONE, &moves_count, moves);
        moves = (move_code*)malloc(moves_count * sizeof(move_code));
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        while (ptm_count > 0) {
            _get_concrete_moves(self, ptm, &moves_count, moves);
            _make_move(self, ptm ,moves[squirrelnoise5(ctr++, seed)%moves_count]);
            _players_to_move(self, &ptm_count, &ptm);
        }
        free(moves);
        return ERR_OK;
    }

    static error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        //TODO move as xyz123, xyz is a=0 A=26 and 123 %hhu
        return ERR_STATE_UNRECOVERABLE;
    }

    static error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf)
    {
        //TODO
        return ERR_STATE_UNRECOVERABLE;
    }

    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        const char* ostr = str_buf;
        for (int iy = 0; iy < opts.wy; iy++) {
            for (int ix = 0; ix < opts.wy; ix++) {
                TWIXT_PP_PLAYER node_player = TWIXT_PP_PLAYER_INVALID;
                _get_node(self, ix, iy, &node_player);
                switch (node_player) {
                    case TWIXT_PP_PLAYER_INVALID: {
                        str_buf += sprintf(str_buf, " ");
                    } break;
                    case TWIXT_PP_PLAYER_WHITE: {
                        str_buf += sprintf(str_buf, "O");
                    } break;
                    case TWIXT_PP_PLAYER_BLACK: {
                        str_buf += sprintf(str_buf, "X");
                    } break;
                    case TWIXT_PP_PLAYER_NONE: {
                        str_buf += sprintf(str_buf, ".");
                    } break;
                }
            }
            str_buf += sprintf(str_buf, "\n");
        }
        *ret_size = str_buf-ostr;
        return ERR_OK;
    }

    //=====
    // game internal methods

    static error_code _get_node(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER* p)
    {
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        if (x < 0 || y < 0 || x >= opts.wx || y >= opts.wy) {
            *p = TWIXT_PP_PLAYER_INVALID;
        } else {
            *p = data.gameboard[y][x].player;
        }
        return ERR_OK;
    }

    static error_code _set_node(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER p, bool* wins)
    {
        //TODO
        return ERR_STATE_UNRECOVERABLE;
    }

    static error_code _get_node_connections(game* self, uint8_t x, uint8_t y, uint8_t* conn)
    {
        data_repr& data = _get_repr(self);
        *conn = data.gameboard[y][x].connections;
        return ERR_OK;
    }

}

static const twixt_pp_internal_methods twixt_pp_gbe_internal_methods{
    .get_node = surena::_get_node,
    .set_node = surena::_set_node,
    .get_node_connections = surena::_get_node_connections,
};

const game_methods twixt_pp_gbe{

    .game_name = "TwixT",
    .variant_name = "PP",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .options = true,
        .options_bin = true,
        .options_bin_ref = true,
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = false,
        .sync_counter = false,
        .move_ordering = false,
        .id = false,
        .eval = false,
        .playout = true,
        .print = true,
    },
    .internal_methods = (void*)&twixt_pp_gbe_internal_methods,
    
    #include "surena/game_impl.h"
    
};
