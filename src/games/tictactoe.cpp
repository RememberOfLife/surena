#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "surena/util/fast_prng.h"
#include "surena/util/noise.h"
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/tictactoe.h"

namespace {

    // general purpose helpers for opts, data

    struct data_repr {
        /*
        board as:
        789
        456
        123
        state bits (MSB<->LSB): ... RR CC 998877 665544 332211
        where RR is results and CC is current player
        */
        uint32_t state;
    };

    data_repr& get_repr(game* self)
    {
        return *((data_repr*)(self->data1));
    }

    // forward declare everything to allow for inlining at least in this unit
    GF_UNUSED(get_last_error);
    error_code create(game* self, game_init init_info);
    error_code destroy(game* self);
    error_code clone(game* self, game* clone_target);
    error_code copy_from(game* self, game* other);
    error_code compare(game* self, game* other, bool* ret_equal);
    GF_UNUSED(export_options);
    error_code export_state(game* self, size_t* ret_size, char* str);
    error_code import_state(game* self, const char* str);
    GF_UNUSED(serialize);
    error_code players_to_move(game* self, uint8_t* ret_count, player_id* players);
    error_code get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    GF_UNUSED(get_concrete_move_probabilities);
    GF_UNUSED(get_concrete_moves_ordered);
    GF_UNUSED(get_actions);
    error_code is_legal_move(game* self, player_id player, move_code move);
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

