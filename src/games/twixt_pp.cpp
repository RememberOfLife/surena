#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "surena/util/noise.h"
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/twixt_pp.h"

namespace {

    // general purpose helpers for opts, data

    typedef twixt_pp_options opts_repr;

    struct data_repr {
        opts_repr opts;

        TWIXT_PP_PLAYER current_player;
        TWIXT_PP_PLAYER winning_player;
        uint16_t remaining_inner_nodes;
        std::unordered_map<uint16_t, twixt_pp_graph> graph_map;
        uint16_t next_graph_id;
        std::vector<std::vector<twixt_pp_node>> gameboard; // white plays vertical and black horizontal per default, gameboard[iy][ix]
        bool pie_swap; // if this is true while it is blacks turn, they swap move to mirror it as theirs, then set false
        uint16_t swap_target;
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
    GF_UNUSED(get_last_error);
    error_code create(game* self, game_init init_info);
    error_code export_options_str(game* self, size_t* ret_size, char* str);
    error_code get_options_bin_ref(game* self, void** ret_bin_ref);
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
    GF_UNUSED(get_concrete_moves_ordered); //TODO
    GF_UNUSED(get_actions);
    error_code is_legal_move(game* self, player_id player, move_code move, sync_counter sync);
    GF_UNUSED(move_to_action);
    GF_UNUSED(is_action);
    error_code make_move(game* self, player_id player, move_code move);
    error_code get_results(game* self, uint8_t* ret_count, player_id* players);
    GF_UNUSED(export_legacy);
    GF_UNUSED(get_sync_counter);
    GF_UNUSED(get_scores);
    GF_UNUSED(id); //TODO
    GF_UNUSED(eval); //TODO
    GF_UNUSED(discretize);
    error_code playout(game* self, uint64_t seed);
    GF_UNUSED(redact_keep_state);
    GF_UNUSED(export_sync_data);
    GF_UNUSED(release_sync_data);
    GF_UNUSED(import_sync_data);
    error_code get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    error_code get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    error_code print(game* self, size_t* ret_size, char* str_buf);

    /* same for internals */
    error_code get_node(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER* p);
    error_code set_node(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER p, uint8_t connection_mask, bool* wins);
    error_code get_node_connections(game* self, uint8_t x, uint8_t y, uint8_t* conn);
    error_code get_node_collisions(game* self, uint8_t x, uint8_t y, uint8_t* collisions);
    error_code set_connection(game* self, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool* wins);
    error_code can_swap(game* self, bool* swap_available);

    // implementation

    error_code create(game* self, game_init init_info)
    {
        self->data1 = new (malloc(sizeof(data_repr))) data_repr();
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        self->data2 = NULL;

        opts_repr& opts = get_opts(self);
        opts.wx = 24;
        opts.wy = 24;
        opts.pie_swap = true;
        GAME_INIT_OPTS_TYPE options_type = (init_info.source_type == GAME_INIT_SOURCE_TYPE_STANDARD ? init_info.source.standard.opts_type : GAME_INIT_OPTS_TYPE_DEFAULT);
        switch (options_type) {
            case GAME_INIT_OPTS_TYPE_DEFAULT: {
                // pass
            } break;
            case GAME_INIT_OPTS_TYPE_STR: {
                if (init_info.source.standard.opts.str == NULL) {
                    break;
                }
                // this accepts formats in:
                // "white/black+" and "square+"
                // the '+' is optional and indicates swap rule being enabled (a "plus" for black)
                uint8_t square_size = 0;
                char square_swap = '\0';
                int ec_square = sscanf(init_info.source.standard.opts.str, "%hhu%c", &square_size, &square_swap);
                char double_swap = '\0';
                int ec_double = sscanf(init_info.source.standard.opts.str, "%hhu/%hhu%c", &opts.wy, &opts.wx, &double_swap);
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
            } break;
            case GAME_INIT_OPTS_TYPE_BIN: {
                if (init_info.source.standard.opts.bin == NULL) {
                    break;
                }
                opts = *(opts_repr*)init_info.source.standard.opts.bin;
            } break;
        }
        if (opts.wx < 5 || opts.wx > 128 || opts.wy < 5 || opts.wy > 128) {
            free(self->data1);
            self->data1 = NULL;
            return ERR_INVALID_INPUT;
        }

        self->sizer = (buf_sizer){
            .options_str = 9,
            .state_str = (size_t)(opts.wy * opts.wx * 5 + 1 + 4),
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = (uint32_t)(opts.wx * opts.wy - 3),
            .max_results = 1,
            .move_str = 6,
            .print_str = (size_t)(opts.wx * opts.wy + opts.wy + 1),
        };
        const char* initial_state = NULL;
        if (init_info.source_type == GAME_INIT_SOURCE_TYPE_STANDARD) {
            initial_state = init_info.source.standard.initial_state;
        }
        return import_state(self, initial_state);
    }

