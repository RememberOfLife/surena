#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "surena/util/fast_prng.hpp"
#include "surena/util/noise.hpp"
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/tictactoe.h"

namespace surena {
    
    // game data state representation and general getter

    typedef struct data_repr {
        /*
        board as:
        789
        456
        123
        state bits (MSB<->LSB): ... RR CC 998877 665544 332211
        where RR is results and CC is current player
        */
        uint32_t state;
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

    static error_code _get_cell(game* self, int x, int y, player_id* p);
    static error_code _set_cell(game* self, int x, int y, player_id p);
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
        self->data = NULL;
        return ERR_OK;
    }

    static error_code _clone(game* self, game* clone_target)
    {
        if (clone_target == NULL) {
            return ERR_INVALID_INPUT;
        }
        *clone_target = *self;
        error_code ec = clone_target->methods->create(clone_target);
        if (ec != ERR_OK) {
            return ec;
        }
        memcpy(clone_target->data, self->data, sizeof(data_repr));
        return ERR_OK;
    }
    
    static error_code _copy_from(game* self, game* other)
    {
        self->sync_ctr = other->sync_ctr;
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
        if (str == NULL) {
            data.state = 1 << 18; // player one starts
            return ERR_OK;
        }
        data.state = 0;
        // load from diy tictactoe format, somewhat like chess fen, "board p_cur p_res"
        int y = 2;
        int x = 0;
        // get square fillings
        bool advance_segment = false;
        while (!advance_segment) {
            switch (*str) {
                case 'X': {
                    if (x > 2 || y < 0) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    _set_cell(self, x++, y, 1);
                } break;
                case 'O': {
                    if (x > 2 || y < 0) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    _set_cell(self, x++, y, 2);
                } break;
                case '1':
                case '2':
                case '3': { // empty squares
                    for (int place_empty = (*str)-'0'; place_empty > 0; place_empty--) {
                        if (x > 2) {
                            // out of bounds board
                            return ERR_INVALID_INPUT;
                        }
                        _set_cell(self, x++, y, PLAYER_NONE);
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
        // save to diy tictactoe format, somewhat like chess fen
        if (str == NULL) {
            return 16; // max 15 + 1 zero terminator byte
        }
        const char* ostr = str;
        // save board
        player_id cell_player;
        for (int y = 2; y >= 0; y--) {
            int empty_squares = 0;
            for (int x = 0; x < 3; x++) {
                _get_cell(self, x, y, &cell_player);
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
        player_id ptm = (data.state >> 18) & 0b11;
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
            *ret_count = 9;
            return ERR_OK;
        }
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        if (ptm_count == 0 || player != ptm) {
            *ret_count = 0;
            return ERR_OK;
        }
        uint32_t count = 0;
        player_id cell_player;
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                _get_cell(self, x, y, &cell_player);
                if (cell_player == PLAYER_NONE) {
                    moves[count++] = (y << 2) | x;
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
        int x = move & 0b11;
        int y = (move >> 2) & 0b11;
        if (x > 2 || y > 2) {
            return ERR_INVALID_INPUT;
        }
        player_id cell_player;
        _get_cell(self, x, y, &cell_player);
        if (cell_player != PLAYER_NONE) {
            return ERR_INVALID_INPUT;
        }
        return ERR_OK;
    }

    static error_code _make_move(game* self, player_id player, move_code move)
    {
        self->sync_ctr++;
        data_repr& data = _get_repr(self);
        // set move as current player
        int x = move & 0b11;
        int y = (move >> 2) & 0b11;
        int current_player = (data.state >> 18) & 0b11;
        _set_cell(self, x, y, current_player);
        // detect win for current player
        bool win = false;
        for (int i = 0; i < 3; i++) {
            player_id cell_player_0i;
            player_id cell_player_1i;
            player_id cell_player_2i;
            player_id cell_player_i0;
            player_id cell_player_i1;
            player_id cell_player_i2;
            _get_cell(self, 0, i, &cell_player_0i);
            _get_cell(self, 1, i, &cell_player_1i);
            _get_cell(self, 2, i, &cell_player_2i);
            _get_cell(self, i, 0, &cell_player_i0);
            _get_cell(self, i, 1, &cell_player_i1);
            _get_cell(self, i, 2, &cell_player_i2);
            if (cell_player_0i == cell_player_1i && cell_player_0i == cell_player_2i && cell_player_0i != 0
                || cell_player_i0 == cell_player_i1 && cell_player_i0 == cell_player_i2 && cell_player_i0 != 0) {
                win = true;
            }
        }
        player_id cell_player_00;
        player_id cell_player_11;
        player_id cell_player_22;
        player_id cell_player_02;
        player_id cell_player_20;
        _get_cell(self, 0, 0, &cell_player_00);
        _get_cell(self, 1, 1, &cell_player_11);
        _get_cell(self, 2, 2, &cell_player_22);
        _get_cell(self, 0, 2, &cell_player_02);
        _get_cell(self, 2, 0, &cell_player_20);
        if ((cell_player_00 == cell_player_11 && cell_player_00 == cell_player_22
            || cell_player_02 == cell_player_11 && cell_player_02 == cell_player_20) && cell_player_11 != 0) {
            win = true;
        }
        if (win) {
            // current player has won, mark result as current player and set current player to 0
            data.state &= ~(0b11<<20); // reset result to 0, otherwise result may be 4 after an internal state update
            data.state |= current_player << 20;
            data.state &= ~(0b11<<18);
            return ERR_OK;
        }
        // detect draw
        bool draw = true;
        for (int i = 0; i < 18; i += 2) {
            if (((data.state >> i) & 0b11) == 0) {
                draw = false;
                break;
            }
        }
        if (draw) {
            // set current player to 0, result is 0 already
            data.state &= ~(0b11<<18);
            return ERR_OK;
        }
        // switch player
        _set_current_player(self, (current_player == 1) ? 2 : 1);
        return ERR_OK;
    }

    static error_code _get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        *ret_count = 1;
        if (players == NULL) {
            return ERR_OK;
        }
        data_repr& data = _get_repr(self);
        player_id result = (player_id)((data.state >> 20) & 0b11);
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
        uint32_t state_noise = squirrelnoise5(data.state, 0);
        *ret_id = ((uint64_t)state_noise << 32) | (uint64_t)squirrelnoise5(state_noise, state_noise);
        return ERR_OK;
    }

    static error_code _playout(game* self, uint64_t seed)
    {
        fast_prng rng(seed);
        move_code moves[9];
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
        if (x < 0 || x > 2 || y < 0 || y > 2) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        move_id |= x;
        move_id |= (y << 2);
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
        int x = move & 0b11;
        int y = (move >> 2) & 0b11;
        *ret_size = sprintf(str_buf, "%c%c", 'a' + x, '0' + y);
        return ERR_OK;
    }

    static error_code _debug_print(game* self, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            *ret_size = 13;
            return ERR_OK;
        }
        player_id cell_player;
        for (int y = 2; y >= 0; y--) {
            for (int x = 0; x < 3; x++) {
                _get_cell(self, x, y, &cell_player);
                switch (cell_player) {
                    case 1: {
                        str_buf += sprintf(str_buf, "X");
                    } break;
                    case 2:{ 
                        str_buf += sprintf(str_buf, "O");
                    } break;
                    default: {
                        str_buf += sprintf(str_buf, ".");
                    } break;
                }
            }
            str_buf += sprintf(str_buf, "\n");
        }
        return ERR_OK;
    }

    //=====
    // game internal methods
    
    static error_code _get_cell(game* self, int x, int y, player_id* p)
    {
        data_repr& data = _get_repr(self);
        // shift over the correct 2 bits representing the player at that position
        *p = ((data.state >> (y*6+x*2)) & 0b11);
        return ERR_OK;
    }

    static error_code _set_cell(game* self, int x, int y, player_id p)
    {
        data_repr& data = _get_repr(self);   
        player_id pc;
        _get_cell(self, x, y, &pc);
        int offset = (y*6+x*2);
        // new_state = current_value xor (current_value xor new_value)
        data.state ^= ((((uint32_t)pc) << offset) ^ (((uint32_t)p) << offset));
        return ERR_OK;
    }
    
    static error_code _set_current_player(game* self, player_id p)
    {
        data_repr& data = _get_repr(self);   
        data.state &= ~(0b11<<18); // reset current player to 0
        data.state |= p<<18; // insert new current player
        return ERR_OK;
    }
    
    static error_code _set_result(game* self, player_id p)
    {
        data_repr& data = _get_repr(self);        
        data.state &= ~(0b11<<20); // reset result to 0
        data.state |= p<<20; // insert new result
        return ERR_OK;
    }

}

static const tictactoe_internal_methods tictactoe_gbe_internal_methods{
    .get_cell = surena::_get_cell,
    .set_cell = surena::_set_cell,
    .set_current_player = surena::_set_current_player,
    .set_result = surena::_set_result,
};

const game_methods tictactoe_gbe{

    .game_name = "TicTacToe",
    .variant_name = "Standard",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = false,
    },
    .internal_methods = (void*)&tictactoe_gbe_internal_methods,
    
    #include "surena/game_impl.h"
    
};

//TODO fix X/O enum
