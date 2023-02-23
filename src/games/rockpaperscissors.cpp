#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rosalia/noise.h"
#include "rosalia/semver.h"
#include "rosalia/serialization.h"

#include "surena/game.h"

#include "surena/games/rockpaperscissors.h"

// general purpose helpers for opts, data, bufs

namespace {

    struct export_buffers {
        char* state;
        player_id* players_to_move;
        move_data* concrete_moves;
        move_data* actions;
        player_id* results;
        sync_data* sync_out;
        move_data_sync move_out;
        char* move_str;
        char* print;
    };

    struct state_repr {
        uint8_t acc[2];
        bool done;
        player_id res;
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
static error_code get_played_gf(game* self, player_id p, uint8_t* m);
static error_code calc_done_gf(game* self);

// need internal function pointer struct here
static const rockpaperscissors_internal_methods rockpaperscissors_gbe_internal_methods{
    .get_played = get_played_gf,
    .calc_done = calc_done_gf,
};

// declare and form game
#define SURENA_GDD_BENAME rockpaperscissors_standard_gbe
#define SURENA_GDD_GNAME "RockPaperScissors"
#define SURENA_GDD_VNAME "Standard"
#define SURENA_GDD_INAME "surena_default"
#define SURENA_GDD_VERSION ((semver){1, 0, 1})
#define SURENA_GDD_INTERNALS &rockpaperscissors_gbe_internal_methods
#define SURENA_GDD_FF_SIMULTANEOUS_MOVES
#define SURENA_GDD_FF_DISCRETIZE
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
        bufs.state = (char*)malloc(8 * sizeof(char)); // oversized
        bufs.players_to_move = (player_id*)malloc(2 * sizeof(player_id));
        bufs.concrete_moves = (move_data*)malloc(3 * sizeof(move_data));
        bufs.actions = (move_data*)malloc(1 * sizeof(move_data));
        bufs.results = (player_id*)malloc(1 * sizeof(char));
        {
            // we know there is only ever one sync data
            bufs.sync_out = (sync_data*)malloc(1 * sizeof(sync_data));
            bufs.sync_out->player_c = 1;
            bufs.sync_out->players = (player_id*)malloc(2 * sizeof(player_id));
            bufs.sync_out->players[0] = 1;
            bufs.sync_out->players[1] = 2;
            blob_create(&bufs.sync_out->b, 2 * sizeof(uint8_t));
        }
        bufs.move_str = (char*)malloc(2 * sizeof(char));
        bufs.print = (char*)malloc(8 * sizeof(char)); // oversized
        if (bufs.state == NULL ||
            bufs.players_to_move == NULL ||
            bufs.concrete_moves == NULL ||
            bufs.actions == NULL ||
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
        free(bufs.actions);
        free(bufs.results);
        {
            // we know there is only ever one sync data
            free(bufs.sync_out->players);
            blob_destroy(&bufs.sync_out->b);
            free(bufs.sync_out);
        }
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
    *ret_count = 2;
    return ERR_OK;
}

static error_code export_state_gf(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    char* outbuf = bufs.state;
    uint8_t acc1 = data.acc[0];
    if (data.done == false && acc1 != ROCKPAPERSCISSORS_NONE && (player != 1 && player != PLAYER_NONE)) {
        acc1 = ROCKPAPERSCISSORS_ANY;
    }
    switch (acc1) {
        case ROCKPAPERSCISSORS_NONE: {
            outbuf += sprintf(outbuf, "-");
        } break;
        case ROCKPAPERSCISSORS_ANY: {
            outbuf += sprintf(outbuf, "*");
        } break;
        case ROCKPAPERSCISSORS_ROCK: {
            outbuf += sprintf(outbuf, "R");
        } break;
        case ROCKPAPERSCISSORS_PAPER: {
            outbuf += sprintf(outbuf, "P");
        } break;
        case ROCKPAPERSCISSORS_SCISSOR: {
            outbuf += sprintf(outbuf, "S");
        } break;
    }
    outbuf += sprintf(outbuf, "-");
    uint8_t acc2 = data.acc[1];
    if (data.done == false && acc2 != ROCKPAPERSCISSORS_NONE && (player != 2 && player != PLAYER_NONE)) {
        acc2 = ROCKPAPERSCISSORS_ANY;
    }
    switch (acc2) {
        case ROCKPAPERSCISSORS_NONE: {
            outbuf += sprintf(outbuf, "-");
        } break;
        case ROCKPAPERSCISSORS_ANY: {
            outbuf += sprintf(outbuf, "*");
        } break;
        case ROCKPAPERSCISSORS_ROCK: {
            outbuf += sprintf(outbuf, "R");
        } break;
        case ROCKPAPERSCISSORS_PAPER: {
            outbuf += sprintf(outbuf, "P");
        } break;
        case ROCKPAPERSCISSORS_SCISSOR: {
            outbuf += sprintf(outbuf, "S");
        } break;
    }
    *ret_size = outbuf - bufs.state;
    *ret_str = bufs.state;
    return ERR_OK;
}

static error_code import_state_gf(game* self, const char* str)
{
    state_repr& data = get_repr(self);
    data = (state_repr){
        .acc = {ROCKPAPERSCISSORS_NONE, ROCKPAPERSCISSORS_NONE},
        .done = false,
        .res = PLAYER_NONE,
    };
    if (str == NULL) {
        return ERR_OK;
    }
    char tc[2];
    int rc = sscanf(str, "%c-%c", &tc[0], &tc[1]);
    if (rc != 2) {
        return ERR_INVALID_INPUT;
    }
    for (int i = 0; i < 2; i++) {
        switch (tc[i]) {
            case '-': {
                data.acc[i] = ROCKPAPERSCISSORS_NONE;
            } break;
            case '*': {
                data.acc[i] = ROCKPAPERSCISSORS_ANY;
            } break;
            case 'R': {
                data.acc[i] = ROCKPAPERSCISSORS_ROCK;
            } break;
            case 'P': {
                data.acc[i] = ROCKPAPERSCISSORS_PAPER;
            } break;
            case 'S': {
                data.acc[i] = ROCKPAPERSCISSORS_SCISSOR;
            } break;
            default: {
                return ERR_INVALID_INPUT;
            } break;
        }
    }
    calc_done_gf(self);
    return ERR_OK;
}

static error_code players_to_move_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    state_repr& data = get_repr(self);
    if (data.done == true) {
        *ret_count = 0;
        return ERR_OK;
    }
    export_buffers& bufs = get_bufs(self);
    uint8_t count = 0;
    player_id* outbuf = bufs.players_to_move;
    for (uint8_t i = 0; i < 2; i++) {
        if (data.acc[i] == ROCKPAPERSCISSORS_NONE) {
            outbuf[count++] = i + 1;
        }
    }
    *ret_count = count;
    *ret_players = outbuf;
    return ERR_OK;
}

static error_code get_concrete_moves_gf(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    uint8_t count = 0;
    move_data* outbuf = bufs.concrete_moves;
    outbuf[count++] = game_e_create_move_small(ROCKPAPERSCISSORS_ROCK);
    outbuf[count++] = game_e_create_move_small(ROCKPAPERSCISSORS_PAPER);
    outbuf[count++] = game_e_create_move_small(ROCKPAPERSCISSORS_SCISSOR);
    *ret_count = count;
    *ret_moves = outbuf;
    return ERR_OK;
}

static error_code get_actions_gf(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    uint8_t count = 0;
    move_data* outbuf = bufs.actions;
    outbuf[count++] = game_e_create_move_small(ROCKPAPERSCISSORS_ANY);
    *ret_count = count;
    *ret_moves = outbuf;
    return ERR_OK;
}

static error_code is_legal_move_gf(game* self, player_id player, move_data_sync move)
{
    //TODO is this ok?
    state_repr& data = get_repr(self);
    if (data.done == true || data.acc[player - 1] != ROCKPAPERSCISSORS_NONE) {
        return ERR_INVALID_INPUT;
    }
    return ERR_OK;
}

static error_code move_to_action_gf(game* self, player_id player, move_data_sync move, player_id target_player, move_data_sync** ret_action)
{
    //TODO is this ok?
    export_buffers& bufs = get_bufs(self);
    if (player == target_player) { // playing player gets their move, everyone else gets the any action
        bufs.move_out = game_e_create_move_sync_small(self, move.md.cl.code);
    } else {
        bufs.move_out = game_e_create_move_sync_small(self, ROCKPAPERSCISSORS_ANY);
    }
    *ret_action = &bufs.move_out;
    return ERR_OK;
}

static error_code is_action_gf(game* self, player_id player, move_data_sync move, bool* ret_is_action)
{
    //TODO is this ok?
    *ret_is_action = move.md.cl.code == ROCKPAPERSCISSORS_ANY;
    return ERR_OK;
}

static error_code make_move_gf(game* self, player_id player, move_data_sync move)
{
    //TODO is this ok?
    state_repr& data = get_repr(self);
    data.acc[player - 1] = move.md.cl.code;
    calc_done_gf(self);
    return ERR_OK;
}

static error_code get_results_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    bufs.results[0] = data.res;
    *ret_count = 1;
    if (data.res == PLAYER_NONE) {
        *ret_count = 0;
        return ERR_OK;
    }
    *ret_players = bufs.results;
    return ERR_OK;
}