    error_code export_options_str(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = get_opts(self);
        if (opts.wx == opts.wy) {
            *ret_size = sprintf(str, "%hhu%c", opts.wx, opts.pie_swap ? '+' : '\0');
        } else {
            *ret_size = sprintf(str, "%hhu/%hhu%c", opts.wy, opts.wx, opts.pie_swap ? '+' : '\0');
        }
        return ERR_OK;
    }

    error_code get_options_bin_ref(game* self, void** ret_bin_ref)
    {
        *(opts_repr**)ret_bin_ref = &get_opts(self);
        return ERR_OK;
    }

    error_code destroy(game* self)
    {
        delete (data_repr*)self->data1;
        self->data1 = NULL;
        return ERR_OK;
    }

    error_code clone(game* self, game* clone_target)
    {
        if (clone_target == NULL) {
            return ERR_INVALID_INPUT;
        }
        clone_target->methods = self->methods;
        opts_repr& opts = get_opts(self);
        error_code ec = clone_target->methods->create(
            clone_target,
            (game_init){
                .source_type = GAME_INIT_SOURCE_TYPE_STANDARD,
                .source = {
                    .standard = {
                        .opts_type = GAME_INIT_OPTS_TYPE_BIN,
                        .opts = {
                            .bin = &opts,
                        },
                        .legacy_str = NULL,
                        .initial_state = NULL,
                    },
                },
            }
        );
        if (ec != ERR_OK) {
            return ec;
        }
        *(data_repr*)clone_target->data1 = *(data_repr*)self->data1;
        return ERR_OK;
    }

    error_code copy_from(game* self, game* other)
    {
        *(data_repr*)self->data1 = *(data_repr*)other->data1;
        return ERR_OK;
    }

    error_code compare(game* self, game* other, bool* ret_equal)
    {
        //TODO
        return ERR_STATE_CORRUPTED;
    }

