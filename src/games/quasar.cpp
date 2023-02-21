#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rosalia/noise.h"
#include "rosalia/semver.h"

#include "surena/game.h"

#include "surena/games/quasar.h"

// general purpose helpers for opts, data, bufs

namespace {

    struct export_buffers {
        char* state;
        player_id* players_to_move;
        move_data* concrete_moves;
        float* move_probabilities;
        player_id* results;
        move_data_sync move_out;
        char* move_str;
        char* print;
    };

    struct state_repr {
        uint64_t seed;
        uint64_t move_ctr;
        uint8_t num;
        uint8_t generating; // 0 if not, 1 if 1-8, 2 if 4-7
        bool done;
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
static error_code get_num_gf(game* self, uint8_t* n);
static error_code get_score_gf(game* self, int* s);
static error_code can_big_gf(game* self, bool* r);
static error_code can_payout_gf(game* self, bool* r);
static error_code can_small_gf(game* self, bool* r);

// need internal function pointer struct here
static const quasar_internal_methods quasar_gbe_internal_methods{
    .get_num = get_num_gf,
    .get_score = get_score_gf,
    .can_big = can_big_gf,
    .can_payout = can_payout_gf,
    .can_small = can_small_gf};

// declare and form game
#define SURENA_GDD_BENAME quasar_standard_gbe
#define SURENA_GDD_GNAME "Quasar"
#define SURENA_GDD_VNAME "Standard"
#define SURENA_GDD_INAME "surena_default"
#define SURENA_GDD_VERSION ((semver){1, 0, 1})
#define SURENA_GDD_INTERNALS &quasar_gbe_internal_methods
#define SURENA_GDD_FF_RANDOM_MOVES
#define SURENA_GDD_FF_ID
#define SURENA_GDD_FF_EVAL
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
        bufs.state = (char*)malloc(32 * sizeof(char)); // oversized
        bufs.players_to_move = (player_id*)malloc(1 * sizeof(player_id));
        bufs.concrete_moves = (move_data*)malloc(8 * sizeof(move_data));
        bufs.move_probabilities = (float*)malloc(8 * sizeof(float));
        bufs.results = (player_id*)malloc(1 * sizeof(char));
        bufs.move_str = (char*)malloc(2 * sizeof(char));
        bufs.print = (char*)malloc(64 * sizeof(char)); // oversized
        if (bufs.state == NULL ||
            bufs.players_to_move == NULL ||
            bufs.concrete_moves == NULL ||
            bufs.move_probabilities == NULL ||
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
        free(bufs.move_probabilities);
        free(bufs.results);
        free(bufs.move_str);
        free(bufs.print);
    }
    return grerrorf(self, ERR_OK, NULL);
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
    *ret_count = 1;
    return ERR_OK;
}

static error_code export_state_gf(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    char* outbuf = bufs.state;
    outbuf += sprintf(outbuf, "%hhu", data.num);
    if (data.done == true) {
        outbuf += sprintf(outbuf, " D");
    } else {
        if (data.generating == 0) {
            outbuf += sprintf(outbuf, " -");
        } else if (data.generating == 1) {
            outbuf += sprintf(outbuf, " B");
        } else if (data.generating == 2) {
            outbuf += sprintf(outbuf, " S");
        }
    }
    outbuf += sprintf(outbuf, " %lu", data.move_ctr);
    *ret_size = outbuf - bufs.state;
    *ret_str = bufs.state;
    return ERR_OK;
}

static error_code import_state_gf(game* self, const char* str)
{
    state_repr& data = get_repr(self);
    data = (state_repr){
        .seed = SEED_NONE,
        .move_ctr = 0,
        .num = 0,
        .generating = 1,
        .done = false,
    };
    if (str == NULL) {
        return ERR_OK;
    }
    char tc;
    int rc = sscanf(str, "%hhu %c %lu", &data.num, &tc, &data.move_ctr);
    if (rc != 3) {
        return ERR_INVALID_INPUT;
    }
    switch (tc) {
        case '-': {
            data.generating = 0;
        } break;
        case 'B': {
            data.generating = 1;
        } break;
        case 'D': {
            data.done = true;
        } break;
        case 'S': {
            data.generating = 2;
        } break;
        default: {
            return ERR_INVALID_INPUT;
        } break;
    }
    return ERR_OK;
}

static error_code players_to_move_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    state_repr& data = get_repr(self);
    *ret_count = 1;
    if (data.done == true) {
        *ret_count = 0;
        return ERR_OK;
    }
    export_buffers& bufs = get_bufs(self);
    player_id* outbuf = bufs.players_to_move;
    *outbuf = (data.generating == 0 ? 1 : PLAYER_RAND);
    *ret_players = outbuf;
    return ERR_OK;
}

