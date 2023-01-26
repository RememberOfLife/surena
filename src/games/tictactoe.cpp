#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rosalia/rand.h"
#include "rosalia/noise.h"
#include "rosalia/semver.h"

#include "surena/game.h"

#include "surena/games/tictactoe.h"

//TODO move to c?

// general purpose helpers for opts, data, bufs

namespace {

    struct export_buffers {
        char* state;
        player_id* players_to_move;
        move_data* concrete_moves;
        player_id* results;
        move_data_sync move_out;
        char* move_str;
        char* print;
    };

    struct state_repr {
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

    struct game_data {
        export_buffers bufs;
        state_repr state;
    };

    export_buffers& get_bufs(game* self)
    {
        return ((game_data*)(self->data1))->bufs;
    }

    state_repr& get_repr(game* self)
    {
        return ((game_data*)(self->data1))->state;
    }

} // namespace

#ifdef __cplusplus
extern "C" {
#endif

// impl internal declarations
static error_code get_cell_gf(game* self, int x, int y, player_id* p);
static error_code set_cell_gf(game* self, int x, int y, player_id p);
static error_code set_current_player_gf(game* self, player_id p);
static error_code set_result_gf(game* self, player_id p);

// need internal function pointer struct here
static const tictactoe_internal_methods tictactoe_gbe_internal_methods{
    .get_cell = get_cell_gf,
    .set_cell = set_cell_gf,
    .set_current_player = set_current_player_gf,
    .set_result = set_result_gf,
};

// declare and form game
#define SURENA_GDD_BENAME tictactoe_standard_gbe
#define SURENA_GDD_GNAME "TicTacToe"
#define SURENA_GDD_VNAME "Standard"
#define SURENA_GDD_INAME "surena_default"
#define SURENA_GDD_VERSION ((semver){0, 2, 0})
#define SURENA_GDD_INTERNALS &tictactoe_gbe_internal_methods
#define SURENA_GDD_FF_ID
#define SURENA_GDD_FF_PLAYOUT
#define SURENA_GDD_FF_PRINT
#include "surena/game_decldef.h"

// implementation

static error_code create_gf(game* self, game_init* init_info)
{
    self->data1 = malloc(sizeof(game_data));
    if (self->data1 == NULL) {
        return ERR_OUT_OF_MEMORY;
    }
    self->data2 = NULL;
    {
        export_buffers& bufs = get_bufs(self);
        bufs.state = (char*)malloc(16 * sizeof(char));
        bufs.players_to_move = (player_id*)malloc(1 * sizeof(player_id));
        bufs.concrete_moves = (move_data*)malloc(9 * sizeof(move_data));
        bufs.results = (player_id*)malloc(1 * sizeof(char));
        bufs.move_str = (char*)malloc(3 * sizeof(char));
        bufs.print = (char*)malloc(13 * sizeof(char));
        if (bufs.state == NULL ||
            bufs.players_to_move == NULL ||
            bufs.concrete_moves == NULL ||
            bufs.results == NULL ||
            bufs.move_str == NULL ||
            bufs.print == NULL) {
            destroy_gf(self);
            return ERR_OUT_OF_MEMORY;
        }
    }
    const char* initial_state = NULL;
    if (init_info->source_type == GAME_INIT_SOURCE_TYPE_STANDARD) {
        initial_state = init_info->source.standard.state;
    }
    return import_state_gf(self, initial_state);
}

static error_code destroy_gf(game* self)
{
    {
        export_buffers& bufs = get_bufs(self);
        free(bufs.state);
        free(bufs.players_to_move);
        free(bufs.concrete_moves);
        free(bufs.results);
        free(bufs.move_str);
        free(bufs.print);
    }
    free(self->data1);
    self->data1 = NULL;
    return ERR_OK;
}

static error_code clone_gf(game* self, game* clone_target)
{
    clone_target->methods = self->methods;
    game_init init_info = (game_init){.source_type = GAME_INIT_SOURCE_TYPE_DEFAULT};
    error_code ec = create_gf(clone_target, &init_info);
    if (ec != ERR_OK) {
        return ec;
    }
    copy_from_gf(clone_target, self);
    return ERR_OK;
}

static error_code copy_from_gf(game* self, game* other)
{
    get_repr(self) = get_repr(other);
    return ERR_OK;
}

static error_code compare_gf(game* self, game* other, bool* ret_equal)
{
    *ret_equal = (memcmp(&get_repr(self), &get_repr(other), sizeof(state_repr)) == 0);
    return ERR_OK;
}

static error_code player_count_gf(game* self, uint8_t* ret_count)
{
    *ret_count = 2;
    return ERR_OK;
}

static error_code export_state_gf(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    char* outbuf = bufs.state;
    // save to diy tictactoe format, somewhat like chess fen
    if (outbuf == NULL) {
        return ERR_INVALID_INPUT;
    }
    const char* ostr = outbuf;
    // save board
    player_id cell_player;
    for (int y = 2; y >= 0; y--) {
        int empty_squares = 0;
        for (int x = 0; x < 3; x++) {
            get_cell_gf(self, x, y, &cell_player);
            if (cell_player == PLAYER_NONE) {
                empty_squares++;
            } else {
                // if the current square isnt empty, print its representation, before that print empty squares, if any
                if (empty_squares > 0) {
                    outbuf += sprintf(outbuf, "%d", empty_squares);
                    empty_squares = 0;
                }
                outbuf += sprintf(outbuf, "%c", (cell_player == 1 ? 'X' : 'O'));
            }
        }
        if (empty_squares > 0) {
            outbuf += sprintf(outbuf, "%d", empty_squares);
        }
        if (y > 0) {
            outbuf += sprintf(outbuf, "/");
        }
    }
    // current player
    uint8_t ptm_count;
    const player_id* ptm_buf;
    players_to_move_gf(self, &ptm_count, &ptm_buf);
    player_id ptm;
    if (ptm_count == 0) {
        ptm = PLAYER_NONE;
    } else {
        ptm = ptm_buf[0];
    }
    switch (ptm) {
        case PLAYER_NONE: {
            outbuf += sprintf(outbuf, " -");
        } break;
        case 1: {
            outbuf += sprintf(outbuf, " X");
        } break;
        case 2: {
            outbuf += sprintf(outbuf, " O");
        } break;
    }
    // result player
    uint8_t res_count;
    const player_id* res_buf;
    get_results_gf(self, &res_count, &res_buf);
    player_id res;
    if (res_count == 0) {
        res = PLAYER_NONE;
    } else {
        res = res_buf[0];
    }
    switch (res) {
        case PLAYER_NONE: {
            outbuf += sprintf(outbuf, " -");
        } break;
        case 1: {
            outbuf += sprintf(outbuf, " X");
        } break;
        case 2: {
            outbuf += sprintf(outbuf, " O");
        } break;
    }
    *ret_size = outbuf - ostr;
    *ret_str = bufs.state;
    return ERR_OK;
}

static error_code import_state_gf(game* self, const char* str)
{
    state_repr& data = get_repr(self);
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
                set_cell_gf(self, x++, y, 1);
            } break;
            case 'O': {
                if (x > 2 || y < 0) {
                    // out of bounds board
                    return ERR_INVALID_INPUT;
                }
                set_cell_gf(self, x++, y, 2);
            } break;
            case '1':
            case '2':
            case '3': { // empty squares
                for (int place_empty = (*str) - '0'; place_empty > 0; place_empty--) {
                    if (x > 2) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    set_cell_gf(self, x++, y, PLAYER_NONE);
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
            set_current_player_gf(self, PLAYER_NONE);
        } break;
        case 'X': {
            set_current_player_gf(self, 1);
        } break;
        case 'O': {
            set_current_player_gf(self, 2);
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
            set_result_gf(self, PLAYER_NONE);
        } break;
        case 'X': {
            set_result_gf(self, 1);
        } break;
        case 'O': {
            set_result_gf(self, 2);
        } break;
        default: {
            // failure, ran out of str to use or got invalid character
            return ERR_INVALID_INPUT;
        } break;
    }
    return ERR_OK;
}

static error_code players_to_move_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    *ret_count = 1;
    state_repr& data = get_repr(self);
    player_id ptm = (data.state >> 18) & 0b11;
    if (ptm == PLAYER_NONE) {
        *ret_count = 0;
        return ERR_OK;
    }
    export_buffers& bufs = get_bufs(self);
    player_id* outbuf = bufs.players_to_move;
    *outbuf = ptm;
    *ret_players = outbuf;
    return ERR_OK;
}