    error_code get_cell(game* self, int x, int y, player_id* p);
    error_code set_cell(game* self, int x, int y, player_id p);
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
            .state_str = 16,
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = 9,
            .max_results = 1,
            .move_str = 3,
            .print_str = 13,
        };
        const char* initial_state = NULL;
        if (init_info.source_type == GAME_INIT_SOURCE_TYPE_STANDARD) {
            initial_state = init_info.source.standard.state;
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
        // save to diy tictactoe format, somewhat like chess fen
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        const char* ostr = str;
        // save board
        player_id cell_player;
        for (int y = 2; y >= 0; y--) {
            int empty_squares = 0;
            for (int x = 0; x < 3; x++) {
                get_cell(self, x, y, &cell_player);
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
                    set_cell(self, x++, y, 1);
                } break;
                case 'O': {
                    if (x > 2 || y < 0) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    set_cell(self, x++, y, 2);
                } break;
                case '1':
                case '2':
                case '3': { // empty squares
                    for (int place_empty = (*str) - '0'; place_empty > 0; place_empty--) {
                        if (x > 2) {
                            // out of bounds board
                            return ERR_INVALID_INPUT;
                        }
                        set_cell(self, x++, y, PLAYER_NONE);
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
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = get_repr(self);
        player_id ptm = (data.state >> 18) & 0b11;
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
        uint32_t count = 0;
        player_id cell_player;
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                get_cell(self, x, y, &cell_player);
                if (cell_player == PLAYER_NONE) {
                    moves[count++] = (y << 2) | x;
                }
            }
        }
        *ret_count = count;
        return ERR_OK;
    }

    error_code is_legal_move(game* self, player_id player, move_code move)
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
        int x = move & 0b11;
        int y = (move >> 2) & 0b11;
        if (x > 2 || y > 2) {
            return ERR_INVALID_INPUT;
        }
        player_id cell_player;
        get_cell(self, x, y, &cell_player);
        if (cell_player != PLAYER_NONE) {
            return ERR_INVALID_INPUT;
        }
        return ERR_OK;
    }

    error_code make_move(game* self, player_id player, move_code move)
    {
        data_repr& data = get_repr(self);
        // set move as current player
        int x = move & 0b11;
        int y = (move >> 2) & 0b11;
        int current_player = (data.state >> 18) & 0b11;
        set_cell(self, x, y, current_player);
        // detect win for current player
        bool win = false;
        for (int i = 0; i < 3; i++) {
            player_id cell_player_0i;
            player_id cell_player_1i;
            player_id cell_player_2i;
            player_id cell_player_i0;
            player_id cell_player_i1;
            player_id cell_player_i2;
            get_cell(self, 0, i, &cell_player_0i);
            get_cell(self, 1, i, &cell_player_1i);
            get_cell(self, 2, i, &cell_player_2i);
            get_cell(self, i, 0, &cell_player_i0);
            get_cell(self, i, 1, &cell_player_i1);
            get_cell(self, i, 2, &cell_player_i2);
            if (cell_player_0i == cell_player_1i && cell_player_0i == cell_player_2i && cell_player_0i != 0 || cell_player_i0 == cell_player_i1 && cell_player_i0 == cell_player_i2 && cell_player_i0 != 0) {
                win = true;
            }
        }
        player_id cell_player_00;
        player_id cell_player_11;
        player_id cell_player_22;
        player_id cell_player_02;
        player_id cell_player_20;
        get_cell(self, 0, 0, &cell_player_00);
        get_cell(self, 1, 1, &cell_player_11);
        get_cell(self, 2, 2, &cell_player_22);
        get_cell(self, 0, 2, &cell_player_02);
        get_cell(self, 2, 0, &cell_player_20);
        if ((cell_player_00 == cell_player_11 && cell_player_00 == cell_player_22 || cell_player_02 == cell_player_11 && cell_player_02 == cell_player_20) && cell_player_11 != 0) {
            win = true;
        }
        if (win) {
            // current player has won, mark result as current player and set current player to 0
            data.state &= ~(0b11 << 20); // reset result to 0, otherwise result may be 4 after an internal state update
            data.state |= current_player << 20;
            data.state &= ~(0b11 << 18);
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
            data.state &= ~(0b11 << 18);
            return ERR_OK;
        }
        // switch player
        set_current_player(self, (current_player == 1) ? 2 : 1);
        return ERR_OK;
    }

    error_code get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = get_repr(self);
        player_id result = (player_id)((data.state >> 20) & 0b11);
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
        uint32_t state_noise = squirrelnoise5(data.state, 0);
        *ret_id = ((uint64_t)state_noise << 32) | (uint64_t)squirrelnoise5(state_noise, state_noise);
        return ERR_OK;
    }

    error_code playout(game* self, uint64_t seed)
    {
        fast_prng rng;
        fprng_srand(&rng, seed);
        move_code moves[9];
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
        if (x < 0 || x > 2 || y < 0 || y > 2) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        move_id |= x;
        move_id |= (y << 2);
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
        int x = move & 0b11;
        int y = (move >> 2) & 0b11;
        *ret_size = sprintf(str_buf, "%c%c", 'a' + x, '0' + y);
        return ERR_OK;
    }

    error_code print(game* self, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        player_id cell_player;
        for (int y = 2; y >= 0; y--) {
            for (int x = 0; x < 3; x++) {
                get_cell(self, x, y, &cell_player);
                switch (cell_player) {
                    case 1: {
                        str_buf += sprintf(str_buf, "X");
                    } break;
                    case 2: {
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

    error_code get_cell(game* self, int x, int y, player_id* p)
    {
        data_repr& data = get_repr(self);
        // shift over the correct 2 bits representing the player at that position
        *p = ((data.state >> (y * 6 + x * 2)) & 0b11);
        return ERR_OK;
    }

    error_code set_cell(game* self, int x, int y, player_id p)
    {
        data_repr& data = get_repr(self);
        player_id pc;
        get_cell(self, x, y, &pc);
        int offset = (y * 6 + x * 2);
        // new_state = current_value xor (current_value xor new_value)
        data.state ^= ((((uint32_t)pc) << offset) ^ (((uint32_t)p) << offset));
        return ERR_OK;
    }

    error_code set_current_player(game* self, player_id p)
    {
        data_repr& data = get_repr(self);
        data.state &= ~(0b11 << 18); // reset current player to 0
        data.state |= p << 18; // insert new current player
        return ERR_OK;
    }

    error_code set_result(game* self, player_id p)
    {
        data_repr& data = get_repr(self);
        data.state &= ~(0b11 << 20); // reset result to 0
        data.state |= p << 20; // insert new result
        return ERR_OK;
    }

} // namespace

static const tictactoe_internal_methods tictactoe_gbe_internal_methods{
    .get_cell = get_cell,
    .set_cell = set_cell,
    .set_current_player = set_current_player,
    .set_result = set_result,
};

const game_methods tictactoe_gbe{

    .game_name = "TicTacToe",
    .variant_name = "Standard",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .error_strings = false,
        .options = false,
        .serializable = false,
        .legacy = false,
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = false,
        .move_ordering = false,
        .scores = false,
        .id = true,
        .eval = false,
        .playout = true,
        .print = true,
    },
    .internal_methods = (void*)&tictactoe_gbe_internal_methods,

#include "surena/game_impl.h"

};

//TODO fix X/O enum