static error_code discretize_gf(game* self, uint64_t seed)
{
    state_repr& data = get_repr(self);
    for (uint8_t i = 0; i < 2; i++) {
        if (data.acc[i] == ROCKPAPERSCISSORS_ANY) {
            data.acc[i] = 2 + squirrelnoise5(0, seed) % 3;
        }
    }
    calc_done_gf(self);
    return ERR_OK;
}

static error_code redact_keep_state_gf(game* self, uint8_t count, const player_id* players)
{
    //TODO redact to ANY if the other player hasnt played yet
    state_repr& data = get_repr(self);
    if (data.done == true) {
        return ERR_OK; // nothing to redact //TODO is this fine?
    }
    bool redact[2];
    for (uint8_t i = 0; i < count; i++) {
        redact[players[i] - 1] = true;
    }
    for (int i = 0; i < 2; i++) {
        if (redact[i] == true && data.acc[i] != ROCKPAPERSCISSORS_NONE) {
            data.acc[i] = ROCKPAPERSCISSORS_ANY;
        }
    }
    return ERR_OK;
}

static error_code export_sync_data_gf(game* self, uint32_t* ret_count, const sync_data** ret_sync_data)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    if (data.done == false) {
        return ERR_OK;
    }
    for (int i = 0; i < 2; i++) {
        ((uint8_t*)bufs.sync_out->b.data)[i] = data.acc[i];
    }
    *ret_count = 1;
    *ret_sync_data = bufs.sync_out;
    return ERR_OK;
}

