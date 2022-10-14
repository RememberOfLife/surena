#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "surena/util/noise.h"
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/chess.h"

namespace {

    // general purpose helpers for opts, data

    struct data_repr {
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

    data_repr& get_repr(game* self)
    {
        return *((data_repr*)(self->data1));
    }

    // forward declare everything to allow for inlining at least in this unit
    const char* get_last_error(game* self);
    GF_UNUSED(create_with_opts_str);
    GF_UNUSED(create_with_opts_bin);
    GF_UNUSED(create_deserialize);
    error_code create_default(game* self);
    GF_UNUSED(export_options_str);
    GF_UNUSED(get_options_bin_ref);
    error_code destroy(game* self);
    error_code clone(game* self, game* clone_target);
    error_code copy_from(game* self, game* other);
    error_code compare(game* self, game* other, bool* ret_equal);
    error_code import_state(game* self, const char* str);
    error_code export_state(game* self, size_t* ret_size, char* str);
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
    GF_UNUSED(get_sync_counter);
    error_code id(game* self, uint64_t* ret_id);
    GF_UNUSED(eval);
    GF_UNUSED(discretize);
    GF_UNUSED(playout);
    GF_UNUSED(redact_keep_state);
    GF_UNUSED(export_sync_data);
    GF_UNUSED(release_sync_data);
    GF_UNUSED(import_sync_data);
    error_code get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    error_code get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    error_code debug_print(game* self, size_t* ret_size, char* str_buf);

    error_code get_cell(game* self, int x, int y, CHESS_piece* p);
    error_code set_cell(game* self, int x, int y, CHESS_piece p);
    error_code set_current_player(game* self, player_id p);
    error_code set_result(game* self, player_id p);
    error_code count_positions(game* self, int depth, uint64_t* count);
    error_code apply_move_internal(game* self, move_code move, bool replace_castling_by_kings);
    error_code get_moves_pseudo_legal(game* self, uint32_t* move_cnt, move_code* move_vec);

    // implementation

    const char* get_last_error(game* self)
    {
        return (char*)self->data2;
    }

    error_code create_default(game* self)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        self->data2 = NULL;
        self->sizer = (buf_sizer){
            .state_str = 128,
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = CHESS_MAX_MOVES,
            .max_results = 1,
            .move_str = 6,
            .print_str = 256, // calc proper size
        };
        return ERR_OK;
    }

    error_code destroy(game* self)
    {
        free(self->data1);
        self->data1 = NULL;
        free(self->data2);
        self->data2 = NULL;
        return ERR_OK;
    }