static error_code get_concrete_moves_gf(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    export_buffers& bufs = get_bufs(self);
    move_data* outbuf = bufs.concrete_moves;
    uint8_t ptm_count;
    const player_id* ptm;
    players_to_move_gf(self, &ptm_count, &ptm);
    if (ptm_count == 0 || player != *ptm) {
        *ret_count = 0;
        return ERR_OK;
    }
    uint32_t count = 0;
    player_id cell_player;
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            get_cell_gf(self, x, y, &cell_player);
            if (cell_player == PLAYER_NONE) {
                outbuf[count++] = game_e_create_move_small((y << 2) | x);
            }
        }
    }
    *ret_count = count;
    *ret_moves = bufs.concrete_moves;
    return ERR_OK;
}

static error_code is_legal_move_gf(game* self, player_id player, move_data_sync move)
{
    if (game_e_move_sync_is_none(move) == true) {
        return ERR_INVALID_INPUT;
    }
    uint8_t ptm_count;
    const player_id* ptm;
    players_to_move_gf(self, &ptm_count, &ptm);
    if (*ptm != player) {
        return ERR_INVALID_INPUT;
    }
    move_code mcode = move.md.cl.code;
    int x = mcode & 0b11;
    int y = (mcode >> 2) & 0b11;
    if (x > 2 || y > 2) {
        return ERR_INVALID_INPUT;
    }
    player_id cell_player;
    get_cell_gf(self, x, y, &cell_player);
    if (cell_player != PLAYER_NONE) {
        return ERR_INVALID_INPUT;
    }
    return ERR_OK;
}