    error_code import_state(game* self, const char* str)
    {
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        data.current_player = TWIXT_PP_PLAYER_WHITE;
        data.winning_player = TWIXT_PP_PLAYER_INVALID;
        data.remaining_inner_nodes = opts.wx * opts.wy;
        data.graph_map.clear();
        data.next_graph_id = 1;
        data.gameboard.clear();
        data.gameboard.reserve(opts.wy);
        for (int iy = 0; iy < opts.wy; iy++) {
            std::vector<twixt_pp_node> tile_row_vector{};
            tile_row_vector.resize(opts.wx, twixt_pp_node{TWIXT_PP_PLAYER_NONE, 0, 0, 0});
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
        // load from diy twixt format, "board p_cur p_res"
        // board horizontal lines, numbers to denote blank spaces, '/' for next line, top and bottom rows are 2 nodes short
        int y = 0;
        int x = 1;
        int x_moves = 0;
        int o_moves = 0;
        std::vector<std::vector<uint8_t>> conn_masks;
        conn_masks.reserve(opts.wy);
        for (int iy = 0; iy < opts.wy; iy++) {
            std::vector<uint8_t> conn_mask_row_vector{};
            conn_mask_row_vector.resize(opts.wx, 0);
            conn_masks.push_back(std::move(conn_mask_row_vector));
        }
        // get cell fillings
        bool advance_segment = false;
        while (!advance_segment) {
            switch (*str) {
                case 'O': {
                    if (x < 0 || x >= opts.wx || y < 0 || y > opts.wy || ((y == 0 || y == opts.wy - 1) && x == opts.wx - 1)) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    // get realized connections mask, if any
                    uint8_t conn_mask = 0x0F;
                    if (*(str + 1) == '.' || *(str + 1) == ':') {
                        conn_mask = 0;
                        char conn_pattern[4];
                        int ec = sscanf(str + 1, "%c%c%c%c", &conn_pattern[0], &conn_pattern[1], &conn_pattern[2], &conn_pattern[3]);
                        if (ec != 4) {
                            // missing connections mask pattern
                            return ERR_INVALID_INPUT;
                        }
                        str += 4;
                        for (int cmi = 0; cmi < 4; cmi++) {
                            if (conn_pattern[cmi] != '.' && conn_pattern[cmi] != ':') {
                                // missing connections mask pattern
                                return ERR_INVALID_INPUT;
                            }
                        }
                        conn_mask |= (conn_pattern[0] == ':' ? TWIXT_PP_DIR_RT : 0);
                        conn_mask |= (conn_pattern[1] == ':' ? TWIXT_PP_DIR_RB : 0);
                        conn_mask |= (conn_pattern[2] == ':' ? TWIXT_PP_DIR_BR : 0);
                        conn_mask |= (conn_pattern[3] == ':' ? TWIXT_PP_DIR_BL : 0);
                    }
                    conn_masks[y][x] = conn_mask;
                    set_node(self, x++, y, TWIXT_PP_PLAYER_WHITE, 0x00, NULL);
                    o_moves++;
                    if (o_moves == 1 && x_moves == 0) {
                        data.swap_target = ((x - 1) << 8) | y;
                    }
                } break;
                case 'X': {
                    if (x < 0 || x >= opts.wx || y < 0 || y > opts.wy || ((y == 0 || y == opts.wy - 1) && x == opts.wx - 1)) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    // get realized connections mask, if any
                    uint8_t conn_mask = 0x0F;
                    if (*(str + 1) == '.' || *(str + 1) == ':') {
                        conn_mask = 0;
                        char conn_pattern[4];
                        int ec = sscanf(str + 1, "%c%c%c%c", &conn_pattern[0], &conn_pattern[1], &conn_pattern[2], &conn_pattern[3]);
                        if (ec != 4) {
                            // missing connections mask pattern
                            return ERR_INVALID_INPUT;
                        }
                        str += 4;
                        for (int cmi = 0; cmi < 4; cmi++) {
                            if (conn_pattern[cmi] != '.' && conn_pattern[cmi] != ':') {
                                // missing connections mask pattern
                                return ERR_INVALID_INPUT;
                            }
                        }
                        conn_mask |= (conn_pattern[0] == ':' ? TWIXT_PP_DIR_RT : 0);
                        conn_mask |= (conn_pattern[1] == ':' ? TWIXT_PP_DIR_RB : 0);
                        conn_mask |= (conn_pattern[2] == ':' ? TWIXT_PP_DIR_BR : 0);
                        conn_mask |= (conn_pattern[3] == ':' ? TWIXT_PP_DIR_BL : 0);
                    }
                    conn_masks[y][x] = conn_mask;
                    set_node(self, x++, y, TWIXT_PP_PLAYER_BLACK, 0x00, NULL);
                    x_moves++;
                } break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': { // empty cells
                    uint32_t dacc = 0;
                    while (true) {
                        if (*str == '\0') {
                            return ERR_INVALID_INPUT;
                        }
                        if ((*str) < '0' || (*str) > '9') {
                            str--; // loop increments itself later on, we only peek the value here
                            break;
                        }
                        dacc *= 10;
                        uint32_t ladd = (*str) - '0';
                        if (ladd > opts.wx) {
                            return ERR_INVALID_INPUT;
                        }
                        dacc += ladd;
                        str++;
                    }
                    for (int place_empty = 0; place_empty < dacc; place_empty++) {
                        if (x < 0 || x >= opts.wx || y < 0 || y > opts.wy || ((y == 0 || y == opts.wy - 1) && x == opts.wx - 1)) {
                            // out of bounds board
                            return ERR_INVALID_INPUT;
                        }
                        set_node(self, x++, y, TWIXT_PP_PLAYER_NONE, 0xFF, NULL);
                    }
                } break;
                case '/': { // advance to next
                    y++;
                    x = (y == opts.wy - 1 ? 1 : 0);
                } break;
                case ' ': { // advance to next segment
                    advance_segment = true;
                } break;
                default: {
                    // failure, ran out of str to use or got invalid character
                    return ERR_INVALID_INPUT;
                } break;
            }
            str++;
        }
        // disable swap status if unavailable by x/o moves
        if (o_moves > 1 || x_moves > 0) {
            data.pie_swap &= false;
        }
        // place all nodes again, this time with their conn mask
        {
            TWIXT_PP_PLAYER cell_player;
            for (int y = 0; y < opts.wy; y++) {
                for (int x = 0; x < opts.wx; x++) {
                    get_node(self, x, y, &cell_player);
                    if (cell_player == TWIXT_PP_PLAYER_INVALID || cell_player == TWIXT_PP_PLAYER_NONE) {
                        continue;
                    } else {
                        set_node(self, x, y, cell_player, conn_masks[y][x], NULL);
                        //TODO record iswin to check with the later set winning player
                    }
                }
            }
        }
        // current player
        switch (*str) {
            case '-': {
                data.current_player = TWIXT_PP_PLAYER_NONE;
            } break;
            case 'O': {
                data.current_player = TWIXT_PP_PLAYER_WHITE;
            } break;
            case 'X': {
                data.current_player = TWIXT_PP_PLAYER_BLACK;
            } break;
            default: {
                // failure, ran out of str to use or got invalid character
                return ERR_INVALID_INPUT;
            } break;
        }
        // swap is only available for strictly legal play
        if (data.pie_swap && ((o_moves == 1 && data.current_player != TWIXT_PP_PLAYER_BLACK) || (o_moves == 0 && data.current_player != TWIXT_PP_PLAYER_WHITE))) {
            data.pie_swap &= false;
        }
        str++;
        if (*str != ' ') {
            // failure, ran out of str to use or got invalid character
            return ERR_INVALID_INPUT;
        }
        str++;
        // result player
        switch (*str) {
            case '-': {
                data.winning_player = TWIXT_PP_PLAYER_NONE;
            } break;
            case 'O': {
                data.winning_player = TWIXT_PP_PLAYER_WHITE;
            } break;
            case 'X': {
                data.winning_player = TWIXT_PP_PLAYER_BLACK;
            } break;
            default: {
                // failure, ran out of str to use or got invalid character
                return ERR_INVALID_INPUT;
            } break;
        }
        return ERR_OK;
    }