    error_code clone(game* self, game* clone_target)
    {
        if (clone_target == NULL) {
            return ERR_INVALID_INPUT;
        }
        *clone_target = *self;
        error_code ec = clone_target->methods->create_default(clone_target);
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

    error_code import_state(game* self, const char* str)
    {
        data_repr& data = get_repr(self);
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

    error_code export_state(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
        const char* ostr = str;
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
                        str += sprintf(str, "%d", empty_squares);
                        empty_squares = 0;
                    }
                    str += sprintf(str, "%c", CHESS_PIECE_TYPE_CHARS[data.board[y][x].type] + (data.board[y][x].player == CHESS_PLAYER_BLACK ? 32 : 0));
                }
            }
            if (empty_squares > 0) {
                str += sprintf(str, "%d", empty_squares);
            }
            if (y > 0) {
                str += sprintf(str, "/");
            }
        }
        // save current player
        switch (data.current_player) {
            case CHESS_PLAYER_NONE: {
                str += sprintf(str, " - ");
            } break;
            case CHESS_PLAYER_WHITE: {
                str += sprintf(str, " w ");
            } break;
            case CHESS_PLAYER_BLACK: {
                str += sprintf(str, " b ");
            } break;
        }
        // save castling rights
        if (!data.castling_white_king && !data.castling_white_queen && !data.castling_black_king && !data.castling_black_queen) {
            str += sprintf(str, "-");
        } else {
            if (data.castling_white_king) {
                str += sprintf(str, "K");
            }
            if (data.castling_white_queen) {
                str += sprintf(str, "Q");
            }
            if (data.castling_black_king) {
                str += sprintf(str, "k");
            }
            if (data.castling_black_queen) {
                str += sprintf(str, "q");
            }
        }
        // save enpassant target
        if (data.enpassant_target == 0xFF) {
            str += sprintf(str, " - ");
        } else {
            str += sprintf(str, " %c%c ", 'a' + ((data.enpassant_target >> 4) & 0x0F), '1' + (data.enpassant_target & 0x0F));
        }
        // save halfmove and fullmove clock
        str += sprintf(str, "%d %d", data.halfmove_clock, data.fullmove_clock);
        *ret_size = str - ostr;
        return ERR_OK;
    }

    error_code players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = get_repr(self);
        if (data.current_player == CHESS_PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = data.current_player;
        return ERR_OK;
    }

    error_code get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        if (moves == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
        //TODO encode things like check in the move, for move string printing in ptn?
        //TODO this is an extremely slow and ugly movegen, use bitboards instead of mailbox
        // if game is over, return empty move list
        if (data.current_player == CHESS_PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        uint32_t pseudo_move_cnt;
        move_code pseudo_moves[CHESS_MAX_MOVES * 4]; //TODO calculate proper size for this
        get_moves_pseudo_legal(self, &pseudo_move_cnt, pseudo_moves);
        // check move legality by checking king capture
        game pseudo_game;
        clone(self, &pseudo_game);
        uint32_t move_cnt = 0;
        for (int i = 0; i < pseudo_move_cnt; i++) {
            pseudo_game.methods->copy_from(&pseudo_game, self);
            apply_move_internal(&pseudo_game, pseudo_moves[i], true);
            bool is_legal = true;
            uint32_t response_move_cnt;
            move_code response_moves[CHESS_MAX_MOVES];
            get_moves_pseudo_legal(&pseudo_game, &response_move_cnt, response_moves);
            for (int j = 0; j < response_move_cnt; j++) {
                int cm_tx = (response_moves[j] >> 4) & 0x0F;
                int cm_ty = response_moves[j] & 0x0F;
                CHESS_piece pseudo_piece;
                get_cell(&pseudo_game, cm_tx, cm_ty, &pseudo_piece);
                if (pseudo_piece.type == CHESS_PIECE_TYPE_KING) {
                    is_legal = false;
                    break;
                }
            }
            if (is_legal) {
                moves[move_cnt++] = pseudo_moves[i];
            }
        }
        *ret_count = move_cnt;
        destroy(&pseudo_game);
        return ERR_OK;
    }

    error_code is_legal_move(game* self, player_id player, move_code move, sync_counter sync)
    {
        //TODO use better detection
        if (move == MOVE_NONE) {
            return ERR_INVALID_INPUT;
        }
        player_id ptm;
        uint8_t ptm_count;
        players_to_move(self, &ptm_count, &ptm);
        if (ptm != player) {
            return ERR_INVALID_INPUT;
        }
        uint32_t move_cnt;
        move_code moves[CHESS_MAX_MOVES];
        get_concrete_moves(self, PLAYER_NONE, &move_cnt, moves);
        while (move_cnt > 0) {
            move_cnt--;
            if (moves[move_cnt] == move) {
                return ERR_OK;
            }
        }
        return ERR_INVALID_INPUT;
    }

    error_code make_move(game* self, player_id player, move_code move)
    {
        data_repr& data = get_repr(self);
        apply_move_internal(self, move, false); // this swaps players after the move on its own
        //TODO draw on halfmove clock, should this happen here? probably just offer a move to claim draw, but for both players..
        //TODO does draw on threfold repetition happen here?
        //TODO better detection for win by checkmate and draw by stalemate
        uint32_t available_move_cnt;
        move_code available_moves[CHESS_MAX_MOVES];
        get_concrete_moves(self, PLAYER_NONE, &available_move_cnt, available_moves);
        if (available_move_cnt == 0) {
            // this is at least a stalemate here, now see if the other player would have a way to capture the king next turn
            data.current_player = data.current_player == CHESS_PLAYER_WHITE ? CHESS_PLAYER_BLACK : CHESS_PLAYER_WHITE;
            get_moves_pseudo_legal(self, &available_move_cnt, available_moves);
            for (int i = 0; i < available_move_cnt; i++) {
                int cm_tx = (available_moves[i] >> 4) & 0x0F;
                int cm_ty = available_moves[i] & 0x0F;
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

    error_code get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = get_repr(self);
        if (data.current_player != CHESS_PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = data.winning_player;
        return ERR_OK;
    }

    error_code id(game* self, uint64_t* ret_id)
    {
        //TODO use proper zobrist
        data_repr& data = get_repr(self);
        uint32_t state_noise = strhash((char*)self->data1, (char*)self->data1 + sizeof(data_repr));
        *ret_id = ((uint64_t)state_noise << 32) | (uint64_t)squirrelnoise5(state_noise, state_noise);
        return ERR_OK;
    }

    error_code get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        size_t str_len = strlen(str);
        if (str_len >= 1 && str[0] == '-') {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        if (str_len < 4 || str_len > 5) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        int ox = str[0] - 'a';
        int oy = str[1] - '1';
        int tx = str[2] - 'a';
        int ty = str[3] - '1';
        if (ox < 0 || ox > 7 || oy < 0 || oy > 7 || tx < 0 || tx > 7 || ty < 0 || ty > 7) {
            *ret_move = MOVE_NONE;
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
        *ret_move = (promotion << 16) | (ox << 12) | (oy << 8) | (tx << 4) | (ty);
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
        int ox = (move >> 12) & 0x0F;
        int oy = (move >> 8) & 0x0F;
        int tx = (move >> 4) & 0x0F;
        int ty = move & 0x0F;
        CHESS_PIECE_TYPE promotion = static_cast<CHESS_PIECE_TYPE>((move >> 16) & 0x0F);
        *ret_size = sprintf(str_buf, "%c%c%c%c", 'a' + ox, '1' + oy, 'a' + tx, '1' + ty);
        if (promotion != CHESS_PIECE_TYPE_NONE) {
            *ret_size += sprintf(str_buf + 4, "%c", CHESS_PIECE_TYPE_CHARS[promotion] + 32);
        }
        return ERR_OK;
    }

    error_code debug_print(game* self, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = get_repr(self);
        const char* ostr = str_buf;
        str_buf += sprintf(str_buf, "castling rights: ");
        str_buf += sprintf(str_buf, data.castling_white_king ? "K" : "-");
        str_buf += sprintf(str_buf, data.castling_white_queen ? "Q" : "-");
        str_buf += sprintf(str_buf, data.castling_black_king ? "k" : "-");
        str_buf += sprintf(str_buf, data.castling_black_queen ? "q" : "-");
        str_buf += sprintf(str_buf, "\n");
        str_buf += sprintf(str_buf, "en passant target: ");
        if (data.enpassant_target == 0xFF) {
            str_buf += sprintf(str_buf, "--");
        } else {
            str_buf += sprintf(str_buf, "%c%c", 'a' + ((data.enpassant_target >> 4) & 0x0F), '1' + (data.enpassant_target & 0x0F));
        }
        str_buf += sprintf(str_buf, "\n\n");
        for (int y = 7; y >= 0; y--) {
            str_buf += sprintf(str_buf, "%c ", '1' + y);
            for (int x = 0; x < 8; x++) {
                if (data.board[y][x].player == CHESS_PLAYER_NONE) {
                    str_buf += sprintf(str_buf, "%c", CHESS_PIECE_TYPE_CHARS[CHESS_PIECE_TYPE_NONE]);
                    continue;
                }
                // add 32 to make piece chars lower case for black
                str_buf += sprintf(str_buf, "%c", CHESS_PIECE_TYPE_CHARS[data.board[y][x].type] + (data.board[y][x].player == CHESS_PLAYER_BLACK ? 32 : 0));
            }
            str_buf += sprintf(str_buf, "\n");
        }
        str_buf += sprintf(str_buf, "\n  ");
        for (int x = 0; x < 8; x++) {
            str_buf += sprintf(str_buf, "%c", 'a' + x);
        }
        str_buf += sprintf(str_buf, "\n");
        *ret_size = str_buf - ostr;
        return ERR_OK;
    }

    //=====
    // game internal methods

    error_code get_cell(game* self, int x, int y, CHESS_piece* p)
    {
        data_repr& data = get_repr(self);
        *p = data.board[y][x];
        return ERR_OK;
    }

    error_code set_cell(game* self, int x, int y, CHESS_piece p)
    {
        data_repr& data = get_repr(self);
        data.board[y][x] = p;
        return ERR_OK;
    }

    error_code set_current_player(game* self, player_id p)
    {
        data_repr& data = get_repr(self);
        data.current_player = (CHESS_PLAYER)p;
        return ERR_OK;
    }

    error_code set_result(game* self, player_id p)
    {
        data_repr& data = get_repr(self);
        data.winning_player = (CHESS_PLAYER)p;
        return ERR_OK;
    }

    error_code count_positions(game* self, int depth, uint64_t* count)
    {
        // this chess implementation is valid against all chessprogrammingwiki positions, tested up to depth 4
        uint32_t move_cnt;
        move_code moves[CHESS_MAX_MOVES];
        get_concrete_moves(self, PLAYER_NONE, &move_cnt, moves);
        if (depth == 1) {
            *count = move_cnt; // bulk counting since get_moves generates only legal moves
        }
        uint64_t positions = 0;
        game test_game;
        clone(self, &test_game);
        for (int i = 0; i < move_cnt; i++) {
            copy_from(&test_game, self);
            apply_move_internal(&test_game, moves[i], false);
            uint64_t test_game_positions;
            count_positions(&test_game, depth - 1, &test_game_positions);
            positions += test_game_positions;
        }
        *count = positions;
        destroy(&test_game);
        return ERR_OK;
    }

    error_code apply_move_internal(game* self, move_code move, bool replace_castling_by_kings)
    {
        data_repr& data = get_repr(self);
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

    error_code get_moves_pseudo_legal(game* self, uint32_t* move_cnt, move_code* move_vec)
    {
        data_repr& data = get_repr(self);
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
                }
            }
        }
        *move_cnt = gather_move_cnt;
        return ERR_OK;
    }

} // namespace

const char CHESS_PIECE_TYPE_CHARS[7] = {'-', 'K', 'Q', 'R', 'B', 'N', 'P'}; // none, king, queen, rook, bishop, knight, pawn

static const chess_internal_methods chess_gbe_internal_methods{
    .get_cell = get_cell,
    .set_cell = set_cell,
    .set_current_player = set_current_player,
    .set_result = set_result,
    .count_positions = count_positions,
    .apply_move_internal = apply_move_internal,
    .get_moves_pseudo_legal = get_moves_pseudo_legal,
};

const game_methods chess_gbe{

    .game_name = "Chess",
    .variant_name = "Standard",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .options = false,
        .options_bin = false,
        .options_bin_ref = false,
        .serializable = false,
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = false,
        .sync_counter = false,
        .move_ordering = false,
        .id = true,
        .eval = false,
        .playout = false,
        .print = true,
    },
    .internal_methods = (void*)&chess_gbe_internal_methods,

#include "surena/game_impl.h"

};