static error_code make_move_gf(game* self, player_id player, move_data_sync move)
{
    state_repr& data = get_repr(self);
    // set move as current player
    move_code mcode = move.md.cl.code;
    int x = mcode & 0b11;
    int y = (mcode >> 2) & 0b11;
    int current_player = (data.state >> 18) & 0b11;
    set_cell_gf(self, x, y, current_player);
    // detect win for current player
    bool win = false;
    for (int i = 0; i < 3; i++) {
        player_id cell_player_0i;
        player_id cell_player_1i;
        player_id cell_player_2i;
        player_id cell_player_i0;
        player_id cell_player_i1;
        player_id cell_player_i2;
        get_cell_gf(self, 0, i, &cell_player_0i);
        get_cell_gf(self, 1, i, &cell_player_1i);
        get_cell_gf(self, 2, i, &cell_player_2i);
        get_cell_gf(self, i, 0, &cell_player_i0);
        get_cell_gf(self, i, 1, &cell_player_i1);
        get_cell_gf(self, i, 2, &cell_player_i2);
        if (cell_player_0i == cell_player_1i && cell_player_0i == cell_player_2i && cell_player_0i != 0 || cell_player_i0 == cell_player_i1 && cell_player_i0 == cell_player_i2 && cell_player_i0 != 0) {
            win = true;
        }
    }
    player_id cell_player_00;
    player_id cell_player_11;
    player_id cell_player_22;
    player_id cell_player_02;
    player_id cell_player_20;
    get_cell_gf(self, 0, 0, &cell_player_00);
    get_cell_gf(self, 1, 1, &cell_player_11);
    get_cell_gf(self, 2, 2, &cell_player_22);
    get_cell_gf(self, 0, 2, &cell_player_02);
    get_cell_gf(self, 2, 0, &cell_player_20);
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
    set_current_player_gf(self, (current_player == 1) ? 2 : 1);
    return ERR_OK;
}

static error_code get_results_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    export_buffers& bufs = get_bufs(self);
    player_id* outbuf = bufs.results;
    *ret_count = 1;
    state_repr& data = get_repr(self);
    player_id result = (player_id)((data.state >> 20) & 0b11);
    if (result == PLAYER_NONE) {
        *ret_count = 0;
        return ERR_OK;
    }
    *outbuf = result;
    *ret_players = outbuf;
    return ERR_OK;
}

static error_code id_gf(game* self, uint64_t* ret_id)
{
    state_repr& data = get_repr(self);
    uint32_t state_noise = squirrelnoise5(data.state, 0);
    *ret_id = ((uint64_t)state_noise << 32) | (uint64_t)squirrelnoise5(state_noise, state_noise);
    return ERR_OK;
}