    error_code export_state(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        const char* ostr = str;
        TWIXT_PP_PLAYER cell_player;
        for (int y = 0; y < opts.wy; y++) {
            int empty_cells = 0;
            for (int x = 0; x < opts.wx; x++) {
                get_node(self, x, y, &cell_player);
                if (cell_player == TWIXT_PP_PLAYER_INVALID) {
                    continue;
                }
                if (cell_player == TWIXT_PP_PLAYER_NONE) {
                    empty_cells++;
                } else {
                    // if the current cell isnt empty, print its representation, before that print empty cells, if any
                    if (empty_cells > 0) {
                        str += sprintf(str, "%d", empty_cells);
                        empty_cells = 0;
                    }
                    str += sprintf(str, "%c", (cell_player == TWIXT_PP_PLAYER_WHITE ? 'O' : 'X'));
                    // if the current node does not have all right/bottom dir connections available actually placed, add the connection pattern
                    uint8_t dir_available = 0;
                    TWIXT_PP_PLAYER avail_player;
                    get_node(self, x + 2, y - 1, &avail_player);
                    dir_available |= (cell_player == avail_player ? TWIXT_PP_DIR_RT : 0);
                    get_node(self, x + 2, y + 1, &avail_player);
                    dir_available |= (cell_player == avail_player ? TWIXT_PP_DIR_RB : 0);
                    get_node(self, x + 1, y + 2, &avail_player);
                    dir_available |= (cell_player == avail_player ? TWIXT_PP_DIR_BR : 0);
                    get_node(self, x - 1, y + 2, &avail_player);
                    dir_available |= (cell_player == avail_player ? TWIXT_PP_DIR_BL : 0);
                    uint8_t realized_conns;
                    get_node_connections(self, x, y, &realized_conns);
                    if (dir_available & ~realized_conns) {
                        // there exist available connections that are not realized, emit connection pattern
                        str += sprintf(str, "%c%c%c%c", (realized_conns & TWIXT_PP_DIR_RT) ? ':' : '.', (realized_conns & TWIXT_PP_DIR_RB) ? ':' : '.', (realized_conns & TWIXT_PP_DIR_BR) ? ':' : '.', (realized_conns & TWIXT_PP_DIR_BL) ? ':' : '.');
                    }
                }
            }
            if (empty_cells > 0) {
                str += sprintf(str, "%d", empty_cells);
            }
            if (y < opts.wy - 1) {
                str += sprintf(str, "/");
            }
        }
        // current player
        player_id ptm;
        uint8_t ptm_count;
        players_to_move(self, &ptm_count, &ptm);
        if (ptm_count == 0) {
            ptm = TWIXT_PP_PLAYER_NONE;
        }
        switch (ptm) {
            case TWIXT_PP_PLAYER_NONE: {
                str += sprintf(str, " -");
            } break;
            case TWIXT_PP_PLAYER_WHITE: {
                str += sprintf(str, " O");
            } break;
            case TWIXT_PP_PLAYER_BLACK: {
                str += sprintf(str, " X");
            } break;
        }
        // result player
        player_id res;
        uint8_t res_count;
        get_results(self, &res_count, &res);
        if (res_count == 0) {
            res = TWIXT_PP_PLAYER_NONE;
        }
        switch (res) {
            case TWIXT_PP_PLAYER_NONE: {
                str += sprintf(str, " -");
            } break;
            case TWIXT_PP_PLAYER_WHITE: {
                str += sprintf(str, " O");
            } break;
            case TWIXT_PP_PLAYER_BLACK: {
                str += sprintf(str, " X");
            } break;
        }
        *ret_size = str - ostr;
        return ERR_OK;
    }

    error_code players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = get_repr(self);
        if (data.current_player == PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = data.current_player;
        return ERR_OK;
    }

    error_code get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        if (moves == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        uint32_t move_cnt = 0;
        for (int iy = 0; iy < opts.wy; iy++) {
            if ((iy == 0 || iy == opts.wy - 1) && player == TWIXT_PP_PLAYER_BLACK) {
                continue;
            }
            for (int ix = 0; ix < opts.wx; ix++) {
                if ((ix == 0 || ix == opts.wx - 1) && player == TWIXT_PP_PLAYER_WHITE) {
                    continue;
                }
                if (data.gameboard[iy][ix].player == TWIXT_PP_PLAYER_NONE) {
                    // add the free tile to the return vector
                    moves[move_cnt++] = (ix << 8) | iy;
                }
            }
        }
        if (data.pie_swap == true && data.current_player == TWIXT_PP_PLAYER_BLACK) {
            moves[move_cnt++] = TWIXT_PP_MOVE_SWAP;
        }
        *ret_count = move_cnt;
        return ERR_OK;
    }