static error_code import_sync_data_gf(game* self, blob b)
{
    state_repr& data = get_repr(self);
    for (int i = 0; i < 2; i++) {
        data.acc[i] = ((uint8_t*)b.data)[i];
    }
    calc_done_gf(self);
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
        case '*': {
            mcode = ROCKPAPERSCISSORS_ANY;
        } break;
        case 'R': {
            mcode = ROCKPAPERSCISSORS_ROCK;
        } break;
        case 'P': {
            mcode = ROCKPAPERSCISSORS_PAPER;
        } break;
        case 'S': {
            mcode = ROCKPAPERSCISSORS_SCISSOR;
        } break;
        default: {
            return ERR_INVALID_INPUT;
        } break;
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
    switch (mcode) {
        case ROCKPAPERSCISSORS_ANY: {
            outbuf += sprintf(outbuf, "*");
        } break;
        case ROCKPAPERSCISSORS_ROCK: {
            outbuf += sprintf(outbuf, "R");
        } break;
        case ROCKPAPERSCISSORS_PAPER: {
            outbuf += sprintf(outbuf, "P");
        } break;
        case ROCKPAPERSCISSORS_SCISSOR: {
            outbuf += sprintf(outbuf, "S");
        } break;
    }
    *ret_size = outbuf - bufs.move_str;
    *ret_str = bufs.move_str;
    return ERR_OK;
}