static error_code get_concrete_moves_gf(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    move_data* outbuf = bufs.concrete_moves;
    uint32_t count = 0;
    if (data.generating == 0) {
        bool can_big;
        bool can_payout;
        bool can_small;
        can_big_gf(self, &can_big);
        can_payout_gf(self, &can_payout);
        can_small_gf(self, &can_small);
        if (can_big == true) {
            outbuf[count++] = game_e_create_move_small(QUASAR_MOVE_BIG);
        }
        if (can_payout == true) {
            outbuf[count++] = game_e_create_move_small(QUASAR_MOVE_PAYOUT);
        }
        if (can_small == true) {
            outbuf[count++] = game_e_create_move_small(QUASAR_MOVE_SMALL);
        }
    } else {
        if (data.seed != SEED_NONE) {
            int min = (data.generating == 1 ? 1 : 4);
            int range = (data.generating == 1 ? 8 : 4);
            int val = min + squirrelnoise5(data.move_ctr, data.seed) % range; //BUG this has modulo bias, use proper rand_intn
            outbuf[count++] = game_e_create_move_small(val);
        } else {
            if (data.generating == 1) {
                for (int i = 1; i < 9; i++) {
                    outbuf[count++] = game_e_create_move_small(i);
                }
            } else {
                for (int i = 4; i < 8; i++) {
                    outbuf[count++] = game_e_create_move_small(i);
                }
            }
        }
    }
    *ret_count = count;
    *ret_moves = bufs.concrete_moves;
    return ERR_OK;
}

static error_code get_concrete_move_probabilities_gf(game* self, player_id player, uint32_t* ret_count, const float** ret_move_probabilities)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    float* outbuf = bufs.move_probabilities;
    uint32_t count = 0;
    if (data.generating == 1) {
        for (int i = 1; i < 9; i++) {
            outbuf[count++] = 0.125;
        }
    } else {
        for (int i = 4; i < 8; i++) {
            outbuf[count++] = 0.25;
        }
    }
    *ret_count = count;
    *ret_move_probabilities = bufs.move_probabilities;
    return ERR_OK;
}

static error_code is_legal_move_gf(game* self, player_id player, move_data_sync move)
{
    if (game_e_move_sync_is_none(move) == true) {
        return ERR_INVALID_INPUT;
    }
    state_repr& data = get_repr(self);
    move_code mcode = move.md.cl.code;
    if (mcode == QUASAR_MOVE_BIG) {
        bool can_do;
        can_big_gf(self, &can_do);
        if (can_do == false) {
            return ERR_INVALID_INPUT;
        }
    } else if (mcode == QUASAR_MOVE_PAYOUT) {
        bool can_do;
        can_payout_gf(self, &can_do);
        if (can_do == false) {
            return ERR_INVALID_INPUT;
        }
    } else if (mcode == QUASAR_MOVE_SMALL) {
        bool can_do;
        can_small_gf(self, &can_do);
        if (can_do == false) {
            return ERR_INVALID_INPUT;
        }
    } else {
        if (data.generating == 0) {
            return ERR_INVALID_INPUT;
        }
        uint8_t val = mcode & 0xFF;
        if (data.generating == 1) {
            if (val < 1 || val > 8) {
                return ERR_INVALID_INPUT;
            }
        } else {
            if (val < 4 || val > 7) {
                return ERR_INVALID_INPUT;
            }
        }
    }
    return ERR_OK;
}

