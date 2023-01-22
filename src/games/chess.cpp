#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rosalia/noise.h"
#include "rosalia/semver.h"

#include "surena/game.h"

#include "surena/games/chess.h"

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
        CHESS_piece board[8][8]; // board[y][x] starting with origin (0,0) on bottom left of the board
        uint32_t halfmove_clock = 0;
        uint32_t fullmove_clock = 1;
        uint8_t enpassant_target = 0xFF; // left nibble is x, right nibble is y
        CHESS_PLAYER current_player : 2;
        CHESS_PLAYER winning_player : 2;
        bool castling_white_king : 1;
        bool castling_white_queen : 1;
        bool castling_black_king : 1;
        bool castling_black_queen : 1;
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

const char CHESS_PIECE_TYPE_CHARS[7] = {'-', 'K', 'Q', 'R', 'B', 'N', 'P'}; // none, king, queen, rook, bishop, knight, pawn

static error_code get_cell_gf(game* self, int x, int y, CHESS_piece* p);
static error_code set_cell_gf(game* self, int x, int y, CHESS_piece p);
static error_code set_current_player_gf(game* self, player_id p);
static error_code set_result_gf(game* self, player_id p);
static error_code count_positions_gf(game* self, int depth, uint64_t* count);
static error_code apply_move_internal_gf(game* self, move_code move, bool replace_castling_by_kings);
static error_code get_moves_pseudo_legal_gf(game* self, uint32_t* move_cnt, move_code* move_vec);

static const chess_internal_methods chess_gbe_internal_methods{
    .get_cell = get_cell_gf,
    .set_cell = set_cell_gf,
    .set_current_player = set_current_player_gf,
    .set_result = set_result_gf,
    .count_positions = count_positions_gf,
    .apply_move_internal = apply_move_internal_gf,
    .get_moves_pseudo_legal = get_moves_pseudo_legal_gf,
};

// declare and form game
#define SURENA_GDD_BENAME chess_standard_gbe
#define SURENA_GDD_GNAME "Chess"
#define SURENA_GDD_VNAME "Standard"
#define SURENA_GDD_INAME "surena_default"
#define SURENA_GDD_VERSION ((semver){0, 2, 0})
#define SURENA_GDD_INTERNALS &chess_gbe_internal_methods
#define SURENA_GDD_FF_ID
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
        bufs.state = (char*)malloc(128 * sizeof(char));
        bufs.players_to_move = (player_id*)malloc(1 * sizeof(player_id));
        bufs.concrete_moves = (move_data*)malloc(CHESS_MAX_MOVES * sizeof(move_data));
        bufs.results = (player_id*)malloc(1 * sizeof(player_id));
        bufs.move_str = (char*)malloc(6 * sizeof(char));
        bufs.print = (char*)malloc(256 * sizeof(char)); // calc proper size
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
    // save board
    int y = 7;
    int x = 0;
    for (int y = 7; y >= 0; y--) {
        int empty_squares = 0;
        for (int x = 0; x < 8; x++) {
            if (data.board[y][x].player == CHESS_PLAYER_NONE) {
                // square is empty, count following empty squares and just print the number once
                empty_squares++;
            } else {
                // if the current square isnt empty, print its representation, before that print empty squares, if applicable
                if (empty_squares > 0) {
                    outbuf += sprintf(outbuf, "%d", empty_squares);
                    empty_squares = 0;
                }
                outbuf += sprintf(outbuf, "%c", CHESS_PIECE_TYPE_CHARS[data.board[y][x].type] + (data.board[y][x].player == CHESS_PLAYER_BLACK ? 32 : 0));
            }
        }
        if (empty_squares > 0) {
            outbuf += sprintf(outbuf, "%d", empty_squares);
        }
        if (y > 0) {
            outbuf += sprintf(outbuf, "/");
        }
    }
    // save current player
    switch (data.current_player) {
        case CHESS_PLAYER_NONE: {
            outbuf += sprintf(outbuf, " - ");
        } break;
        case CHESS_PLAYER_WHITE: {
            outbuf += sprintf(outbuf, " w ");
        } break;
        case CHESS_PLAYER_BLACK: {
            outbuf += sprintf(outbuf, " b ");
        } break;
        case CHESS_PLAYER_COUNT: {
            assert(0);
        }
    }
    // save castling rights
    if (!data.castling_white_king && !data.castling_white_queen && !data.castling_black_king && !data.castling_black_queen) {
        outbuf += sprintf(outbuf, "-");
    } else {
        if (data.castling_white_king) {
            outbuf += sprintf(outbuf, "K");
        }
        if (data.castling_white_queen) {
            outbuf += sprintf(outbuf, "Q");
        }
        if (data.castling_black_king) {
            outbuf += sprintf(outbuf, "k");
        }
        if (data.castling_black_queen) {
            outbuf += sprintf(outbuf, "q");
        }
    }
    // save enpassant target
    if (data.enpassant_target == 0xFF) {
        outbuf += sprintf(outbuf, " - ");
    } else {
        outbuf += sprintf(outbuf, " %c%c ", 'a' + ((data.enpassant_target >> 4) & 0x0F), '1' + (data.enpassant_target & 0x0F));
    }
    // save halfmove and fullmove clock
    outbuf += sprintf(outbuf, "%d %d", data.halfmove_clock, data.fullmove_clock);
    *ret_size = outbuf - ostr;
    *ret_str = bufs.state;
    return ERR_OK;
}

