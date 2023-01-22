#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rosalia/rand.h"
#include "rosalia/noise.h"
#include "rosalia/semver.h"

#include "surena/game.h"

#include "surena/games/tictactoe_ultimate.h"

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

static error_code check_result_gf(game* self, uint32_t state, player_id* ret_p);
static error_code get_cell_gf(game* self, uint32_t state, int x, int y, player_id* ret_p);
static error_code set_cell_gf(game* self, uint32_t* state, int x, int y, player_id p);
static error_code get_cell_global_gf(game* self, int x, int y, player_id* ret_p);
static error_code set_cell_global_gf(game* self, int x, int y, player_id p);
static error_code get_cell_local_gf(game* self, int x, int y, player_id* ret_p);
static error_code set_cell_local_gf(game* self, int x, int y, player_id p);
static error_code get_global_target_gf(game* self, uint8_t* ret);
static error_code set_global_target_gf(game* self, int x, int y);
static error_code set_current_player_gf(game* self, player_id p);
static error_code set_result_gf(game* self, player_id p);

static const tictactoe_ultimate_internal_methods tictactoe_ultimate_gbe_internal_methods{
    .check_result = check_result_gf,
    .get_cell = get_cell_gf,
    .set_cell = set_cell_gf,
    .get_cell_global = get_cell_global_gf,
    .set_cell_global = set_cell_global_gf,
    .get_cell_local = get_cell_local_gf,
    .set_cell_local = set_cell_local_gf,
    .get_global_target = get_global_target_gf,
    .set_global_target = set_global_target_gf,
    .set_current_player = set_current_player_gf,
    .set_result = set_result_gf,
};