static error_code make_move_gf(game* self, player_id player, move_data_sync move)
{
    state_repr& data = get_repr(self);
    move_code mcode = move.md.cl.code;
    if (mcode == QUASAR_MOVE_BIG) {
        data.generating = 1;
    } else if (mcode == QUASAR_MOVE_PAYOUT) {
        data.done = true;
    } else if (mcode == QUASAR_MOVE_SMALL) {
        data.generating = 2;
    } else {
        data.num += mcode & 0xFF;
        data.generating = 0;
    }
    data.move_ctr++;
    if (data.num >= 20) { //TODO want this?
        data.done = true;
    }
    return ERR_OK;
}

static error_code get_results_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    player_id* outbuf = bufs.results;
    *ret_count = 0;
    if (data.done == true) {
        int score;
        get_score_gf(self, &score);
        if (score > 0) {
            *ret_count = 1;
            *outbuf = 1;
        }
    }
    *ret_players = outbuf;
    return ERR_OK;
}

static error_code id_gf(game* self, uint64_t* ret_id)
{
    state_repr& data = get_repr(self);
    *ret_id = (uint64_t)data.num | ((uint64_t)(data.done ? 1 : 0) << 8) | ((uint64_t)data.generating & 0b11 << 9);
    return ERR_OK;
}

static error_code eval_gf(game* self, player_id player, float* ret_eval)
{
    state_repr& data = get_repr(self);
    if (data.done == true) {
        int score;
        get_score_gf(self, &score);
        *ret_eval = score;
    } else {
        // from the wiki https://masseffect.fandom.com/wiki/Quasar
        if (data.num > 20) {
            *ret_eval = -200;
            return ERR_OK;
        }
        float lut[] = {45, 38, 36, 37, 43, 47, 51, 37, 18, 22, 30, 17, 18, 19, -25, -56, 50, 100, 150, 200};
        *ret_eval = lut[data.num];
    }
    return ERR_OK;
}

static error_code discretize_gf(game* self, uint64_t seed)
{
    state_repr& data = get_repr(self);
    data.seed = seed;
    return ERR_OK;
}

static error_code redact_keep_state_gf(game* self, uint8_t count, const player_id* players)
{
    bool keep_rand = false;
    for (uint8_t i = 0; i < count; i++) {
        if (players[i] == PLAYER_RAND) {
            keep_rand = true;
            break;
        }
    }
    if (keep_rand == false) {
        state_repr& data = get_repr(self);
        data.seed = SEED_NONE;
    }
    return ERR_OK;
}

static error_code get_move_data_gf(game* self, player_id player, const char* str, move_data_sync** ret_move)
{
    export_buffers& bufs = get_bufs(self);
    if (strlen(str) != 1) {
        bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
        *ret_move = &bufs.move_out;
        return ERR_INVALID_INPUT;
    }
    move_code mcode = MOVE_NONE;
    switch (str[0]) {
        case 'B': {
            mcode = QUASAR_MOVE_BIG;
        } break;
        case 'P': {
            mcode = QUASAR_MOVE_PAYOUT;
        } break;
        case 'S': {
            mcode = QUASAR_MOVE_SMALL;
        } break;
    }
    if (mcode == MOVE_NONE) {
        if (str[0] < '1' || str[0] > '8') {
            bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
            *ret_move = &bufs.move_out;
            return ERR_INVALID_INPUT;
        }
        mcode = str[0] - '0';
    }
    bufs.move_out = game_e_create_move_sync_small(self, mcode);
    *ret_move = &bufs.move_out;
    return ERR_OK;
}