static error_code import_state_gf(game* self, const char* str)
{
    state_repr& data = get_repr(self);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            data.board[y][x] = CHESS_piece{CHESS_PLAYER_NONE, CHESS_PIECE_TYPE_NONE}; // reason for this being CHESS_PIECE_TYPE_KING ?
        }
    }
    if (str == NULL) {
        str = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; // load standard initial position
    }
    // defaults
    data.halfmove_clock = 0;
    data.fullmove_clock = 1;
    data.enpassant_target = 0xFF;
    data.current_player = CHESS_PLAYER_WHITE;
    data.winning_player = CHESS_PLAYER_NONE;
    // loads state from FEN strings
    int y = 7; // FEN starts on the top left of the board
    int x = 0;
    // loop through the pieces segment
    bool advance_segment = false;
    while (!advance_segment) {
        CHESS_PLAYER color = (*str > 96) ? CHESS_PLAYER_BLACK : CHESS_PLAYER_WHITE; // check if letter is lowercase
        switch (*str) {
            case 'k':
            case 'K': {
                if (x > 7 || y < 0) {
                    // out of bounds board
                    return ERR_INVALID_INPUT;
                }
                data.board[y][x++] = CHESS_piece{color, CHESS_PIECE_TYPE_KING};
            } break;
            case 'q':
            case 'Q': {
                if (x > 7 || y < 0) {
                    // out of bounds board
                    return ERR_INVALID_INPUT;
                }
                data.board[y][x++] = CHESS_piece{color, CHESS_PIECE_TYPE_QUEEN};
            } break;
            case 'r':
            case 'R': {
                if (x > 7 || y < 0) {
                    // out of bounds board
                    return ERR_INVALID_INPUT;
                }
                data.board[y][x++] = CHESS_piece{color, CHESS_PIECE_TYPE_ROOK};
            } break;
            case 'b':
            case 'B': {
                if (x > 7 || y < 0) {
                    // out of bounds board
                    return ERR_INVALID_INPUT;
                }
                data.board[y][x++] = CHESS_piece{color, CHESS_PIECE_TYPE_BISHOP};
            } break;
            case 'n':
            case 'N': {
                if (x > 7 || y < 0) {
                    // out of bounds board
                    return ERR_INVALID_INPUT;
                }
                data.board[y][x++] = CHESS_piece{color, CHESS_PIECE_TYPE_KNIGHT};
            } break;
            case 'p':
            case 'P': {
                if (x > 7 || y < 0) {
                    // out of bounds board
                    return ERR_INVALID_INPUT;
                }
                data.board[y][x++] = CHESS_piece{color, CHESS_PIECE_TYPE_PAWN};
            } break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8': { // place empty pieces
                for (int place_empty = (*str) - '0'; place_empty > 0; place_empty--) {
                    if (x > 7) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    data.board[y][x++] = CHESS_piece{CHESS_PLAYER_NONE, CHESS_PIECE_TYPE_NONE};
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
    // get active color
    switch (*str) {
        case '-': {
            data.current_player = CHESS_PLAYER_NONE;
        } break;
        case 'w': {
            data.current_player = CHESS_PLAYER_WHITE;
        } break;
        case 'b': {
            data.current_player = CHESS_PLAYER_BLACK;
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
    // get castling rights, only existing ones are printed, '-' if none, ordered as KQkq
    advance_segment = false;
    data.castling_white_king = false;
    data.castling_white_queen = false;
    data.castling_black_king = false;
    data.castling_black_queen = false;
    while (!advance_segment) {
        switch (*str) {
            case 'K': {
                data.castling_white_king = true;
            } break;
            case 'Q': {
                data.castling_white_queen = true;
            } break;
            case 'k': {
                data.castling_black_king = true;
            } break;
            case 'q': {
                data.castling_black_queen = true;
            } break;
            case '-': {
                advance_segment = true;
                str++;
                if (*str != ' ') {
                    return ERR_INVALID_INPUT;
                }
            } break;
            case ' ': {
                advance_segment = true;
            } break;
            default: {
                // failure, ran out of str to use or got invalid character
                return ERR_INVALID_INPUT;
            } break;
        }
        str++;
    }
    if (*str == '\0') {
        return ERR_INVALID_INPUT;
    }
    // get enpassant target
    if (*str == '-') {
        data.enpassant_target = 0xFF;
    } else {
        x = (*str) - 'a';
        if (x < 0 || x > 7) {
            return ERR_INVALID_INPUT;
        }
        str++;
        if (*str == '\0') {
            return ERR_INVALID_INPUT;
        }
        y = (*str) - '1';
        if (y < 0 || y > 7) {
            return ERR_INVALID_INPUT;
        }
        data.enpassant_target = (x << 4) | y;
    }
    str++;
    if (*str != ' ') {
        // failure, ran out of str to use or got invalid character
        return ERR_INVALID_INPUT;
    }
    str++;
    // get halfmove clock
    data.halfmove_clock = 0;
    while (true) {
        if (*str == '\0') {
            return ERR_INVALID_INPUT;
        }
        if ((*str) == ' ') {
            break;
        }
        data.halfmove_clock *= 10;
        uint32_t ladd = (*str) - '0';
        if (ladd > 9) {
            return ERR_INVALID_INPUT;
        }
        data.halfmove_clock += ladd;
        str++;
    }
    str++;
    // get fullmove clock
    data.fullmove_clock = 0;
    while (true) {
        if ((*str) == '\0') {
            break;
        }
        data.fullmove_clock *= 10;
        uint32_t ladd = (*str) - '0';
        if (ladd > 9) {
            return ERR_INVALID_INPUT;
        }
        data.fullmove_clock += ladd;
        str++;
    }
    return ERR_OK;
}

static error_code players_to_move_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    *ret_count = 1;
    state_repr& data = get_repr(self);
    player_id ptm = data.current_player;
    if (ptm == CHESS_PLAYER_NONE) {
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
    state_repr& data = get_repr(self);
    //TODO encode things like check in the move, for move string printing in ptn?
    //TODO this is an extremely slow and ugly movegen, use bitboards instead of mailbox
    // if game is over, return empty move list
    if (data.current_player == CHESS_PLAYER_NONE) {
        *ret_count = 0;
        return ERR_OK;
    }
    uint32_t pseudo_move_cnt;
    move_code pseudo_moves[CHESS_MAX_MOVES * 4]; //TODO calculate proper size for this
    get_moves_pseudo_legal_gf(self, &pseudo_move_cnt, pseudo_moves);
    // check move legality by checking king capture
    game pseudo_game;
    clone_gf(self, &pseudo_game);
    uint32_t move_cnt = 0;
    for (int i = 0; i < pseudo_move_cnt; i++) {
        copy_from_gf(&pseudo_game, self);
        apply_move_internal_gf(&pseudo_game, pseudo_moves[i], true);
        bool is_legal = true;
        uint32_t response_move_cnt;
        move_code response_moves[CHESS_MAX_MOVES];
        get_moves_pseudo_legal_gf(&pseudo_game, &response_move_cnt, response_moves);
        for (int j = 0; j < response_move_cnt; j++) {
            int cm_tx = (response_moves[j] >> 4) & 0x0F;
            int cm_ty = response_moves[j] & 0x0F;
            CHESS_piece pseudo_piece;
            get_cell_gf(&pseudo_game, cm_tx, cm_ty, &pseudo_piece);
            if (pseudo_piece.type == CHESS_PIECE_TYPE_KING) {
                is_legal = false;
                break;
            }
        }
        if (is_legal) {
            outbuf[move_cnt++] = game_e_create_move_small(pseudo_moves[i]);
        }
    }
    *ret_count = move_cnt;
    *ret_moves = bufs.concrete_moves;
    destroy_gf(&pseudo_game);
    return ERR_OK;
}

static error_code is_legal_move_gf(game* self, player_id player, move_data_sync move)
{
    //TODO use better detection
    if (game_e_move_sync_is_none(move) == true) {
        return ERR_INVALID_INPUT;
    }
    uint8_t ptm_count;
    const player_id* ptm;
    players_to_move_gf(self, &ptm_count, &ptm);
    if (*ptm != player) {
        return ERR_INVALID_INPUT;
    }
    uint32_t move_cnt;
    const move_data* moves;
    get_concrete_moves_gf(self, *ptm, &move_cnt, &moves);
    while (move_cnt > 0) {
        move_cnt--;
        if (moves[move_cnt].cl.code == move.md.cl.code) {
            return ERR_OK;
        }
    }
    return ERR_INVALID_INPUT;
}

static error_code make_move_gf(game* self, player_id player, move_data_sync move)
{
    state_repr& data = get_repr(self);
    apply_move_internal_gf(self, move.md.cl.code, false); // this swaps players after the move on its own
    //TODO draw on halfmove clock, should this happen here? probably just offer a move to claim draw, but for both players..
    //TODO does draw on threfold repetition happen here?
    //TODO better detection for win by checkmate and draw by stalemate
    uint32_t available_move_cnt;
    const move_data* available_moves_data;
    get_concrete_moves_gf(self, data.current_player, &available_move_cnt, &available_moves_data);
    move_code available_moves_code[CHESS_MAX_MOVES];
    if (available_move_cnt == 0) {
        // this is at least a stalemate here, now see if the other player would have a way to capture the king next turn
        data.current_player = data.current_player == CHESS_PLAYER_WHITE ? CHESS_PLAYER_BLACK : CHESS_PLAYER_WHITE;
        get_moves_pseudo_legal_gf(self, &available_move_cnt, available_moves_code);
        for (int i = 0; i < available_move_cnt; i++) {
            int cm_tx = (available_moves_code[i] >> 4) & 0x0F;
            int cm_ty = available_moves_code[i] & 0x0F;
            if (data.board[cm_ty][cm_tx].type == CHESS_PIECE_TYPE_KING) {
                data.winning_player = data.current_player;
                data.current_player = CHESS_PLAYER_NONE;
                return ERR_OK;
            }
        }
        data.current_player = CHESS_PLAYER_NONE;
        data.winning_player = CHESS_PLAYER_NONE;
        return ERR_OK;
    }
    return ERR_OK;
}

static error_code get_results_gf(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    export_buffers& bufs = get_bufs(self);
    player_id* outbuf = bufs.results;
    *ret_count = 1;
    state_repr& data = get_repr(self);
    if (data.current_player != CHESS_PLAYER_NONE) {
        *ret_count = 0;
        return ERR_OK;
    }
    *outbuf = data.winning_player;
    *ret_players = outbuf;
    return ERR_OK;
}

static error_code id_gf(game* self, uint64_t* ret_id)
{
    //TODO use proper zobrist
    state_repr& data = get_repr(self);
    uint32_t state_noise = strhash((char*)self->data1, (char*)self->data1 + sizeof(state_repr));
    *ret_id = ((uint64_t)state_noise << 32) | (uint64_t)squirrelnoise5(state_noise, state_noise);
    return ERR_OK;
}

static error_code get_move_data_gf(game* self, player_id player, const char* str, move_data_sync** ret_move)
{
    export_buffers& bufs = get_bufs(self);
    size_t str_len = strlen(str);
    if (str_len >= 1 && str[0] == '-') {
        bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
        *ret_move = &bufs.move_out;
        return ERR_INVALID_INPUT;
    }
    if (str_len < 4 || str_len > 5) {
        bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
        *ret_move = &bufs.move_out;
        return ERR_INVALID_INPUT;
    }
    int ox = str[0] - 'a';
    int oy = str[1] - '1';
    int tx = str[2] - 'a';
    int ty = str[3] - '1';
    if (ox < 0 || ox > 7 || oy < 0 || oy > 7 || tx < 0 || tx > 7 || ty < 0 || ty > 7) {
        bufs.move_out = game_e_create_move_sync_small(self, MOVE_NONE);
        *ret_move = &bufs.move_out;
        return ERR_INVALID_INPUT;
    }
    CHESS_PIECE_TYPE promotion = CHESS_PIECE_TYPE_NONE;
    if (str_len == 5) {
        // encode promotion
        switch (str[4]) {
            case 'q': {
                promotion = CHESS_PIECE_TYPE_QUEEN;
            } break;
            case 'r': {
                promotion = CHESS_PIECE_TYPE_ROOK;
            } break;
            case 'b': {
                promotion = CHESS_PIECE_TYPE_BISHOP;
            } break;
            case 'n': {
                promotion = CHESS_PIECE_TYPE_KNIGHT;
            } break;
            default: {
                return ERR_INVALID_INPUT;
            } break;
        }
    }
    bufs.move_out = game_e_create_move_sync_small(self, (promotion << 16) | (ox << 12) | (oy << 8) | (tx << 4) | (ty));
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
    int ox = (mcode >> 12) & 0x0F;
    int oy = (mcode >> 8) & 0x0F;
    int tx = (mcode >> 4) & 0x0F;
    int ty = mcode & 0x0F;
    CHESS_PIECE_TYPE promotion = static_cast<CHESS_PIECE_TYPE>((mcode >> 16) & 0x0F);
    *ret_size = sprintf(outbuf, "%c%c%c%c", 'a' + ox, '1' + oy, 'a' + tx, '1' + ty);
    if (promotion != CHESS_PIECE_TYPE_NONE) {
        *ret_size += sprintf(outbuf + 4, "%c", CHESS_PIECE_TYPE_CHARS[promotion] + 32);
    }
    *ret_str = bufs.move_str;
    return ERR_OK;
}

static error_code print_gf(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    export_buffers& bufs = get_bufs(self);
    char* outbuf = bufs.print;
    state_repr& data = get_repr(self);
    const char* ostr = outbuf;
    outbuf += sprintf(outbuf, "castling rights: ");
    outbuf += sprintf(outbuf, data.castling_white_king ? "K" : "-");
    outbuf += sprintf(outbuf, data.castling_white_queen ? "Q" : "-");
    outbuf += sprintf(outbuf, data.castling_black_king ? "k" : "-");
    outbuf += sprintf(outbuf, data.castling_black_queen ? "q" : "-");
    outbuf += sprintf(outbuf, "\n");
    outbuf += sprintf(outbuf, "en passant target: ");
    if (data.enpassant_target == 0xFF) {
        outbuf += sprintf(outbuf, "--");
    } else {
        outbuf += sprintf(outbuf, "%c%c", 'a' + ((data.enpassant_target >> 4) & 0x0F), '1' + (data.enpassant_target & 0x0F));
    }
    outbuf += sprintf(outbuf, "\n\n");
    for (int y = 7; y >= 0; y--) {
        outbuf += sprintf(outbuf, "%c ", '1' + y);
        for (int x = 0; x < 8; x++) {
            if (data.board[y][x].player == CHESS_PLAYER_NONE) {
                outbuf += sprintf(outbuf, "%c", CHESS_PIECE_TYPE_CHARS[CHESS_PIECE_TYPE_NONE]);
                continue;
            }
            // add 32 to make piece chars lower case for black
            outbuf += sprintf(outbuf, "%c", CHESS_PIECE_TYPE_CHARS[data.board[y][x].type] + (data.board[y][x].player == CHESS_PLAYER_BLACK ? 32 : 0));
        }
        outbuf += sprintf(outbuf, "\n");
    }
    outbuf += sprintf(outbuf, "\n  ");
    for (int x = 0; x < 8; x++) {
        outbuf += sprintf(outbuf, "%c", 'a' + x);
    }
    outbuf += sprintf(outbuf, "\n");
    *ret_size = outbuf - ostr;
    *ret_str = bufs.print;
    return ERR_OK;
}

//=====
// game internal methods

static error_code get_cell_gf(game* self, int x, int y, CHESS_piece* p)
{
    state_repr& data = get_repr(self);
    *p = data.board[y][x];
    return ERR_OK;
}

static error_code set_cell_gf(game* self, int x, int y, CHESS_piece p)
{
    state_repr& data = get_repr(self);
    data.board[y][x] = p;
    return ERR_OK;
}

static error_code set_current_player_gf(game* self, player_id p)
{
    state_repr& data = get_repr(self);
    data.current_player = (CHESS_PLAYER)p;
    return ERR_OK;
}

static error_code set_result_gf(game* self, player_id p)
{
    state_repr& data = get_repr(self);
    data.winning_player = (CHESS_PLAYER)p;
    return ERR_OK;
}

static error_code count_positions_gf(game* self, int depth, uint64_t* count)
{
    // this chess implementation is valid against all chessprogrammingwiki positions, tested up to depth 4
    state_repr& data = get_repr(self);
    uint32_t move_cnt;
    const move_data* moves;
    get_concrete_moves_gf(self, data.current_player, &move_cnt, &moves);
    if (depth == 1) {
        *count = move_cnt; // bulk counting since get_moves generates only legal moves
    }
    uint64_t positions = 0;
    game test_game;
    clone_gf(self, &test_game);
    for (int i = 0; i < move_cnt; i++) {
        copy_from_gf(&test_game, self);
        apply_move_internal_gf(&test_game, moves[i].cl.code, false);
        uint64_t test_game_positions;
        count_positions_gf(&test_game, depth - 1, &test_game_positions);
        positions += test_game_positions;
    }
    *count = positions;
    destroy_gf(&test_game);
    return ERR_OK;
}

static error_code apply_move_internal_gf(game* self, move_code move, bool replace_castling_by_kings)
{
    state_repr& data = get_repr(self);
    int ox = (move >> 12) & 0x0F;
    int oy = (move >> 8) & 0x0F;
    int tx = (move >> 4) & 0x0F;
    int ty = move & 0x0F;
    data.board[ty][tx] = data.board[oy][ox];
    data.board[oy][ox] = CHESS_piece{CHESS_PLAYER_NONE, CHESS_PIECE_TYPE_NONE};
    // if move is enpassant capture, also remove the double pushed pawn
    if (data.enpassant_target == (move & 0xFF) && data.board[ty][tx].type == CHESS_PIECE_TYPE_PAWN) {
        data.board[oy][ox + (tx - ox)] = CHESS_piece{CHESS_PLAYER_NONE, CHESS_PIECE_TYPE_NONE};
    }
    // enpassant caputure is only valid immediately after the double push
    data.enpassant_target = 0xFF;
    // set new enpassant target on double pushed pawn
    // piece is pawn and has moved vertically more than 1 square
    if (data.board[ty][tx].type == CHESS_PIECE_TYPE_PAWN) {
        if (ty - oy > 1) {
            data.enpassant_target = (tx << 4) | (oy + 1);
        }
        if (oy - ty > 1) {
            data.enpassant_target = (tx << 4) | (ty + 1);
        }
    }
    // perform castling
    // piece is king and has moved horizontally more than one
    if (data.board[ty][tx].type == CHESS_PIECE_TYPE_KING) {
        if (tx - ox > 1) { // kingside
            data.board[ty][ox + 1] = data.board[ty][7];
            if (replace_castling_by_kings) {
                data.board[ty][ox + 1] = CHESS_piece{data.board[ty][7].player, CHESS_PIECE_TYPE_KING};
                data.board[oy][ox] = CHESS_piece{data.board[ty][7].player, CHESS_PIECE_TYPE_KING};
            }
            data.board[ty][7] = CHESS_piece{CHESS_PLAYER_NONE, CHESS_PIECE_TYPE_NONE};
        }
        if (ox - tx > 1) { // queenside
            data.board[ty][ox - 1] = data.board[ty][0];
            if (replace_castling_by_kings) {
                data.board[ty][ox - 1] = CHESS_piece{data.board[ty][0].player, CHESS_PIECE_TYPE_KING};
                data.board[oy][ox] = CHESS_piece{data.board[ty][0].player, CHESS_PIECE_TYPE_KING};
            }
            data.board[ty][0] = CHESS_piece{CHESS_PLAYER_NONE, CHESS_PIECE_TYPE_NONE};
        }
    }
    // pawn promotion
    // piece is pawn and target is y=0 or y=7, promote to supplied type
    if (data.board[ty][tx].type == CHESS_PIECE_TYPE_PAWN && (ty == 0 || ty == 7)) {
        data.board[ty][tx].type = static_cast<CHESS_PIECE_TYPE>((move >> 16) & 0x0F);
    }
    // revoke castling rights if any exist
    if (data.castling_white_king || data.castling_white_queen || data.castling_black_king || data.castling_black_queen) {
        if (data.board[ty][tx].type == CHESS_PIECE_TYPE_KING) {
            if (data.current_player == CHESS_PLAYER_WHITE) {
                data.castling_white_king = false;
                data.castling_white_queen = false;
            }
            if (data.current_player == CHESS_PLAYER_BLACK) {
                data.castling_black_king = false;
                data.castling_black_queen = false;
            }
        }
        if (data.board[ty][tx].type == CHESS_PIECE_TYPE_ROOK) {
            // check which side to revoke castling for
            if (ox == 0) {
                if (data.current_player == CHESS_PLAYER_WHITE) {
                    data.castling_white_queen = false;
                }
                if (data.current_player == CHESS_PLAYER_BLACK) {
                    data.castling_black_queen = false;
                }
            }
            if (ox == 7) {
                if (data.current_player == CHESS_PLAYER_WHITE) {
                    data.castling_white_king = false;
                }
                if (data.current_player == CHESS_PLAYER_BLACK) {
                    data.castling_black_king = false;
                }
            }
        }
    }
    // swap current player
    data.current_player = data.current_player == CHESS_PLAYER_WHITE ? CHESS_PLAYER_BLACK : CHESS_PLAYER_WHITE;
    return ERR_OK;
}

static error_code get_moves_pseudo_legal_gf(game* self, uint32_t* move_cnt, move_code* move_vec)
{
    state_repr& data = get_repr(self);
    // directions are: N,S,W,E,NW,SE,NE,SW
    const int directions_x[8] = {0, 0, -1, 1, -1, 1, 1, -1};
    const int directions_y[8] = {1, -1, 0, 0, 1, -1, 1, -1};
    // vectors are clockwise from the top
    const int knight_vx[8] = {1, 2, 2, 1, -1, -2, -2, -1};
    const int knight_vy[8] = {2, 1, -1, -2, -2, -1, 1, 2};
    // vector are: WHITE{NW,N,NE,2N}, BLACK{SW,S,SE,2S}
    const int pawn_cvx[8] = {-1, 0, 1, 0, -1, 0, 1, 0};
    const int pawn_cvy[8] = {1, 1, 1, 2, -1, -1, -1, -2};
    uint32_t gather_move_cnt = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            CHESS_piece current_piece = data.board[y][x];
            if (current_piece.player != data.current_player) {
                continue;
            }
            // gen move for this piece
            switch (current_piece.type) {
                case CHESS_PIECE_TYPE_NONE: {
                    // pass
                } break;
                case CHESS_PIECE_TYPE_KING: {
                    for (int d = 0; d < 8; d++) {
                        int tx = x + directions_x[d];
                        int ty = y + directions_y[d];
                        if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                            continue;
                        }
                        CHESS_piece target_piece = data.board[ty][tx];
                        if (target_piece.player == data.current_player) {
                            continue;
                        }
                        move_vec[gather_move_cnt++] = (x << 12) | (y << 8) | (tx << 4) | (ty);
                    }
                    // king is not allowed to move through attacked squares while castling, this is handled elsewhere for now
                    if (data.current_player == CHESS_PLAYER_WHITE) {
                        if (data.castling_white_king && data.board[y][x + 1].type == CHESS_PIECE_TYPE_NONE && data.board[y][x + 2].type == CHESS_PIECE_TYPE_NONE &&
                            data.board[y][7].player == data.current_player && data.board[y][7].type == CHESS_PIECE_TYPE_ROOK) {
                            move_vec[gather_move_cnt++] = (x << 12) | (y << 8) | ((x + 2) << 4) | (y);
                        }
                        if (data.castling_white_queen && data.board[y][x - 1].type == CHESS_PIECE_TYPE_NONE && data.board[y][x - 2].type == CHESS_PIECE_TYPE_NONE && data.board[y][x - 3].type == CHESS_PIECE_TYPE_NONE &&
                            data.board[y][0].player == data.current_player && data.board[y][0].type == CHESS_PIECE_TYPE_ROOK) {
                            move_vec[gather_move_cnt++] = (x << 12) | (y << 8) | ((x - 2) << 4) | (y);
                        }
                    }
                    if (data.current_player == CHESS_PLAYER_BLACK) {
                        if (data.castling_black_king && data.board[y][x + 1].type == CHESS_PIECE_TYPE_NONE && data.board[y][x + 2].type == CHESS_PIECE_TYPE_NONE &&
                            data.board[y][7].player == data.current_player && data.board[y][7].type == CHESS_PIECE_TYPE_ROOK) {
                            move_vec[gather_move_cnt++] = (x << 12) | (y << 8) | ((x + 2) << 4) | (y);
                        }
                        if (data.castling_black_queen && data.board[y][x - 1].type == CHESS_PIECE_TYPE_NONE && data.board[y][x - 2].type == CHESS_PIECE_TYPE_NONE && data.board[y][x - 3].type == CHESS_PIECE_TYPE_NONE &&
                            data.board[y][0].player == data.current_player && data.board[y][0].type == CHESS_PIECE_TYPE_ROOK) {
                            move_vec[gather_move_cnt++] = (x << 12) | (y << 8) | ((x - 2) << 4) | (y);
                        }
                    }
                } break;
                case CHESS_PIECE_TYPE_QUEEN:
                case CHESS_PIECE_TYPE_ROOK:
                case CHESS_PIECE_TYPE_BISHOP: {
                    int dmin = current_piece.type == CHESS_PIECE_TYPE_BISHOP ? 4 : 0;
                    int dmax = current_piece.type == CHESS_PIECE_TYPE_ROOK ? 4 : 8;
                    for (int d = dmin; d < dmax; d++) {
                        for (int s = 1; true; s++) {
                            int tx = x + s * directions_x[d];
                            int ty = y + s * directions_y[d];
                            if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                                break;
                            }
                            CHESS_piece target_piece = data.board[ty][tx];
                            if (target_piece.player == data.current_player) {
                                break;
                            }
                            move_vec[gather_move_cnt++] = (x << 12) | (y << 8) | (tx << 4) | (ty);
                            if (target_piece.player != current_piece.player && target_piece.type != CHESS_PIECE_TYPE_NONE) {
                                break;
                            }
                        }
                    }
                } break;
                case CHESS_PIECE_TYPE_KNIGHT: {
                    for (int d = 0; d < 8; d++) {
                        int tx = x + knight_vx[d];
                        int ty = y + knight_vy[d];
                        if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                            continue;
                        }
                        CHESS_piece target_piece = data.board[ty][tx];
                        if (target_piece.player == data.current_player) {
                            continue;
                        }
                        move_vec[gather_move_cnt++] = (x << 12) | (y << 8) | (tx << 4) | (ty);
                    }
                } break;
                case CHESS_PIECE_TYPE_PAWN: {
                    int dmin = data.current_player == CHESS_PLAYER_BLACK ? 4 : 0;
                    int dmax = data.current_player == CHESS_PLAYER_WHITE ? 4 : 8;
                    for (int d = dmin; d < dmax; d++) {
                        int tx = x + pawn_cvx[d];
                        int ty = y + pawn_cvy[d];
                        if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                            continue;
                        }
                        CHESS_piece target_piece = data.board[ty][tx];
                        if (target_piece.player == data.current_player) {
                            continue;
                        }
                        if (tx != x && target_piece.type == CHESS_PIECE_TYPE_NONE && data.enpassant_target != ((tx << 4) | (ty))) {
                            // pawn would move diagonally, but there isnt any piece there to capture, and it also isnt an en passant target
                            continue;
                        }
                        if (y != 1 && y != 6 && (ty - y > 1 || y - ty > 1)) {
                            // do not allow double pawn push if it isnt in its starting position
                            continue;
                        }
                        if ((y == 1 || y == 6) && (ty - y > 1 || y - ty > 1) && data.board[y + pawn_cvy[d - 2]][tx].type != CHESS_PIECE_TYPE_NONE) {
                            // double pawn push only allowed over empty squares
                            continue;
                        }
                        if (tx - x == 0 && target_piece.type != CHESS_PIECE_TYPE_NONE) {
                            // can only advance straight forward into open spaces
                            continue;
                        }
                        if (ty == 0 || ty == 7) {
                            // pawn promotion, instead add 4 moves for the 4 types of promotions
                            move_vec[gather_move_cnt++] = (CHESS_PIECE_TYPE_QUEEN << 16) | (x << 12) | (y << 8) | (tx << 4) | (ty);
                            move_vec[gather_move_cnt++] = (CHESS_PIECE_TYPE_ROOK << 16) | (x << 12) | (y << 8) | (tx << 4) | (ty);
                            move_vec[gather_move_cnt++] = (CHESS_PIECE_TYPE_BISHOP << 16) | (x << 12) | (y << 8) | (tx << 4) | (ty);
                            move_vec[gather_move_cnt++] = (CHESS_PIECE_TYPE_KNIGHT << 16) | (x << 12) | (y << 8) | (tx << 4) | (ty);
                            continue;
                        }
                        move_vec[gather_move_cnt++] = (x << 12) | (y << 8) | (tx << 4) | (ty);
                    }
                } break;
                case CHESS_PIECE_TYPE_COUNT: {
                    assert(0);
                } break;
            }
        }
    }
    *move_cnt = gather_move_cnt;
    return ERR_OK;
}

#ifdef __cplusplus
}
#endif