    error_code is_legal_move(game* self, player_id player, move_code move, sync_counter sync)
    {
        if (move == MOVE_NONE) {
            return ERR_INVALID_INPUT;
        }
        player_id ptm;
        uint8_t ptm_count;
        players_to_move(self, &ptm_count, &ptm);
        if (ptm != player) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
        if (move == TWIXT_PP_MOVE_SWAP) {
            if (data.pie_swap == true && data.current_player == TWIXT_PP_PLAYER_BLACK) {
                return ERR_OK;
            }
            return ERR_INVALID_INPUT;
        }
        int ix = (move >> 8) & 0xFF;
        int iy = move & 0xFF;
        if (data.gameboard[iy][ix].player != TWIXT_PP_PLAYER_NONE) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = get_opts(self);
        if (((ix == 0 || ix == opts.wx - 1) && player == TWIXT_PP_PLAYER_WHITE) || ((iy == 0 || iy == opts.wy - 1) && player == TWIXT_PP_PLAYER_BLACK)) {
            return ERR_INVALID_INPUT;
        }
        return ERR_OK;
    }

    error_code make_move(game* self, player_id player, move_code move)
    {
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);

        if (move == TWIXT_PP_MOVE_SWAP) {
            // use swap target to give whites move to black
            int sx = (data.swap_target >> 8) & 0xFF;
            int sy = data.swap_target & 0xFF;

            //BUG on non-square board this swap can access out of bounds elements
            data.gameboard[sy][sx].player = TWIXT_PP_PLAYER_BLACK;
            if (sx != sy) {
                data.gameboard[sx][sy] = data.gameboard[sy][sx];
                data.gameboard[sy][sx] = (twixt_pp_node){TWIXT_PP_PLAYER_NONE, 0, 0, 0};
            }

            data.pie_swap = false;
            data.current_player = TWIXT_PP_PLAYER_WHITE;
            return ERR_OK;
        }

        int tx = (move >> 8) & 0xFF;
        int ty = move & 0xFF;

        if (data.pie_swap == true) {
            if (data.current_player == TWIXT_PP_PLAYER_WHITE) {
                data.swap_target = (tx << 8) | ty;
            }
            if (data.current_player == TWIXT_PP_PLAYER_BLACK) {
                data.pie_swap = false;
            }
        }

        bool wins;
        // set node updates graph structures in the backend, and informs us if this move is winning for the current player
        set_node(self, tx, ty, data.current_player, 0xFF, &wins);

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

    error_code get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = get_repr(self);
        if (data.current_player != TWIXT_PP_PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = data.winning_player;
        return ERR_OK;
    }

    error_code playout(game* self, uint64_t seed)
    {
        uint32_t ctr = 0;
        move_code* moves;
        uint32_t moves_count;
        get_concrete_moves(self, TWIXT_PP_PLAYER_NONE, &moves_count, moves);
        moves = (move_code*)malloc(moves_count * sizeof(move_code));
        player_id ptm;
        uint8_t ptm_count;
        players_to_move(self, &ptm_count, &ptm);
        while (ptm_count > 0) {
            get_concrete_moves(self, ptm, &moves_count, moves);
            make_move(self, ptm, moves[squirrelnoise5(ctr++, seed) % moves_count]);
            players_to_move(self, &ptm_count, &ptm);
        }
        free(moves);
        return ERR_OK;
    }

    error_code get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        if (strlen(str) >= 1 && str[0] == '-') {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        if (strlen(str) < 2) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = get_opts(self);
        if (opts.pie_swap == true && strcmp(str, "swap") == 0) {
            *ret_move = TWIXT_PP_MOVE_SWAP;
            return ERR_OK;
        }
        char msc = '\0';
        char lsc = '\0';
        uint8_t y = 0;
        int ec = sscanf(str, "%c%c%hhu", &msc, &lsc, &y);
        if (ec != 3) {
            msc = 'a' - 1;
            ec = sscanf(str, "%c%hhu", &lsc, &y);
            if (ec != 2) {
                return ERR_INVALID_INPUT;
            }
        }
        uint8_t x = ((msc - 'a' + 1) * 26) + (lsc - 'a');
        // y = opts.wy - y - 1; // swap out y coords so axes are bottom left
        y--; // move strings go from 1-n
        TWIXT_PP_PLAYER cell_player;
        get_node(self, x, y, &cell_player);
        if (cell_player == TWIXT_PP_PLAYER_INVALID) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        *ret_move = (x << 8) | y;
        return ERR_OK;
    }

    error_code get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        if (move == MOVE_NONE) {
            *ret_size = sprintf(str_buf, "-");
            return ERR_OK;
        }
        opts_repr& opts = get_opts(self);
        if (opts.pie_swap == true && move == TWIXT_PP_MOVE_SWAP) {
            *ret_size = sprintf(str_buf, "swap");
            return ERR_OK;
        }
        uint8_t x = (move >> 8) & 0xFF;
        uint8_t y = move & 0xFF;
        // y = opts.wy - y - 1; // swap out y coords so axes are bottom left
        y++; // move strings go from 1-n
        char msc = '\0';
        if (x > 25) {
            msc = 'a' + (x / 26) - 1;
            x = x - (26 * (x / 26));
        }
        if (msc == '\0') {
            *ret_size = sprintf(str_buf, "%c%hhu", 'a' + x, y);
        } else {
            *ret_size = sprintf(str_buf, "%c%c%hhu", msc, 'a' + x, y);
        }
        return ERR_OK;
    }

