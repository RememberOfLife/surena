#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "surena/util/fast_prng.h"
#include "surena/util/noise.h"
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/tictactoe_ultimate.h"

namespace {

    // general purpose helpers for opts, data

    struct data_repr {
        // origin bottom left, y upwards, x to the right
        // individual boards work like standard tictactoe board
        uint32_t board[3][3];
        uint32_t global_board; // 0 is ongoing, 3 is draw result
        // global target is the local board that the current player has to play to
        int8_t global_target_x;
        int8_t global_target_y;
        player_id current_player;
        player_id winning_player;
    };

    data_repr& get_repr(game* self)
    {
        return *((data_repr*)(self->data1));
    }

    // forward declare everything to allow for inlining at least in this unit
    GF_UNUSED(get_last_error);
    error_code create(game* self, game_init init_info);
    GF_UNUSED(export_options_str);
    GF_UNUSED(get_options_bin_ref);
    error_code destroy(game* self);
    error_code clone(game* self, game* clone_target);
    error_code copy_from(game* self, game* other);
    error_code compare(game* self, game* other, bool* ret_equal);
    error_code export_state(game* self, size_t* ret_size, char* str);
    error_code import_state(game* self, const char* str);
    GF_UNUSED(serialize);
    error_code players_to_move(game* self, uint8_t* ret_count, player_id* players);
    error_code get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    GF_UNUSED(get_concrete_move_probabilities);
    GF_UNUSED(get_concrete_moves_ordered);
    GF_UNUSED(get_actions);
    error_code is_legal_move(game* self, player_id player, move_code move, sync_counter sync);
    GF_UNUSED(move_to_action);
    GF_UNUSED(is_action);
    error_code make_move(game* self, player_id player, move_code move);
    error_code get_results(game* self, uint8_t* ret_count, player_id* players);
    GF_UNUSED(export_legacy);
    GF_UNUSED(get_sync_counter);
    GF_UNUSED(get_scores);
    error_code id(game* self, uint64_t* ret_id);
    GF_UNUSED(eval);
    GF_UNUSED(discretize);
    error_code playout(game* self, uint64_t seed);
    GF_UNUSED(redact_keep_state);
    GF_UNUSED(export_sync_data);
    GF_UNUSED(release_sync_data);
    GF_UNUSED(import_sync_data);
    error_code get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    error_code get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    error_code print(game* self, size_t* ret_size, char* str_buf);

    error_code check_result(game* self, uint32_t state, player_id* ret_p);
    error_code get_cell(game* self, uint32_t state, int x, int y, player_id* ret_p);
    error_code set_cell(game* self, uint32_t* state, int x, int y, player_id p);
    error_code get_cell_global(game* self, int x, int y, player_id* ret_p);
    error_code set_cell_global(game* self, int x, int y, player_id p);
    error_code get_cell_local(game* self, int x, int y, player_id* ret_p);
    error_code set_cell_local(game* self, int x, int y, player_id p);
    error_code get_global_target(game* self, uint8_t* ret);
    error_code set_global_target(game* self, int x, int y);
    error_code set_current_player(game* self, player_id p);
    error_code set_result(game* self, player_id p);

    // implementation