static error_code print_gf(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    state_repr& data = get_repr(self);
    char* outbuf = bufs.state;
    uint8_t acc1 = data.acc[0];
    if (data.done == false && acc1 != ROCKPAPERSCISSORS_NONE && (player != 1 && player != PLAYER_NONE)) {
        acc1 = ROCKPAPERSCISSORS_ANY;
    }
    switch (acc1) {
        case ROCKPAPERSCISSORS_NONE: {
            outbuf += sprintf(outbuf, "-");
        } break;
        case ROCKPAPERSCISSORS_ANY: {
            outbuf += sprintf(outbuf, "*");
        } break;
        case ROCKPAPERSCISSORS_ROCK: {
            outbuf += sprintf(outbuf, "R");
        } break;
        case ROCKPAPERSCISSORS_PAPER: {
            outbuf += sprintf(outbuf, "P");
        } break;
        case ROCKPAPERSCISSORS_SCISSOR: {
            outbuf += sprintf(outbuf, "S");
        } break;
    }
    if (data.done == true) {
        switch (data.res) {
            case PLAYER_NONE: {
                outbuf += sprintf(outbuf, "=");
            } break;
            case 1: {
                outbuf += sprintf(outbuf, ">");
            } break;
            case 2: {
                outbuf += sprintf(outbuf, "<");
            } break;
        }
    } else {
        outbuf += sprintf(outbuf, "-");
    }
    uint8_t acc2 = data.acc[1];
    if (data.done == false && acc2 != ROCKPAPERSCISSORS_NONE && (player != 2 && player != PLAYER_NONE)) {
        acc2 = ROCKPAPERSCISSORS_ANY;
    }
    switch (acc2) {
        case ROCKPAPERSCISSORS_NONE: {
            outbuf += sprintf(outbuf, "-");
        } break;
        case ROCKPAPERSCISSORS_ANY: {
            outbuf += sprintf(outbuf, "*");
        } break;
        case ROCKPAPERSCISSORS_ROCK: {
            outbuf += sprintf(outbuf, "R");
        } break;
        case ROCKPAPERSCISSORS_PAPER: {
            outbuf += sprintf(outbuf, "P");
        } break;
        case ROCKPAPERSCISSORS_SCISSOR: {
            outbuf += sprintf(outbuf, "S");
        } break;
    }
    outbuf += sprintf(outbuf, "\n");
    *ret_size = outbuf - bufs.state;
    *ret_str = bufs.state;
    return ERR_OK;
}

//=====
// game internal methods

error_code get_played_gf(game* self, player_id p, uint8_t* m)
{
    state_repr& data = get_repr(self);
    *m = data.acc[p - 1];
    return ERR_OK;
}

error_code calc_done_gf(game* self)
{
    //TODO //BUG //HACK what is one player has any in their acc? for now treat as not done, should be able to signify a state that MUST be discretized or receive sync data to actually resolve..
    state_repr& data = get_repr(self);
    bool is_done = true;
    for (int i = 0; i < 2; i++) {
        if (data.acc[i] == ROCKPAPERSCISSORS_NONE || data.acc[i] == ROCKPAPERSCISSORS_ANY) {
            is_done = false;
            break;
        }
    }
    data.done = is_done;
    if (is_done == false) {
        return ERR_OK;
    }
    if (data.acc[0] == data.acc[1]) {
        data.res = PLAYER_NONE;
        return ERR_OK;
    }
    switch (data.acc[0]) {
        case ROCKPAPERSCISSORS_ROCK: {
            data.res = data.acc[1] == ROCKPAPERSCISSORS_PAPER ? 2 : 1;
        } break;
        case ROCKPAPERSCISSORS_PAPER: {
            data.res = data.acc[1] == ROCKPAPERSCISSORS_SCISSOR ? 2 : 1;
        } break;
        case ROCKPAPERSCISSORS_SCISSOR: {
            data.res = data.acc[1] == ROCKPAPERSCISSORS_ROCK ? 2 : 1;
        } break;
    }
    return ERR_OK;
}

#ifdef __cplusplus
}
#endif
