#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "surena/util/fast_prng.hpp"
#include "surena/util/noise.hpp"
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/tictactoe_ultimate.h"

namespace surena {
    
    // game data state representation and general getter

    typedef struct data_repr {
        // origin bottom left, y upwards, x to the right
        // individual boards work like standard tictactoe board
        uint32_t board[3][3];
        uint32_t global_board; // 0 is ongoing, 3 is draw result
        // global target is the local board that the current player has to play to
        int8_t global_target_x;
        int8_t global_target_y;
        player_id current_player;
        player_id winning_player;
    } data_repr;

    static data_repr& _get_repr(game* self)
    {
        return *((data_repr*)(self->data));
    }

    // forward declare everything to allow for inlining at least in this unit
    static const char* _get_error_string(error_code err);
    GF_UNUSED(import_options_bin);
    GF_UNUSED(import_options_str);
    GF_UNUSED(export_options_str);
    static error_code _create(game* self);
    static error_code _destroy(game* self);
    static error_code _clone(game* self, game* clone_target);
    static error_code _copy_from(game* self, game* other);
    static error_code _compare(game* self, game* other, bool* ret_equal);
    static error_code _import_state(game* self, const char* str);
    static error_code _export_state(game* self, size_t* ret_size, char* str);
    static error_code _get_player_count(game* self, uint8_t* ret_count);
    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players);
    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    GF_UNUSED(get_concrete_move_probabilities);
    GF_UNUSED(get_concrete_moves_ordered);
    GF_UNUSED(get_actions);
    static error_code _is_legal_move(game* self, player_id player, move_code move, uint32_t sync_ctr);
    GF_UNUSED(move_to_action);
    GF_UNUSED(is_action);
    static error_code _make_move(game* self, player_id player, move_code move);
    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players);
    static error_code _id(game* self, uint64_t* ret_id);
    GF_UNUSED(eval);
    GF_UNUSED(discretize);
    static error_code _playout(game* self, uint64_t seed);
    GF_UNUSED(redact_keep_state);
    GF_UNUSED(export_sync_data);
    GF_UNUSED(import_sync_data);
    static error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    static error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf);

    static error_code _check_result(game* self, uint32_t state, player_id* ret_p);
    static error_code _get_cell(game* self, uint32_t state, int x, int y, player_id* ret_p);
    static error_code _set_cell(game* self, uint32_t* state, int x, int y, player_id p);
    static error_code _get_cell_global(game* self, int x, int y, player_id* ret_p);
    static error_code _set_cell_global(game* self, int x, int y, player_id p);
    static error_code _get_cell_local(game* self, int x, int y, player_id* ret_p);
    static error_code _set_cell_local(game* self, int x, int y, player_id p);
    static error_code _get_global_target(game* self, uint8_t* ret);
    static error_code _set_global_target(game* self, int x, int y);
    static error_code _set_current_player(game* self, player_id p);
    static error_code _set_result(game* self, player_id p);

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
        self->data = malloc(sizeof(data_repr));
        if (self->data == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        return ERR_OK;
    }

    static error_code _destroy(game* self)
    {
        free(self->data);
        return ERR_OK;
    }

    static error_code _clone(game* self, game* clone_target)
    {
        //TODO
        game* clone = (game*)malloc(sizeof(game));
        if (clone == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        *clone = *self;
        error_code ec = clone->methods->create(clone);
        if (ec != ERR_OK) {
            return ec;
        }
        memcpy(clone->data, self->data, sizeof(data_repr));
        clone_target = clone;
        return ERR_OK;
    }
    
    static error_code _copy_from(game* self, game* other)
    {
        *self = *other;
        memcpy(self->data, other->data, sizeof(data_repr));
        return ERR_OK;
    }

    static error_code _compare(game* self, game* other, bool* ret_equal)
    {
        *ret_equal = (self->sync_ctr == other->sync_ctr) && (memcmp(self->data, other->data, sizeof(data_repr)) == 0);
        return ERR_OK;
    }

    static error_code _import_state(game* self, const char* str)
    {
        data_repr& data = _get_repr(self);
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
                    _set_cell_local(self, x++, y, 1);
                } break;
                case 'O': {
                    if (x > 8 || y < 0) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    _set_cell_local(self, x++, y, 2);
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
                    for (int place_empty = (*str)-'0'; place_empty > 0; place_empty--) {
                        if (x > 8) {
                            // out of bounds board
                            return ERR_INVALID_INPUT;
                        }
                        _set_cell_local(self, x++, y, PLAYER_NONE);
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
                _check_result(self, data.board[iy][ix], &local_result);
                if (local_result > 0) {
                    _set_cell_global(self, ix, iy, local_result);
                }
            }
        }
        // get global target, if any, otherwise its reset already
        if (*str != '-') {
            data.global_target_x = (*str)-'a';
            str++;
            data.global_target_y = (*str)-'1';
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
                _set_current_player(self, PLAYER_NONE);
            } break;
            case 'X': {
                _set_current_player(self, 1);
            } break;
            case 'O': {
                _set_current_player(self, 2);
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
                _set_result(self, PLAYER_NONE);
            } break;
            case 'X': {
                _set_result(self, 1);
            } break;
            case 'O': {
                _set_result(self, 2);
            } break;
            default: {
                // failure, ran out of str to use or got invalid character
                return ERR_INVALID_INPUT;
            } break;
        }
        return ERR_OK;
    }

    static error_code _export_state(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            *ret_size = 37; // max 36 + 1 zero terminator byte
            return ERR_OK;
        }
        data_repr& data = _get_repr(self);
        const char* ostr = str;
        player_id cell_player;
        for (int y = 8; y >= 0; y--) {
            int empty_squares = 0;
            for (int x = 0; x < 9; x++) {
                _get_cell_local(self, x, y, &cell_player);
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
            str += sprintf(str, " %c%c", 'a'+data.global_target_x, '0'+data.global_target_y);
        } else {
            str += sprintf(str, " -");
        }
        // current player
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
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
        _get_results(self, &res_count, &res);
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
        *ret_size = str-ostr;
        return ERR_OK;
    }

    static error_code _get_player_count(game* self, uint8_t* ret_count)
    {
        *ret_count = 2;
        return ERR_OK;
    }

    static error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        *ret_count = 1;
        if (players == NULL) {
            return ERR_OK;
        }
        data_repr& data = _get_repr(self);
        player_id ptm = data.current_player;
        if (ptm == PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = ptm;
        return ERR_OK;
    }

    static error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        if (moves == NULL) {
            *ret_count = 81;
            return ERR_OK;
        }
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        if (ptm_count == 0 || player != ptm) {
            *ret_count = 0;
            return ERR_OK;
        }
        data_repr& data = _get_repr(self);
        uint32_t count = 0;
        player_id cell_player;
        if (data.global_target_x >= 0 && data.global_target_y >= 0) {
            // only give moves for the target local board
            for (int ly = data.global_target_y*3; ly < data.global_target_y*3+3; ly++) {
                for (int lx = data.global_target_x*3; lx < data.global_target_x*3+3; lx++) {
                    _get_cell_local(self, lx, ly, &cell_player);
                    if (cell_player == PLAYER_NONE) {
                        moves[count++] = (ly << 4) | lx;
                    }
                }
            }
        } else {
            for (int gy = 0; gy < 3; gy++) {
                for (int gx = 0; gx < 3; gx++) {
                    _get_cell_global(self, gx, gy, &cell_player);
                    if (cell_player == PLAYER_NONE) {
                        for (int ly = gy*3; ly < gy*3+3; ly++) {
                            for (int lx = gx*3; lx < gx*3+3; lx++) {
                                _get_cell_local(self, lx, ly, &cell_player);
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

    static error_code _is_legal_move(game* self, player_id player, move_code move, uint32_t sync_ctr)
    {
        if (self->sync_ctr != sync_ctr) {
            return ERR_SYNC_CTR_MISMATCH;
        }
        if (move == MOVE_NONE) {
            return ERR_INVALID_INPUT;
        }
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        if (ptm != player) {
            return ERR_INVALID_INPUT;
        }
        int x = move & 0b1111;
        int y = (move >> 4) & 0b1111;
        if (x > 8 || y > 8) {
            return ERR_INVALID_INPUT;
        }
        player_id cell_player;
        _get_cell_local(self, x, y, &cell_player);
        if (cell_player != PLAYER_NONE) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = _get_repr(self);
        // check if we're playing into the global target, if any
        if (data.global_target_x >= 0 && data.global_target_y >= 0) {
            if ((x / 3 != data.global_target_x) || (y / 3 != data.global_target_y)) {
                return ERR_INVALID_INPUT;
            }
        }
        return ERR_OK;
    }

    static error_code _make_move(game* self, player_id player, move_code move)
    {
        data_repr& data = _get_repr(self);
        // calc move and set cell
        int x = move & 0b1111;
        int y = (move >> 4) & 0b1111;
        _set_cell_local(self, x, y, data.current_player);
        int gx = x / 3;
        int gy = y / 3;
        int lx = x % 3;
        int ly = y % 3;
        // calculate possible result of local board
        uint8_t local_result;
        _check_result(self, data.board[gy][gx], &local_result);
        if (local_result > 0) {
            _set_cell_global(self, gx, gy, local_result);
        }
        // set global target if applicable
        player_id cell_player;
        _get_cell_global(self, lx, ly, &cell_player);
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
        _check_result(self, data.global_board, &global_result);
        if (global_result > 0) {
            data.winning_player = global_result;
            data.current_player = 0;
            return ERR_OK;
        }
        // switch player
        data.current_player = (data.current_player == 1) ? 2 : 1;
        return ERR_OK;
    }

    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        *ret_count = 1;
        if (players == NULL) {
            return ERR_OK;
        }
        data_repr& data = _get_repr(self);
        player_id result = data.winning_player;
        if (result == PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        players[0] = result;
        return ERR_OK;
    }

    static error_code _id(game* self, uint64_t* ret_id)
    {
        data_repr& data = _get_repr(self);
        uint32_t r_id = squirrelnoise5((int32_t)data.global_board, data.global_target_x + data.global_target_y + data.current_player);
        for (int i = 0; i < 9; i++) {
            r_id = squirrelnoise5((int32_t)data.board[i / 3][i % 3], r_id);
        }
        *ret_id = ((uint64_t)r_id << 32) | (uint64_t)squirrelnoise5(r_id, r_id);
        return ERR_OK;
    }

    static error_code _playout(game* self, uint64_t seed)
    {
        data_repr& data = _get_repr(self);
        fast_prng rng(seed);
        move_code moves[81];
        uint32_t moves_count;
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        while (ptm_count > 0) {
            _get_concrete_moves(self, ptm, &moves_count, moves);
            _make_move(self, ptm ,moves[rng.rand()%moves_count]);
            _players_to_move(self, &ptm_count, &ptm);
        }
        return ERR_OK;
    }

    static error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
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
        int x = (str[0]-'a');
        int y = (str[1]-'0');
        if (x < 0 || x > 8 || y < 0 || y > 8) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        move_id |= x;
        move_id |= (y << 4);
        *ret_move = move_id;
        return ERR_OK;
    }

    static error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            *ret_size = 3;
            return ERR_OK;
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

    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            *ret_size = 180; // slightly oversized
            return ERR_OK;
        }
        data_repr& data = _get_repr(self);
        player_id cell_player;
        char global_target_str[3];
        size_t global_target_str_len;
        _get_move_str(self, PLAYER_NONE, (data.global_target_y << 4) | data.global_target_x, &global_target_str_len, global_target_str);
        str_buf += sprintf(str_buf, "global target: %s\n", (data.global_target_x >= 0 && data.global_target_y >= 0) ? global_target_str : "-");
        for (int gy = 2; gy >= 0; gy--) {
            for (int ly = 2; ly >= 0; ly--) {
                str_buf += sprintf(str_buf, "%c ", '0'+(gy*3+ly));
                for (int gx = 0; gx < 3; gx++) {
                    for (int lx = 0; lx < 3; lx++) {
                        _get_cell_global(self, gx, gy, &cell_player);
                        if (cell_player == 0) {
                            _get_cell_local(self, gx*3+lx, gy*3+ly, &cell_player);
                        }
                        switch (cell_player) {
                            case (1): {
                                str_buf += sprintf(str_buf, "X");
                            } break;
                            case (2):{ 
                                str_buf += sprintf(str_buf, "O");
                            } break;
                            case (3):{ 
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
                str_buf += sprintf(str_buf, "%c", 'a'+(gx*3+lx));
            }
            str_buf += sprintf(str_buf, " ");
        }
        str_buf += sprintf(str_buf, "\n");
        return ERR_OK;
    }

    //=====
    // game internal methods
    
    static error_code _check_result(game* self, uint32_t state, player_id* ret_p)
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
            _get_cell(self, state, 0, i, &cell_player_0i);
            _get_cell(self, state, 1, i, &cell_player_1i);
            _get_cell(self, state, 2, i, &cell_player_2i);
            _get_cell(self, state, i, 0, &cell_player_i0);
            _get_cell(self, state, i, 1, &cell_player_i1);
            _get_cell(self, state, i, 2, &cell_player_i2);
            if (cell_player_0i == cell_player_1i && cell_player_0i == cell_player_2i && cell_player_0i != 0
                || cell_player_i0 == cell_player_i1 && cell_player_i0 == cell_player_i2 && cell_player_i0 != 0) {
                win = true;
                _get_cell(self, state, i, i, &state_winner);
            }
        }
        player_id cell_player_00;
        player_id cell_player_11;
        player_id cell_player_22;
        player_id cell_player_02;
        player_id cell_player_20;
        _get_cell(self, state, 0, 0, &cell_player_00);
        _get_cell(self, state, 1, 1, &cell_player_11);
        _get_cell(self, state, 2, 2, &cell_player_22);
        _get_cell(self, state, 0, 2, &cell_player_02);
        _get_cell(self, state, 2, 0, &cell_player_20);
        if ((cell_player_00 == cell_player_11 && cell_player_00 == cell_player_22
            || cell_player_02 == cell_player_11 && cell_player_02 == cell_player_20) && cell_player_11 != 0) {
            win = true;
            _get_cell(self, state, 1, 1, &state_winner);
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

    static error_code _get_cell(game* self, uint32_t state, int x, int y, player_id* ret_p)
    {        
        // shift over the correct 2 bits representing the player at that position
        *ret_p = ((state >> (y*6+x*2)) & 0b11);
        return ERR_OK;
    }
    
    static error_code _set_cell(game* self, uint32_t* state, int x, int y, player_id p)
    {
        player_id pc;
        _get_cell(self, *state, x, y, &pc);
        int offset = (y*6+x*2);
        // new_state = current_value xor (current_value xor new_value)
        *state ^= ((((uint32_t)pc) << offset) ^ (((uint32_t)p) << offset));
        return ERR_OK;
    }
    
    static error_code _get_cell_global(game* self, int x, int y, player_id* ret_p)
    {
        data_repr& data = _get_repr(self);
        _get_cell(self, data.global_board, x, y, ret_p);
        return ERR_OK;
    }
    
    static error_code _set_cell_global(game* self, int x, int y, player_id p)
    {
        data_repr& data = _get_repr(self);
        _set_cell(self, &(data.global_board), x, y, p);
        return ERR_OK;
    }
    
    static error_code _get_cell_local(game* self, int x, int y, player_id* ret_p)
    {
        data_repr& data = _get_repr(self);
        _get_cell(self, data.board[y/3][x/3], x%3, y%3, ret_p);
        return ERR_OK;
    }
    
    static error_code _set_cell_local(game* self, int x, int y, player_id p)
    {
        data_repr& data = _get_repr(self);
        _set_cell(self, &(data.board[y / 3][x / 3]), x % 3, y % 3, p);
        return ERR_OK;
    }
    
    static error_code _get_global_target(game* self, uint8_t* ret)
    {
        data_repr& data = _get_repr(self);
        *ret = ((data.global_target_y == -1 ? 3 : data.global_target_y) << 2) | (data.global_target_x == -1 ? 3 : data.global_target_x);
        return ERR_OK;
    }
    
    static error_code _set_global_target(game* self, int x, int y)
    {
        data_repr& data = _get_repr(self);
        data.global_target_x = x;
        data.global_target_y = y;
        return ERR_OK;
    }
    
    static error_code _set_current_player(game* self, player_id p)
    {
        data_repr& data = _get_repr(self);
        data.current_player = p;
        return ERR_OK;
    }
    
    static error_code _set_result(game* self, player_id p)
    {
        data_repr& data = _get_repr(self);
        data.winning_player = p;
        return ERR_OK;
    }

}

static const tictactoe_ultimate_internal_methods tictactoe_ultimate_gbe_internal_methods{
    .check_result = surena::_check_result,
    .get_cell = surena::_get_cell,
    .set_cell = surena::_set_cell,
    .get_cell_global = surena::_get_cell_global,
    .set_cell_global = surena::_set_cell_global,
    .get_cell_local = surena::_get_cell_local,
    .set_cell_local = surena::_set_cell_local,
    .get_global_target = surena::_get_global_target,
    .set_global_target = surena::_set_global_target,
    .set_current_player = surena::_set_current_player,
    .set_result = surena::_set_result,
};

const game_methods tictactoe_ultimate_gbe{

    .game_name = "TicTacToe",
    .variant_name = "Ultimate",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = false,
    },
    .internal_methods = (void*)&tictactoe_ultimate_gbe_internal_methods,
    
    #include "surena/game_impl.h"
    
};

//TODO fix X/O enum, same as tictactoe_standard