    //TODO somehow needs to display disambiguation information for position where realized connections of nodes are not clear
    error_code print(game* self, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        const char* ostr = str_buf;

        // print all node infos and all graphs for debugging purposes
        // for (int iy = 0; iy < opts.wy; iy++) {
        //     for (int ix = 0; ix < opts.wy; ix++) {
        //         if (data.gameboard[iy][ix].player != TWIXT_PP_PLAYER_INVALID && data.gameboard[iy][ix].player != TWIXT_PP_PLAYER_NONE) {
        //             printf("%d-%d: (%c) %hu, CON:%hhu COL:%hhu\n", ix, iy, data.gameboard[iy][ix].player == TWIXT_PP_PLAYER_WHITE ? 'O' : 'X' ,data.gameboard[iy][ix].graph_id, data.gameboard[iy][ix].connections, data.gameboard[iy][ix].collisions);
        //         }
        //     }
        // }
        // for (auto it = data.graph_map.begin(); it != data.graph_map.end(); it++) {
        //     printf("[%hu]: %hu, low:%u high:%u\n", it->first, it->second.graph_id, it->second.connect_low, it->second.connect_high);
        // }

        for (int iy = 0; iy < opts.wy; iy++) {
            for (int ix = 0; ix < opts.wy; ix++) {
                TWIXT_PP_PLAYER node_player = TWIXT_PP_PLAYER_INVALID;
                get_node(self, ix, iy, &node_player);
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
        *ret_size = str_buf - ostr;
        return ERR_OK;
    }

    //=====
    // game internal methods

    error_code get_node(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER* p)
    {
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        if (x < 0 || y < 0 || x >= opts.wx || y >= opts.wy) {
            *p = TWIXT_PP_PLAYER_INVALID;
        } else {
            *p = data.gameboard[y][x].player;
        }
        return ERR_OK;
    }

    error_code set_node(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER p, uint8_t connection_mask, bool* wins)
    {
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        data.gameboard[y][x].player = p;
        if (p == TWIXT_PP_PLAYER_NONE) {
            if (wins) {
                *wins = false;
            }
            return ERR_OK;
        }
        // try to place all connections
        bool win = false;
        bool rwins;
        if (connection_mask & TWIXT_PP_DIR_RT) {
            set_connection(self, x, y, x + 2, y - 1, &rwins);
            win |= rwins;
        }
        if (connection_mask & TWIXT_PP_DIR_RB) {
            set_connection(self, x, y, x + 2, y + 1, &rwins);
            win |= rwins;
        }
        if (connection_mask & TWIXT_PP_DIR_BR) {
            set_connection(self, x, y, x + 1, y + 2, &rwins);
            win |= rwins;
        }
        if (connection_mask & TWIXT_PP_DIR_BL) {
            set_connection(self, x, y, x - 1, y + 2, &rwins);
            win |= rwins;
        }
        if (connection_mask & (TWIXT_PP_DIR_RT << 4)) {
            set_connection(self, x - 2, y + 1, x, y, &rwins);
            win |= rwins;
        }
        if (connection_mask & (TWIXT_PP_DIR_RB << 4)) {
            set_connection(self, x - 2, y - 1, x, y, &rwins);
            win |= rwins;
        }
        if (connection_mask & (TWIXT_PP_DIR_BR << 4)) {
            set_connection(self, x - 1, y - 2, x, y, &rwins);
            win |= rwins;
        }
        if (connection_mask & (TWIXT_PP_DIR_BL << 4)) {
            set_connection(self, x + 1, y - 2, x, y, &rwins);
            win |= rwins;
        }
        if (data.gameboard[y][x].graph_id == 0) {
            // no connection created, so this node was not joined into another graph, create a new graph for it, and give it the nodes connect qualities
            data.gameboard[y][x].graph_id = data.next_graph_id;
            data.graph_map[data.next_graph_id] = (twixt_pp_graph){
                .graph_id = data.next_graph_id,
                .connect_low = (x == 0 || y == 0),
                .connect_high = (x == opts.wx - 1 || y == opts.wy - 1),
            };
            data.next_graph_id++;
        }
        if (wins) {
            *wins = win;
        }
        return ERR_OK;
    }

    error_code get_node_connections(game* self, uint8_t x, uint8_t y, uint8_t* connections)
    {
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        if (x < 0 || y < 0 || x >= opts.wx || y >= opts.wy) {
            *connections = 0;
        } else {
            *connections = data.gameboard[y][x].connections;
        }
        return ERR_OK;
    }

    error_code get_node_collisions(game* self, uint8_t x, uint8_t y, uint8_t* collisions)
    {
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        if (x < 0 || y < 0 || x >= opts.wx || y >= opts.wy) {
            *collisions = 0;
        } else {
            *collisions = data.gameboard[y][x].collisions;
        }
        return ERR_OK;
    }

    // impl hidden: try to add the collision if node exists, do not adjust by player offset
    void add_collision(game* self, uint8_t x, uint8_t y, uint8_t dir, TWIXT_PP_PLAYER np)
    {
        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        if (x < 0 || y < 0 || x >= opts.wx || y >= opts.wy) {
            return;
        }
        data.gameboard[y][x].collisions |= (dir << (np == TWIXT_PP_PLAYER_WHITE ? 4 : 0));
    }

    error_code set_connection(game* self, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool* wins)
    {
        // check that all of the point exist, and there is no out of bounds
        TWIXT_PP_PLAYER np1;
        get_node(self, x1, y1, &np1);
        TWIXT_PP_PLAYER np2;
        get_node(self, x2, y2, &np2);
        if (np1 == TWIXT_PP_PLAYER_INVALID || np2 == TWIXT_PP_PLAYER_INVALID || np1 != np2 || np1 == TWIXT_PP_PLAYER_NONE) {
            return ERR_OK;
        }

        opts_repr& opts = get_opts(self);
        data_repr& data = get_repr(self);
        // get what type of connection this is
        uint8_t conn_dir = 0;
        if (x2 < x1) {
            // bottom left
            conn_dir = TWIXT_PP_DIR_BL;
        } else if (y2 < y1) {
            // right top
            conn_dir = TWIXT_PP_DIR_RT;
        } else if (x2 - x1 > y2 - y1) {
            // right bottom
            conn_dir = TWIXT_PP_DIR_RB;
        } else {
            // bottom right
            conn_dir = TWIXT_PP_DIR_BR;
        }

        // place only if collision bit is not set
        if ((data.gameboard[y1][x1].collisions & (conn_dir << (np1 == TWIXT_PP_PLAYER_WHITE ? 4 : 0))) != 0) {
            return ERR_OK;
        }
        data.gameboard[y1][x1].connections |= conn_dir;

        // invalidate the opponents collision bits for all of the 9 crossing connections
        TWIXT_PP_PLAYER op = (np1 == TWIXT_PP_PLAYER_WHITE ? TWIXT_PP_PLAYER_BLACK : TWIXT_PP_PLAYER_WHITE);
        switch (conn_dir) {
            /*
            numbers do matter, if drawn on a board then you can mirror it to the other side and they will still hold
            likewise just rotating gives the other combination for both mirrored sides
            */
            case (TWIXT_PP_DIR_RT): { // right top
                add_collision(self, x1 + 1, y1 - 1, TWIXT_PP_DIR_RB, op); // 1
                add_collision(self, x1 + 2, y1 - 2, TWIXT_PP_DIR_BL, op); // 2
                add_collision(self, x1 + 1, y1 - 2, TWIXT_PP_DIR_BR, op); // 3
                add_collision(self, x1 + 0, y1 - 1, TWIXT_PP_DIR_RB, op); // 4
                add_collision(self, x1 + 1, y1 - 1, TWIXT_PP_DIR_BR, op); // 5
                add_collision(self, x1 + 1, y1 - 1, TWIXT_PP_DIR_BL, op); // 6
                add_collision(self, x1 + 0, y1 - 2, TWIXT_PP_DIR_BR, op); // 7
                add_collision(self, x1 - 1, y1 - 1, TWIXT_PP_DIR_RB, op); // 8
                add_collision(self, x1 + 0, y1 - 1, TWIXT_PP_DIR_BR, op); // 9
            } break;
            case (TWIXT_PP_DIR_RB): { // right bottom
                add_collision(self, x1 - 1, y1 + 1, TWIXT_PP_DIR_RT, op); // 1
                add_collision(self, x1 + 0, y1 - 1, TWIXT_PP_DIR_BR, op); // 2
                add_collision(self, x1 + 1, y1 - 1, TWIXT_PP_DIR_BL, op); // 3
                add_collision(self, x1 + 0, y1 + 1, TWIXT_PP_DIR_RT, op); // 4
                add_collision(self, x1 + 1, y1 + 0, TWIXT_PP_DIR_BL, op); // 5
                add_collision(self, x1 + 1, y1 + 0, TWIXT_PP_DIR_BR, op); // 6
                add_collision(self, x1 + 2, y1 - 1, TWIXT_PP_DIR_BL, op); // 7
                add_collision(self, x1 + 1, y1 + 1, TWIXT_PP_DIR_RT, op); // 8
                add_collision(self, x1 + 2, y1 + 0, TWIXT_PP_DIR_BL, op); // 9
            } break;
            case (TWIXT_PP_DIR_BR): { // bottom right
                add_collision(self, x1 + 1, y1 + 1, TWIXT_PP_DIR_BL, op); // 1
                add_collision(self, x1 + 0, y1 + 1, TWIXT_PP_DIR_RB, op); // 2
                add_collision(self, x1 + 0, y1 + 2, TWIXT_PP_DIR_RT, op); // 3
                add_collision(self, x1 + 1, y1 + 0, TWIXT_PP_DIR_BL, op); // 4
                add_collision(self, x1 - 1, y1 + 2, TWIXT_PP_DIR_RT, op); // 5
                add_collision(self, x1 - 1, y1 + 0, TWIXT_PP_DIR_RB, op); // 6
                add_collision(self, x1 + 0, y1 + 1, TWIXT_PP_DIR_RT, op); // 7
                add_collision(self, x1 + 1, y1 - 1, TWIXT_PP_DIR_BL, op); // 8
                add_collision(self, x1 - 1, y1 + 1, TWIXT_PP_DIR_RT, op); // 9
            } break;
            case (TWIXT_PP_DIR_BL): { // bottom left
                add_collision(self, x1 - 1, y1 - 1, TWIXT_PP_DIR_BR, op); // 1
                add_collision(self, x1 - 1, y1 + 1, TWIXT_PP_DIR_RT, op); // 2
                add_collision(self, x1 - 1, y1 + 0, TWIXT_PP_DIR_RB, op); // 3
                add_collision(self, x1 - 1, y1 + 0, TWIXT_PP_DIR_BR, op); // 4
                add_collision(self, x1 - 2, y1 + 0, TWIXT_PP_DIR_RB, op); // 5
                add_collision(self, x1 - 2, y1 + 2, TWIXT_PP_DIR_RT, op); // 6
                add_collision(self, x1 - 1, y1 + 1, TWIXT_PP_DIR_RB, op); // 7
                add_collision(self, x1 - 1, y1 + 1, TWIXT_PP_DIR_BR, op); // 8
                add_collision(self, x1 - 2, y1 + 1, TWIXT_PP_DIR_RB, op); // 9
            } break;
        }

        // get graph id of both nodes, at least one will exist
        uint16_t graph_id1 = data.gameboard[y1][x1].graph_id;
        uint16_t graph_id2 = data.gameboard[y2][x2].graph_id;

        // get merged connect quality for both from graph, or from node if no parent graph, save in cq_*1
        // also, while in the check, resolve graph ids to their true parent graph if applicable
        bool cq_low;
        bool cq_high;
        if (graph_id1 > 0) {
            while (graph_id1 != data.graph_map[graph_id1].graph_id) {
                graph_id1 = data.graph_map[graph_id1].graph_id;
            }
            cq_low = data.graph_map[graph_id1].connect_low;
            cq_high = data.graph_map[graph_id1].connect_high;
        } else {
            cq_low = (x1 == 0 || y1 == 0);
            cq_high = (x1 == opts.wx - 1 || y1 == opts.wy - 1);
        }
        if (graph_id2 > 0) {
            while (graph_id2 != data.graph_map[graph_id2].graph_id) {
                graph_id2 = data.graph_map[graph_id2].graph_id;
            }
            cq_low |= data.graph_map[graph_id2].connect_low;
            cq_high |= data.graph_map[graph_id2].connect_high;
        } else {
            cq_low |= (x2 == 0 || y2 == 0);
            cq_high |= (x2 == opts.wx - 1 || y2 == opts.wy - 1);
        }

        if (graph_id1 == 0) {
            // one node has no parent graph, merge it into the other graph AND merge any connect qualities
            data.gameboard[y1][x1].graph_id = graph_id2;
            data.graph_map[graph_id2].connect_low |= cq_low;
            data.graph_map[graph_id2].connect_high |= cq_high;
            graph_id1 = graph_id2; // swap ids for win check later
        } else if (graph_id2 == 0) {
            // same as above but for graph_id2
            data.gameboard[y2][x2].graph_id = graph_id1;
            data.graph_map[graph_id1].connect_low |= cq_low;
            data.graph_map[graph_id1].connect_high |= cq_high;
        } else {
            // if both have a parent graph,  this means the node being added was parented into the graph map in an earlier connection
            // just need to merge the graphs connect qualities now, and reparent one onto the other
            data.graph_map[graph_id1].connect_low |= cq_low;
            data.graph_map[graph_id1].connect_high |= cq_high;
            data.graph_map[graph_id2].graph_id = graph_id1;
        }

        // if the resulting new parent graph for both nodes now contains both connect qualities, then mark as wins=true
        if (data.graph_map[graph_id1].connect_low && data.graph_map[graph_id1].connect_high) {
            *wins = true;
        } else {
            *wins = false;
        }

        return ERR_OK;
    }

    error_code can_swap(game* self, bool* swap_available)
    {
        data_repr& data = get_repr(self);
        *swap_available = (data.pie_swap == true && data.current_player == TWIXT_PP_PLAYER_BLACK);
        return ERR_OK;
    }

} // namespace

static const twixt_pp_internal_methods twixt_pp_gbe_internal_methods{
    .get_node = get_node,
    .set_node = set_node,
    .get_node_connections = get_node_connections,
    .get_node_collisions = get_node_collisions,
    .set_connection = set_connection,
    .can_swap = can_swap,
};

const game_methods twixt_pp_gbe{

    .game_name = "TwixT",
    .variant_name = "PP",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .error_strings = false,
        .options = true,
        .options_bin = true,
        .options_bin_ref = true,
        .serializable = false,
        .legacy = false,
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = false,
        .sync_counter = false,
        .move_ordering = false,
        .scores = false,
        .id = false,
        .eval = false,
        .playout = true,
        .print = true,
    },
    .internal_methods = (void*)&twixt_pp_gbe_internal_methods,

#include "surena/game_impl.h"

};