static error_code get_move_str_gf(game* self, player_id player, move_data_sync move, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    char* outbuf = bufs.move_str;
    move_code mcode = move.md.cl.code;
    if (mcode == QUASAR_MOVE_BIG) {
        outbuf += sprintf(outbuf, "B");
    } else if (mcode == QUASAR_MOVE_PAYOUT) {
        outbuf += sprintf(outbuf, "P");
    } else if (mcode == QUASAR_MOVE_SMALL) {
        outbuf += sprintf(outbuf, "S");
    } else {
        uint8_t val = mcode & 0xFF;
        outbuf += sprintf(outbuf, "%hhu", val);
    }
    *ret_size = outbuf - bufs.move_str;
    *ret_str = bufs.move_str;
    return ERR_OK;
}

static error_code print_gf(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    char* outbuf = bufs.print;
    {
        int score;
        get_score_gf(self, &score);
        outbuf += sprintf(outbuf, "%hhu: %d", data.num, score);
        if (data.seed != SEED_NONE) {
            outbuf += sprintf(outbuf, "(%016lx)\n", data.seed);
        } else {
            outbuf += sprintf(outbuf, "\n");
        }
        if (data.done == true) {
            outbuf += sprintf(outbuf, "= D\n");
        } else {
            if (data.generating == 0) {
                outbuf += sprintf(outbuf, "> ");
                bool previous = false;
                for (int i = 0; i < 3; i++) {
                    bool can_do;
                    switch (i) {
                        case 0: {
                            can_big_gf(self, &can_do);
                        } break;
                        case 1: {
                            can_payout_gf(self, &can_do);
                        } break;
                        case 2: {
                            can_small_gf(self, &can_do);
                        } break;
                    }
                    if (can_do == true) {
                        if (previous == true) {
                            outbuf += sprintf(outbuf, "/");
                        }
                        switch (i) {
                            case 0: {
                                outbuf += sprintf(outbuf, "B");
                            } break;
                            case 1: {
                                outbuf += sprintf(outbuf, "P");
                            } break;
                            case 2: {
                                outbuf += sprintf(outbuf, "S");
                            } break;
                        }
                        previous = true;
                    }
                }
                outbuf += sprintf(outbuf, "\n");
            } else if (data.generating == 1) {
                outbuf += sprintf(outbuf, "< B\n");
            } else if (data.generating == 2) {
                outbuf += sprintf(outbuf, "< S\n");
            }
        }
    }
    *ret_size = outbuf - bufs.print;
    *ret_str = bufs.print;
    return ERR_OK;
}

//=====
// game internal methods

static error_code get_num_gf(game* self, uint8_t* n)
{
    state_repr& data = get_repr(self);
    *n = data.num;
    return ERR_OK;
}

static error_code get_score_gf(game* self, int* s)
{
    state_repr& data = get_repr(self);
    int score = -200;
    switch (data.num) {
        case 15: {
            score += 50;
        } break;
        case 16: {
            score += 100;
        } break;
        case 17: {
            score += 200;
        } break;
        case 18: {
            score += 250;
        } break;
        case 19: {
            score += 300;
        } break;
        case 20: {
            score += 400;
        } break;
        default: {
            score += 0;
        } break;
    }
    *s = score;
    return ERR_OK;
}

static error_code can_big_gf(game* self, bool* r)
{
    state_repr& data = get_repr(self);
    *r = (data.num <= 19 && data.generating == 0 && data.done == false);
    return ERR_OK;
}

static error_code can_payout_gf(game* self, bool* r)
{
    state_repr& data = get_repr(self);
    *r = (data.num > 15 && data.generating == 0 && data.done == false);
    return ERR_OK;
}

static error_code can_small_gf(game* self, bool* r)
{
    state_repr& data = get_repr(self);
    *r = (data.num <= 16 && data.generating == 0 && data.done == false);
    return ERR_OK;
}

#ifdef __cplusplus
}
#endif