// declare and form game
#define SURENA_GDD_BENAME tictactoe_ultimate_gbe
#define SURENA_GDD_GNAME "TicTacToe"
#define SURENA_GDD_VNAME "Ultimate"
#define SURENA_GDD_INAME "surena_default"
#define SURENA_GDD_VERSION ((semver){0, 2, 0})
#define SURENA_GDD_INTERNALS &tictactoe_ultimate_gbe_internal_methods
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
        bufs.state = (char*)malloc(37 * sizeof(char));
        bufs.players_to_move = (player_id*)malloc(1 * sizeof(player_id));
        bufs.concrete_moves = (move_data*)malloc(81 * sizeof(move_data));
        bufs.results = (player_id*)malloc(1 * sizeof(player_id));
        bufs.move_str = (char*)malloc(3 * sizeof(char));
        bufs.print = (char*)malloc(180 * sizeof(char)); // slightly oversized
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
    if (outbuf == NULL) {
        return ERR_INVALID_INPUT;
    }
    state_repr& data = get_repr(self);
    const char* ostr = outbuf;
    player_id cell_player;
    for (int y = 8; y >= 0; y--) {
        int empty_squares = 0;
        for (int x = 0; x < 9; x++) {
            get_cell_local_gf(self, x, y, &cell_player);
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
    // global target
    if (data.global_target_x >= 0 && data.global_target_y >= 0) {
        outbuf += sprintf(outbuf, " %c%c", 'a' + data.global_target_x, '0' + data.global_target_y);
    } else {
        outbuf += sprintf(outbuf, " -");
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
                set_cell_local_gf(self, x++, y, 1);
            } break;
            case 'O': {
                if (x > 8 || y < 0) {
                    // out of bounds board
                    return ERR_INVALID_INPUT;
                }
                set_cell_local_gf(self, x++, y, 2);
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
                    set_cell_local_gf(self, x++, y, PLAYER_NONE);
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
            check_result_gf(self, data.board[iy][ix], &local_result);
            if (local_result > 0) {
                set_cell_global_gf(self, ix, iy, local_result);
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
    player_id ptm = data.current_player;
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
    state_repr& data = get_repr(self);
    uint32_t count = 0;
    player_id cell_player;
    if (data.global_target_x >= 0 && data.global_target_y >= 0) {
        // only give moves for the target local board
        for (int ly = data.global_target_y * 3; ly < data.global_target_y * 3 + 3; ly++) {
            for (int lx = data.global_target_x * 3; lx < data.global_target_x * 3 + 3; lx++) {
                get_cell_local_gf(self, lx, ly, &cell_player);
                if (cell_player == PLAYER_NONE) {
                    outbuf[count++] = game_e_create_move_small((ly << 4) | lx);
                }
            }
        }
    } else {
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                get_cell_global_gf(self, gx, gy, &cell_player);
                if (cell_player == PLAYER_NONE) {
                    for (int ly = gy * 3; ly < gy * 3 + 3; ly++) {
                        for (int lx = gx * 3; lx < gx * 3 + 3; lx++) {
                            get_cell_local_gf(self, lx, ly, &cell_player);
                            if (cell_player == PLAYER_NONE) {
                                outbuf[count++] = game_e_create_move_small((ly << 4) | lx);
                            }
                        }
                    }
                }
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
    int x = mcode & 0b1111;
    int y = (mcode >> 4) & 0b1111;
    if (x > 8 || y > 8) {
        return ERR_INVALID_INPUT;
    }
    player_id cell_player;
    get_cell_local_gf(self, x, y, &cell_player);
    if (cell_player != PLAYER_NONE) {
        return ERR_INVALID_INPUT;
    }
    state_repr& data = get_repr(self);
    // check if we're playing into the global target, if any
    if (data.global_target_x >= 0 && data.global_target_y >= 0) {
        if ((x / 3 != data.global_target_x) || (y / 3 != data.global_target_y)) {
            return ERR_INVALID_INPUT;
        }
    }
    return ERR_OK;
}

static error_code make_move_gf(game* self, player_id player, move_data_sync move)
{
    state_repr& data = get_repr(self);
    // calc move and set cell
    move_code mcode = move.md.cl.code;
    int x = mcode & 0b1111;
    int y = (mcode >> 4) & 0b1111;
    set_cell_local_gf(self, x, y, data.current_player);
    int gx = x / 3;
    int gy = y / 3;
    int lx = x % 3;
    int ly = y % 3;
    // calculate possible result of local board
    uint8_t local_result;
    check_result_gf(self, data.board[gy][gx], &local_result);
    if (local_result > 0) {
        set_cell_global_gf(self, gx, gy, local_result);
    }
    // set global target if applicable
    player_id cell_player;
    get_cell_global_gf(self, lx, ly, &cell_player);
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
    check_result_gf(self, data.global_board, &global_result);
    if (global_result > 0) {
        data.winning_player = global_result;
        data.current_player = 0;
        return ERR_OK;
    }
    // switch player
    data.current_player = (data.current_player == 1) ? 2 : 1;
    return ERR_OK;
}

static error_code get_results_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    export_buffers& bufs = get_bufs(self);
    player_id* outbuf = bufs.results;
    *ret_count = 1;
    state_repr& data = get_repr(self);
    player_id result = data.winning_player;
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
    uint32_t r_id = squirrelnoise5((int32_t)data.global_board, data.global_target_x + data.global_target_y + data.current_player);
    for (int i = 0; i < 9; i++) {
        r_id = squirrelnoise5((int32_t)data.board[i / 3][i % 3], r_id);
    }
    *ret_id = ((uint64_t)r_id << 32) | (uint64_t)squirrelnoise5(r_id, r_id);
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
        make_move_gf(self, ptm[0], game_e_move_make_sync(self, moves[fprng_rand(&rng) % moves_count]));
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
    if (x < 0 || x > 8 || y < 0 || y > 8) {
        bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
        *ret_move = &bufs.move_out;
        return ERR_INVALID_INPUT;
    }
    move_id |= x;
    move_id |= (y << 4);
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
    int x = mcode & 0b1111;
    int y = (mcode >> 4) & 0b1111;
    *ret_size = sprintf(outbuf, "%c%c", 'a' + x, '0' + y);
    *ret_str = bufs.move_str;
    return ERR_OK;
}

static error_code print_gf(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    char* outbuf = bufs.print;
    state_repr& data = get_repr(self);
    player_id cell_player;
    size_t global_target_str_len;
    const char* global_target_str;
    get_move_str_gf(self, data.current_player, game_e_create_move_sync_small(self, (data.global_target_y << 4) | data.global_target_x), &global_target_str_len, &global_target_str);
    outbuf += sprintf(outbuf, "global target: %s\n", (data.global_target_x >= 0 && data.global_target_y >= 0) ? global_target_str : "-");
    for (int gy = 2; gy >= 0; gy--) {
        for (int ly = 2; ly >= 0; ly--) {
            outbuf += sprintf(outbuf, "%c ", '0' + (gy * 3 + ly));
            for (int gx = 0; gx < 3; gx++) {
                for (int lx = 0; lx < 3; lx++) {
                    get_cell_global_gf(self, gx, gy, &cell_player);
                    if (cell_player == 0) {
                        get_cell_local_gf(self, gx * 3 + lx, gy * 3 + ly, &cell_player);
                    }
                    switch (cell_player) {
                        case (1): {
                            outbuf += sprintf(outbuf, "X");
                        } break;
                        case (2): {
                            outbuf += sprintf(outbuf, "O");
                        } break;
                        case (3): {
                            outbuf += sprintf(outbuf, "=");
                        } break;
                        default: {
                            outbuf += sprintf(outbuf, ".");
                        } break;
                    }
                }
                outbuf += sprintf(outbuf, " ");
            }
            outbuf += sprintf(outbuf, "\n");
        }
        if (gy > 0) {
            outbuf += sprintf(outbuf, "\n");
        }
    }
    outbuf += sprintf(outbuf, "  ");
    for (int gx = 0; gx < 3; gx++) {
        for (int lx = 0; lx < 3; lx++) {
            outbuf += sprintf(outbuf, "%c", 'a' + (gx * 3 + lx));
        }
        outbuf += sprintf(outbuf, " ");
    }
    outbuf += sprintf(outbuf, "\n");
    *ret_size = outbuf - bufs.print;
    *ret_str = bufs.print;
    return ERR_OK;
}

//=====
// game internal methods

static error_code check_result_gf(game* self, uint32_t state, player_id* ret_p)
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
        get_cell_gf(self, state, 0, i, &cell_player_0i);
        get_cell_gf(self, state, 1, i, &cell_player_1i);
        get_cell_gf(self, state, 2, i, &cell_player_2i);
        get_cell_gf(self, state, i, 0, &cell_player_i0);
        get_cell_gf(self, state, i, 1, &cell_player_i1);
        get_cell_gf(self, state, i, 2, &cell_player_i2);
        if (cell_player_0i == cell_player_1i && cell_player_0i == cell_player_2i && cell_player_0i != 0 || cell_player_i0 == cell_player_i1 && cell_player_i0 == cell_player_i2 && cell_player_i0 != 0) {
            win = true;
            get_cell_gf(self, state, i, i, &state_winner);
        }
    }
    player_id cell_player_00;
    player_id cell_player_11;
    player_id cell_player_22;
    player_id cell_player_02;
    player_id cell_player_20;
    get_cell_gf(self, state, 0, 0, &cell_player_00);
    get_cell_gf(self, state, 1, 1, &cell_player_11);
    get_cell_gf(self, state, 2, 2, &cell_player_22);
    get_cell_gf(self, state, 0, 2, &cell_player_02);
    get_cell_gf(self, state, 2, 0, &cell_player_20);
    if ((cell_player_00 == cell_player_11 && cell_player_00 == cell_player_22 || cell_player_02 == cell_player_11 && cell_player_02 == cell_player_20) && cell_player_11 != 0) {
        win = true;
        get_cell_gf(self, state, 1, 1, &state_winner);
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

static error_code get_cell_gf(game* self, uint32_t state, int x, int y, player_id* ret_p)
{
    // shift over the correct 2 bits representing the player at that position
    *ret_p = ((state >> (y * 6 + x * 2)) & 0b11);
    return ERR_OK;
}

static error_code set_cell_gf(game* self, uint32_t* state, int x, int y, player_id p)
{
    player_id pc;
    get_cell_gf(self, *state, x, y, &pc);
    int offset = (y * 6 + x * 2);
    // new_state = current_value xor (current_value xor new_value)
    *state ^= ((((uint32_t)pc) << offset) ^ (((uint32_t)p) << offset));
    return ERR_OK;
}

static error_code get_cell_global_gf(game* self, int x, int y, player_id* ret_p)
{
    state_repr& data = get_repr(self);
    get_cell_gf(self, data.global_board, x, y, ret_p);
    return ERR_OK;
}

static error_code set_cell_global_gf(game* self, int x, int y, player_id p)
{
    state_repr& data = get_repr(self);
    set_cell_gf(self, &(data.global_board), x, y, p);
    return ERR_OK;
}

static error_code get_cell_local_gf(game* self, int x, int y, player_id* ret_p)
{
    state_repr& data = get_repr(self);
    get_cell_gf(self, data.board[y / 3][x / 3], x % 3, y % 3, ret_p);
    return ERR_OK;
}

static error_code set_cell_local_gf(game* self, int x, int y, player_id p)
{
    state_repr& data = get_repr(self);
    set_cell_gf(self, &(data.board[y / 3][x / 3]), x % 3, y % 3, p);
    return ERR_OK;
}

static error_code get_global_target_gf(game* self, uint8_t* ret)
{
    state_repr& data = get_repr(self);
    *ret = ((data.global_target_y == -1 ? 3 : data.global_target_y) << 2) | (data.global_target_x == -1 ? 3 : data.global_target_x);
    return ERR_OK;
}

static error_code set_global_target_gf(game* self, int x, int y)
{
    state_repr& data = get_repr(self);
    data.global_target_x = x;
    data.global_target_y = y;
    return ERR_OK;
}

static error_code set_current_player_gf(game* self, player_id p)
{
    state_repr& data = get_repr(self);
    data.current_player = p;
    return ERR_OK;
}

static error_code set_result_gf(game* self, player_id p)
{
    state_repr& data = get_repr(self);
    data.winning_player = p;
    return ERR_OK;
}

//TODO fix X/O enum, same as tictactoe_standard

#ifdef __cplusplus
}
#endif