    error_code create(game* self, game_init init_info)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        self->data2 = NULL;
        self->sizer = (buf_sizer){
            .state_str = 37,
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = 81,
            .max_results = 1,
            .move_str = 3,
            .print_str = 180, // slightly oversized
        };
        const char* initial_state = NULL;
        if (init_info.source_type == GAME_INIT_SOURCE_TYPE_STANDARD) {
            initial_state = init_info.source.standard.initial_state;
        }
        return import_state(self, initial_state);
    }

    error_code destroy(game* self)
    {
        free(self->data1);
        self->data1 = NULL;
        return ERR_OK;
    }

    error_code clone(game* self, game* clone_target)
    {
        if (clone_target == NULL) {
            return ERR_INVALID_INPUT;
        }
        clone_target->methods = self->methods;
        error_code ec = clone_target->methods->create(clone_target, (game_init){.source_type = GAME_INIT_SOURCE_TYPE_DEFAULT});
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

    error_code export_state(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
        const char* ostr = str;
        player_id cell_player;
        for (int y = 8; y >= 0; y--) {
            int empty_squares = 0;
            for (int x = 0; x < 9; x++) {
                get_cell_local(self, x, y, &cell_player);
                if (cell_player == PLAYER_NONE) {
                    empty_squares++;
                } else {
                    // if the current square isnt empty, print its representation, before that print empty squares, if any
                    if (empty_squares > 0) {
                        str += sprintf(str, "%d", empty_squares);
                        empty_squares = 0;
                    }
                    str += sprintf(str, "%c", (cell_player == 1 ? 'X' : 'O'));
                }
            }
            if (empty_squares > 0) {
                str += sprintf(str, "%d", empty_squares);
            }
            if (y > 0) {
                str += sprintf(str, "/");
            }
        }
        // global target
        if (data.global_target_x >= 0 && data.global_target_y >= 0) {
            str += sprintf(str, " %c%c", 'a' + data.global_target_x, '0' + data.global_target_y);
        } else {
            str += sprintf(str, " -");
        }
        // current player
        player_id ptm;
        uint8_t ptm_count;
        players_to_move(self, &ptm_count, &ptm);
        if (ptm_count == 0) {
            ptm = PLAYER_NONE;
        }
        switch (ptm) {
            case PLAYER_NONE: {
                str += sprintf(str, " -");
            } break;
            case 1: {
                str += sprintf(str, " X");
            } break;
            case 2: {
                str += sprintf(str, " O");
            } break;
        }
        // result player
        player_id res;
        uint8_t res_count;
        get_results(self, &res_count, &res);
        if (res_count == 0) {
            res = PLAYER_NONE;
        }
        switch (res) {
            case PLAYER_NONE: {
                str += sprintf(str, " -");
            } break;
            case 1: {
                str += sprintf(str, " X");
            } break;
            case 2: {
                str += sprintf(str, " O");
            } break;
        }
        *ret_size = str - ostr;
        return ERR_OK;
    }

    error_code import_state(game* self, const char* str)
    {
        data_repr& data = get_repr(self);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                data.board[y][x] = 0;
            }
        }
        data.global_board = 0;
        data.global_target_x = -1;
        data.global_target_y = -1;
        if (str == NULL) {
            data.current_player = 1;
            data.winning_player = 0;
            return ERR_OK;
        }
        // load from diy tictactoe format, "board target p_cur p_res"
        int y = 8;
        int x = 0;
        // get square fillings
        bool advance_segment = false;
        while (!advance_segment) {
            switch (*str) {
                case 'X': {
                    if (x > 8 || y < 0) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    set_cell_local(self, x++, y, 1);
                } break;
                case 'O': {
                    if (x > 8 || y < 0) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    set_cell_local(self, x++, y, 2);
                } break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': { // empty squares
                    for (int place_empty = (*str) - '0'; place_empty > 0; place_empty--) {
                        if (x > 8) {
                            // out of bounds board
                            return ERR_INVALID_INPUT;
                        }
                        set_cell_local(self, x++, y, PLAYER_NONE);
                    }
                } break;
                case '/': { // advance to next
                    y--;
                    x = 0;
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
        // update global board
        for (int iy = 0; iy < 3; iy++) {
            for (int ix = 0; ix < 3; ix++) {
                uint8_t local_result;
                check_result(self, data.board[iy][ix], &local_result);
                if (local_result > 0) {
                    set_cell_global(self, ix, iy, local_result);
                }
            }
        }
        // get global target, if any, otherwise its reset already
        if (*str != '-') {
            data.global_target_x = (*str) - 'a';
            str++;
            data.global_target_y = (*str) - '1';
            if (data.global_target_x < 0 || data.global_target_x > 2 || data.global_target_y < 0 || data.global_target_y > 2) {
                return ERR_INVALID_INPUT;
            }
        }
        str++;
        if (*str != ' ') {
            // failure, ran out of str to use or got invalid character
            return ERR_INVALID_INPUT;
        }
        str++;
        // current player
        switch (*str) {
            case '-': {
                set_current_player(self, PLAYER_NONE);
            } break;
            case 'X': {
                set_current_player(self, 1);
            } break;
            case 'O': {
                set_current_player(self, 2);
            } break;
            default: {
                // failure, ran out of str to use or got invalid character
                return ERR_INVALID_INPUT;
            } break;
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
                set_result(self, PLAYER_NONE);
            } break;
            case 'X': {
                set_result(self, 1);
            } break;
            case 'O': {
                set_result(self, 2);
            } break;
            default: {
                // failure, ran out of str to use or got invalid character
                return ERR_INVALID_INPUT;
            } break;
        }
        return ERR_OK;
    }

    error_code players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_OK;
        }
        *ret_count = 1;
        data_repr& data = get_repr(self);
        player_id ptm = data.current_player;
        if (ptm == PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = ptm;
        return ERR_OK;
    }

    error_code get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        if (moves == NULL) {
            return ERR_INVALID_INPUT;
        }
        player_id ptm;
        uint8_t ptm_count;
        players_to_move(self, &ptm_count, &ptm);
        if (ptm_count == 0 || player != ptm) {
            *ret_count = 0;
            return ERR_OK;
        }
        data_repr& data = get_repr(self);
        uint32_t count = 0;
        player_id cell_player;
        if (data.global_target_x >= 0 && data.global_target_y >= 0) {
            // only give moves for the target local board
            for (int ly = data.global_target_y * 3; ly < data.global_target_y * 3 + 3; ly++) {
                for (int lx = data.global_target_x * 3; lx < data.global_target_x * 3 + 3; lx++) {
                    get_cell_local(self, lx, ly, &cell_player);
                    if (cell_player == PLAYER_NONE) {
                        moves[count++] = (ly << 4) | lx;
                    }
                }
            }
        } else {
            for (int gy = 0; gy < 3; gy++) {
                for (int gx = 0; gx < 3; gx++) {
                    get_cell_global(self, gx, gy, &cell_player);
                    if (cell_player == PLAYER_NONE) {
                        for (int ly = gy * 3; ly < gy * 3 + 3; ly++) {
                            for (int lx = gx * 3; lx < gx * 3 + 3; lx++) {
                                get_cell_local(self, lx, ly, &cell_player);
                                if (cell_player == PLAYER_NONE) {
                                    moves[count++] = (ly << 4) | lx;
                                }
                            }
                        }
                    }
                }
            }
        }
        *ret_count = count;
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
        int x = move & 0b1111;
        int y = (move >> 4) & 0b1111;
        if (x > 8 || y > 8) {
            return ERR_INVALID_INPUT;
        }
        player_id cell_player;
        get_cell_local(self, x, y, &cell_player);
        if (cell_player != PLAYER_NONE) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
        // check if we're playing into the global target, if any
        if (data.global_target_x >= 0 && data.global_target_y >= 0) {
            if ((x / 3 != data.global_target_x) || (y / 3 != data.global_target_y)) {
                return ERR_INVALID_INPUT;
            }
        }
        return ERR_OK;
    }

    error_code make_move(game* self, player_id player, move_code move)
    {
        data_repr& data = get_repr(self);
        // calc move and set cell
        int x = move & 0b1111;
        int y = (move >> 4) & 0b1111;
        set_cell_local(self, x, y, data.current_player);
        int gx = x / 3;
        int gy = y / 3;
        int lx = x % 3;
        int ly = y % 3;
        // calculate possible result of local board
        uint8_t local_result;
        check_result(self, data.board[gy][gx], &local_result);
        if (local_result > 0) {
            set_cell_global(self, gx, gy, local_result);
        }
        // set global target if applicable
        player_id cell_player;
        get_cell_global(self, lx, ly, &cell_player);
        if (cell_player == PLAYER_NONE) {
            data.global_target_x = lx;
            data.global_target_y = ly;
        } else {
            data.global_target_x = -1;
            data.global_target_y = -1;
        }
        if (local_result == 0) {
            // if no local change happened, do not check global win, switch player
            data.current_player = (data.current_player == 1) ? 2 : 1;
            return ERR_OK;
        }
        // detect win for current player
        uint8_t global_result;
        check_result(self, data.global_board, &global_result);
        if (global_result > 0) {
            data.winning_player = global_result;
            data.current_player = 0;
            return ERR_OK;
        }
        // switch player
        data.current_player = (data.current_player == 1) ? 2 : 1;
        return ERR_OK;
    }

    error_code get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = get_repr(self);
        player_id result = data.winning_player;
        if (result == PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        players[0] = result;
        return ERR_OK;
    }

    error_code id(game* self, uint64_t* ret_id)
    {
        data_repr& data = get_repr(self);
        uint32_t r_id = squirrelnoise5((int32_t)data.global_board, data.global_target_x + data.global_target_y + data.current_player);
        for (int i = 0; i < 9; i++) {
            r_id = squirrelnoise5((int32_t)data.board[i / 3][i % 3], r_id);
        }
        *ret_id = ((uint64_t)r_id << 32) | (uint64_t)squirrelnoise5(r_id, r_id);
        return ERR_OK;
    }

    error_code playout(game* self, uint64_t seed)
    {
        fast_prng rng;
        fprng_srand(&rng, seed);
        move_code moves[81];
        uint32_t moves_count;
        player_id ptm;
        uint8_t ptm_count;
        players_to_move(self, &ptm_count, &ptm);
        while (ptm_count > 0) {
            get_concrete_moves(self, ptm, &moves_count, moves);
            make_move(self, ptm, moves[fprng_rand(&rng) % moves_count]);
            players_to_move(self, &ptm_count, &ptm);
        }
        return ERR_OK;
    }

    error_code get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        if (strlen(str) >= 1 && str[0] == '-') {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        if (strlen(str) != 2) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        move_code move_id = 0;
        int x = (str[0] - 'a');
        int y = (str[1] - '0');
        if (x < 0 || x > 8 || y < 0 || y > 8) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        move_id |= x;
        move_id |= (y << 4);
        *ret_move = move_id;
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
        int x = move & 0b1111;
        int y = (move >> 4) & 0b1111;
        *ret_size = sprintf(str_buf, "%c%c", 'a' + x, '0' + y);
        return ERR_OK;
    }

    error_code print(game* self, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
        player_id cell_player;
        char global_target_str[3];
        size_t global_target_str_len;
        get_move_str(self, PLAYER_NONE, (data.global_target_y << 4) | data.global_target_x, &global_target_str_len, global_target_str);
        str_buf += sprintf(str_buf, "global target: %s\n", (data.global_target_x >= 0 && data.global_target_y >= 0) ? global_target_str : "-");
        for (int gy = 2; gy >= 0; gy--) {
            for (int ly = 2; ly >= 0; ly--) {
                str_buf += sprintf(str_buf, "%c ", '0' + (gy * 3 + ly));
                for (int gx = 0; gx < 3; gx++) {
                    for (int lx = 0; lx < 3; lx++) {
                        get_cell_global(self, gx, gy, &cell_player);
                        if (cell_player == 0) {
                            get_cell_local(self, gx * 3 + lx, gy * 3 + ly, &cell_player);
                        }
                        switch (cell_player) {
                            case (1): {
                                str_buf += sprintf(str_buf, "X");
                            } break;
                            case (2): {
                                str_buf += sprintf(str_buf, "O");
                            } break;
                            case (3): {
                                str_buf += sprintf(str_buf, "=");
                            } break;
                            default: {
                                str_buf += sprintf(str_buf, ".");
                            } break;
                        }
                    }
                    str_buf += sprintf(str_buf, " ");
                }
                str_buf += sprintf(str_buf, "\n");
            }
            if (gy > 0) {
                str_buf += sprintf(str_buf, "\n");
            }
        }
        str_buf += sprintf(str_buf, "  ");
        for (int gx = 0; gx < 3; gx++) {
            for (int lx = 0; lx < 3; lx++) {
                str_buf += sprintf(str_buf, "%c", 'a' + (gx * 3 + lx));
            }
            str_buf += sprintf(str_buf, " ");
        }
        str_buf += sprintf(str_buf, "\n");
        return ERR_OK;
    }

    //=====
    // game internal methods

    error_code check_result(game* self, uint32_t state, player_id* ret_p)
    {
        // return 0 if game running, 1-2 for player wins, 3 for draw
        // detect win for current player
        uint8_t state_winner = PLAYER_NONE;
        bool win = false;
        for (int i = 0; i < 3; i++) {
            player_id cell_player_0i;
            player_id cell_player_1i;
            player_id cell_player_2i;
            player_id cell_player_i0;
            player_id cell_player_i1;
            player_id cell_player_i2;
            get_cell(self, state, 0, i, &cell_player_0i);
            get_cell(self, state, 1, i, &cell_player_1i);
            get_cell(self, state, 2, i, &cell_player_2i);
            get_cell(self, state, i, 0, &cell_player_i0);
            get_cell(self, state, i, 1, &cell_player_i1);
            get_cell(self, state, i, 2, &cell_player_i2);
            if (cell_player_0i == cell_player_1i && cell_player_0i == cell_player_2i && cell_player_0i != 0 || cell_player_i0 == cell_player_i1 && cell_player_i0 == cell_player_i2 && cell_player_i0 != 0) {
                win = true;
                get_cell(self, state, i, i, &state_winner);
            }
        }
        player_id cell_player_00;
        player_id cell_player_11;
        player_id cell_player_22;
        player_id cell_player_02;
        player_id cell_player_20;
        get_cell(self, state, 0, 0, &cell_player_00);
        get_cell(self, state, 1, 1, &cell_player_11);
        get_cell(self, state, 2, 2, &cell_player_22);
        get_cell(self, state, 0, 2, &cell_player_02);
        get_cell(self, state, 2, 0, &cell_player_20);
        if ((cell_player_00 == cell_player_11 && cell_player_00 == cell_player_22 || cell_player_02 == cell_player_11 && cell_player_02 == cell_player_20) && cell_player_11 != 0) {
            win = true;
            get_cell(self, state, 1, 1, &state_winner);
        }
        if (win) {
            *ret_p = state_winner;
            return ERR_OK;
        }
        // detect draw
        bool draw = true;
        for (int i = 0; i < 18; i += 2) {
            if (((state >> i) & 0b11) == 0) {
                draw = false;
                break;
            }
        }
        if (draw) {
            *ret_p = 3;
            return ERR_OK;
        }
        *ret_p = 0;
        return ERR_OK;
    }

    error_code get_cell(game* self, uint32_t state, int x, int y, player_id* ret_p)
    {
        // shift over the correct 2 bits representing the player at that position
        *ret_p = ((state >> (y * 6 + x * 2)) & 0b11);
        return ERR_OK;
    }

    error_code set_cell(game* self, uint32_t* state, int x, int y, player_id p)
    {
        player_id pc;
        get_cell(self, *state, x, y, &pc);
        int offset = (y * 6 + x * 2);
        // new_state = current_value xor (current_value xor new_value)
        *state ^= ((((uint32_t)pc) << offset) ^ (((uint32_t)p) << offset));
        return ERR_OK;
    }

    error_code get_cell_global(game* self, int x, int y, player_id* ret_p)
    {
        data_repr& data = get_repr(self);
        get_cell(self, data.global_board, x, y, ret_p);
        return ERR_OK;
    }

    error_code set_cell_global(game* self, int x, int y, player_id p)
    {
        data_repr& data = get_repr(self);
        set_cell(self, &(data.global_board), x, y, p);
        return ERR_OK;
    }

    error_code get_cell_local(game* self, int x, int y, player_id* ret_p)
    {
        data_repr& data = get_repr(self);
        get_cell(self, data.board[y / 3][x / 3], x % 3, y % 3, ret_p);
        return ERR_OK;
    }

    error_code set_cell_local(game* self, int x, int y, player_id p)
    {
        data_repr& data = get_repr(self);
        set_cell(self, &(data.board[y / 3][x / 3]), x % 3, y % 3, p);
        return ERR_OK;
    }

    error_code get_global_target(game* self, uint8_t* ret)
    {
        data_repr& data = get_repr(self);
        *ret = ((data.global_target_y == -1 ? 3 : data.global_target_y) << 2) | (data.global_target_x == -1 ? 3 : data.global_target_x);
        return ERR_OK;
    }

    error_code set_global_target(game* self, int x, int y)
    {
        data_repr& data = get_repr(self);
        data.global_target_x = x;
        data.global_target_y = y;
        return ERR_OK;
    }

    error_code set_current_player(game* self, player_id p)
    {
        data_repr& data = get_repr(self);
        data.current_player = p;
        return ERR_OK;
    }

    error_code set_result(game* self, player_id p)
    {
        data_repr& data = get_repr(self);
        data.winning_player = p;
        return ERR_OK;
    }

} // namespace

static const tictactoe_ultimate_internal_methods tictactoe_ultimate_gbe_internal_methods{
    .check_result = check_result,
    .get_cell = get_cell,
    .set_cell = set_cell,
    .get_cell_global = get_cell_global,
    .set_cell_global = set_cell_global,
    .get_cell_local = get_cell_local,
    .set_cell_local = set_cell_local,
    .get_global_target = get_global_target,
    .set_global_target = set_global_target,
    .set_current_player = set_current_player,
    .set_result = set_result,
};

const game_methods tictactoe_ultimate_gbe{

    .game_name = "TicTacToe",
    .variant_name = "Ultimate",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .error_strings = false,
        .options = false,
        .options_bin = false,
        .options_bin_ref = false,
        .serializable = false,
        .legacy = false,
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = false,
        .sync_counter = false,
        .move_ordering = false,
        .scores = false,
        .id = true,
        .eval = false,
        .playout = true,
        .print = true,
    },
    .internal_methods = (void*)&tictactoe_ultimate_gbe_internal_methods,

#include "surena/game_impl.h"

};

//TODO fix X/O enum, same as tictactoe_standard
