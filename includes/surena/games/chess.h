#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "surena/game.h"

const uint32_t CHESS_MAX_MOVES = 218;

enum CHESS_PLAYER : player_id {
    CHESS_PLAYER_NONE = 0,
    CHESS_PLAYER_WHITE,
    CHESS_PLAYER_BLACK,
};

enum CHESS_PIECE_TYPE : uint8_t {
    CHESS_PIECE_TYPE_NONE = 0,
    CHESS_PIECE_TYPE_KING,
    CHESS_PIECE_TYPE_QUEEN,
    CHESS_PIECE_TYPE_ROOK,
    CHESS_PIECE_TYPE_BISHOP,
    CHESS_PIECE_TYPE_KNIGHT,
    CHESS_PIECE_TYPE_PAWN,
};

extern const char CHESS_PIECE_TYPE_CHARS[7];

typedef struct CHESS_piece_s {
    CHESS_PLAYER player : 2;
    CHESS_PIECE_TYPE type : 3;
} CHESS_piece;

typedef struct chess_internal_methods_s {

    // get piece value of cell (x grows right, y grows up)
    error_code (*get_cell)(game* self, int x, int y, CHESS_piece* p);
    error_code (*set_cell)(game* self, int x, int y, CHESS_piece p);
    error_code (*set_current_player)(game* self, player_id p);
    error_code (*set_result)(game* self, player_id p);
    // error_code (*get_check)(game* self, int* xy); //TODO

    error_code (*count_positions)(game* self, int depth, uint64_t* count); // simple perft

    error_code (*apply_move_internal)(game* self, move_code move, bool replace_castling_by_kings);
    error_code (*get_moves_pseudo_legal)(game* self, uint32_t* move_cnt, move_code* move_vec); // fills move, size assumed >CHESS_MAX_MOVES

} chess_internal_methods;

extern const game_methods chess_gbe;

#ifdef __cplusplus
}
#endif