static error_code playout_gf(game* self, uint64_t seed)
{
    fast_prng rng;
    fprng_srand(&rng, seed);
    uint32_t moves_count;
    const move_data* moves;
    uint8_t ptm_count;
    const player_id* ptm;
    players_to_move_gf(self, &ptm_count, &ptm);
    while (ptm_count > 0) {
        get_concrete_moves_gf(self, ptm[0], &moves_count, &moves);
        make_move_gf(self, ptm[0], game_e_move_make_sync(self, moves[fprng_rand(&rng) % moves_count])); //BUG this has modulo bias, use proper rand_intn
        players_to_move_gf(self, &ptm_count, &ptm);
    }
    return ERR_OK;
}

static error_code get_move_data_gf(game* self, player_id player, const char* str, move_data_sync** ret_move)
{
    export_buffers& bufs = get_bufs(self);
    if (strlen(str) >= 1 && str[0] == '-') {
        bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
        *ret_move = &bufs.move_out;
        return ERR_INVALID_INPUT;
    }
    if (strlen(str) != 2) {
        bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
        *ret_move = &bufs.move_out;
        return ERR_INVALID_INPUT;
    }
    move_code move_id = 0;
    int x = (str[0] - 'a');
    int y = (str[1] - '0');
    if (x < 0 || x > 2 || y < 0 || y > 2) {
        bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
        *ret_move = &bufs.move_out;
        return ERR_INVALID_INPUT;
    }
    move_id |= x;
    move_id |= (y << 2);
    bufs.move_out = game_e_create_move_sync_small(self, move_id);
    *ret_move = &bufs.move_out;
    return ERR_OK;
}

static error_code get_move_str_gf(game* self, player_id player, move_data_sync move, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    char* outbuf = bufs.move_str;
    move_code mcode = move.md.cl.code;
    if (mcode == MOVE_NONE) {
        *ret_size = sprintf(outbuf, "-");
        return ERR_OK;
    }
    int x = mcode & 0b11;
    int y = (mcode >> 2) & 0b11;
    *ret_size = sprintf(outbuf, "%c%c", 'a' + x, '0' + y);
    *ret_str = bufs.move_str;
    return ERR_OK;
}

static error_code print_gf(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    char* outbuf = bufs.print;
    player_id cell_player;
    for (int y = 2; y >= 0; y--) {
        for (int x = 0; x < 3; x++) {
            get_cell_gf(self, x, y, &cell_player);
            switch (cell_player) {
                case 1: {
                    outbuf += sprintf(outbuf, "X");
                } break;
                case 2: {
                    outbuf += sprintf(outbuf, "O");
                } break;
                default: {
                    outbuf += sprintf(outbuf, ".");
                } break;
            }
        }
        outbuf += sprintf(outbuf, "\n");
    }
    *ret_size = outbuf - bufs.print;
    *ret_str = bufs.print;
    return ERR_OK;
}

//=====
// game internal methods

static error_code get_cell_gf(game* self, int x, int y, player_id* p)
{
    state_repr& data = get_repr(self);
    // shift over the correct 2 bits representing the player at that position
    *p = ((data.state >> (y * 6 + x * 2)) & 0b11);
    return ERR_OK;
}

static error_code set_cell_gf(game* self, int x, int y, player_id p)
{
    state_repr& data = get_repr(self);
    player_id pc;
    get_cell_gf(self, x, y, &pc);
    int offset = (y * 6 + x * 2);
    // new_state = current_value xor (current_value xor new_value)
    data.state ^= ((((uint32_t)pc) << offset) ^ (((uint32_t)p) << offset));
    return ERR_OK;
}

static error_code set_current_player_gf(game* self, player_id p)
{
    state_repr& data = get_repr(self);
    data.state &= ~(0b11 << 18); // reset current player to 0
    data.state |= p << 18; // insert new current player
    return ERR_OK;
}

static error_code set_result_gf(game* self, player_id p)
{
    state_repr& data = get_repr(self);
    data.state &= ~(0b11 << 20); // reset result to 0
    data.state |= p << 20; // insert new result
    return ERR_OK;
}

#ifdef __cplusplus
}
#endif
