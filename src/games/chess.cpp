#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "surena/util/fast_prng.hpp"

#include "surena/games/chess.hpp"

namespace surena {

    const char Chess::PIECE_TYPE_CHARS[7] = {'-', 'K', 'Q', 'R', 'B', 'N', 'P'}; // none, king, queen, rook, bishop, knight, pawn

    Chess::Chess()
    {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                board[y][x] = piece{PLAYER_NONE, PIECE_TYPE_KING};
            }
        }
        import_state("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }

    void Chess::import_state(const char* str)
    {
        // loads state from FEN strings
        int y = 7; // FEN starts on the top left of the board
        int x = 0;
        // loop through the pieces segment
        bool advance_segment = false;
        while (!advance_segment) {
            PLAYER color = (*str > 96) ? PLAYER_BLACK : PLAYER_WHITE; // check if letter is lowercase
            switch (*str) {
                case 'k':
                case 'K': {
                    board[y][x++] = piece{color, PIECE_TYPE_KING};
                } break;
                case 'q':
                case 'Q': {
                    board[y][x++] = piece{color, PIECE_TYPE_QUEEN};
                } break;
                case 'r':
                case 'R': {
                    board[y][x++] = piece{color, PIECE_TYPE_ROOK};
                } break;
                case 'b':
                case 'B': {
                    board[y][x++] = piece{color, PIECE_TYPE_BISHOP};
                } break;
                case 'n':
                case 'N': {
                    board[y][x++] = piece{color, PIECE_TYPE_KNIGHT};
                } break;
                case 'p':
                case 'P': {
                    board[y][x++] = piece{color, PIECE_TYPE_PAWN};
                } break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8': { // place empty pieces
                    for (int place_empty = (*str)-'0'; place_empty > 0; place_empty--) {
                        board[y][x++] = piece{PLAYER_NONE, PIECE_TYPE_NONE};
                    }
                } break;
                case '/': { // advance to next
                    y--;
                    x = 0;
                } break;
                case ' ': { // advance to next segment
                    advance_segment = true;
                } break;
            }
            str++;
        }
        // get active color
        switch (*str) {
            case 'w': {
                current_player = PLAYER_WHITE;
            } break;
            case 'b': {
                current_player = PLAYER_BLACK;
            } break;
        }
        str += 2;
        // get castling rights, only existing ones are printed, '-' if none, ordered as KQkq
        advance_segment = false;
        castling_white_king = false;
        castling_white_queen = false;
        castling_black_king = false;
        castling_black_queen = false;
        while (!advance_segment) {
            switch (*str) {
                case 'K': {
                    castling_white_king = true;
                } break;
                case 'Q': {
                    castling_white_queen = true;
                } break;
                case 'k': {
                    castling_black_king = true;
                } break;
                case 'q': {
                    castling_black_queen = true;
                } break;
                case ' ': {
                    advance_segment = true;
                } break;
            }
            str++;
        }
        // get enpassant target
        if (*str == '-') {
            enpassant_target = 0xFF;
        } else {
            x = (*str)-'a';
            str++;
            y = (*str)-'1';
            enpassant_target = (x<<4)|y;
        }
        str += 2;
        // get halfmove clock
        halfmove_clock = 0;
        while (true) {
            if ((*str) == ' ') {
                break;
            }
            halfmove_clock *= 10;
            halfmove_clock += (*str)-'0';
            str++;
        }
        str++;
        // get fullmove clock
        fullmove_clock = 0;
        while (true) {
            if ((*str) == '\0') {
                break;
            }
            fullmove_clock *= 10;
            fullmove_clock += (*str)-'0';
            str++;
        }
    }

    uint32_t Chess::export_state(char* str)
    {
        if (str == NULL) {
            return 128;
        }
        const char* ostr = str;
        // save board
        int y = 7;
        int x = 0;
        for (int y = 7; y >= 0; y--) {
            int empty_squares = 0;
            for (int x = 0; x < 8; x++) {
                if (board[y][x].player == PLAYER_NONE) {
                    // square is empty, count following empty squares and just print the number once
                    empty_squares++;
                } else {
                    // if the current square isnt empty, print its representation, before that print empty squares, if applicable
                    if (empty_squares > 0) {
                        str += sprintf(str, "%d", empty_squares);
                        empty_squares = 0;
                    }
                    str += sprintf(str, "%c", PIECE_TYPE_CHARS[board[y][x].type] + (board[y][x].player == PLAYER_BLACK ? 32 : 0));
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
        switch (current_player) {
            case PLAYER_NONE: {
                str += sprintf(str, " - ");
            } break;
            case PLAYER_WHITE: {
                str += sprintf(str, " w ");
            } break;
            case PLAYER_BLACK: {
                str += sprintf(str, " b ");
            } break;
        }
        // save castling rights
        if (!castling_white_king && !castling_white_queen && !castling_black_king && !castling_black_queen) {
            str += sprintf(str, "-");
        } else {
            if (castling_white_king) {
                str += sprintf(str, "K");
            }
            if (castling_white_queen) {
                str += sprintf(str, "Q");
            }
            if (castling_black_king) {
                str += sprintf(str, "k");
            }
            if (castling_black_queen) {
                str += sprintf(str, "q");
            }
        }
        // save enpassant target
        if (enpassant_target == 0xFF) {
            str += sprintf(str, " - ");
        } else {
            str += sprintf(str, " %c%c ", 'a'+((enpassant_target>>4)&0x0F), '1'+(enpassant_target&0x0F));
        }
        // save halfmove and fullmove clock
        str += sprintf(str, "%d %d", halfmove_clock, fullmove_clock);
        return str-ostr;
    }

    uint8_t Chess::player_to_move()
    {
        return current_player;
    }

    std::vector<uint64_t> Chess::get_moves()
    {
        //TODO encode things like check in the move, for move string printing in ptn?
        //TODO this is an extremely slow and ugly movegen, use bitboards instead of mailbox
        std::vector<uint64_t> moves{};
        // if game is over, return empty move list
        if (player_to_move() == 0) {
            return moves;
        }
        moves = get_moves_pseudo_legal();
        // check move legality by checking king capture
        std::vector<uint64_t> legal_moves{};
        surena::Chess pseudo_game;
        for (int i = 0; i < moves.size(); i++) {
            pseudo_game.copy_from(this);
            pseudo_game.apply_move_internal(moves[i], true);
            bool is_legal = true;
            std::vector<uint64_t> response_moves = pseudo_game.get_moves_pseudo_legal();
            for (int j = 0; j < response_moves.size(); j++) {
                int cm_tx = (response_moves[j] >> 4) & 0x0F;
                int cm_ty = response_moves[j] & 0x0F;
                if (pseudo_game.get_cell(cm_tx, cm_ty).type == PIECE_TYPE_KING) {
                    is_legal = false;
                    break;
                }
            }
            if (is_legal) {
                legal_moves.push_back(moves[i]);
            }
        }
        return legal_moves;
    }

    void Chess::apply_move(uint64_t move_id)
    {
        apply_move_internal(move_id); // this swaps players after the move on its own
        //TODO draw on halfmove clock, should this happen here? probably just offer a move to claim draw, but for both players..
        //TODO does draw on threfold repetition happen here?
        //TODO better detection for win by checkmate and draw by stalemate
        if (get_moves().size() == 0) {
            // this is at least a stalemate here, now see if the other player would have a way to capture the king next turn
            current_player = current_player == PLAYER_WHITE ? PLAYER_BLACK : PLAYER_WHITE;
            std::vector<uint64_t> peudo_moves = get_moves_pseudo_legal();
            for (int i = 0; i < peudo_moves.size(); i++) {
                int cm_tx = (peudo_moves[i] >> 4) & 0x0F;
                int cm_ty = peudo_moves[i] & 0x0F;
                if (board[cm_ty][cm_tx].type == PIECE_TYPE_KING) {
                    winning_player = current_player;
                    current_player = PLAYER_NONE;
                    return;
                }
            }
            current_player = PLAYER_NONE;
            winning_player = RESULT_DRAW;
            return;
        }
    }

    uint8_t Chess::get_result()
    {
        return winning_player;
    }

    void Chess::discretize(uint64_t seed)
    {}

    uint8_t Chess::perform_playout(uint64_t seed)
    {
        fast_prng rng(seed);
        while (player_to_move() != 0) {
            std::vector<uint64_t> moves_available = get_moves();
            apply_move(moves_available[rng.rand()%moves_available.size()]);
        }
        return get_result();
    }

    surena::Game* Chess::clone() 
    {
        return new Chess(*this);
    }

    void Chess::copy_from(Game* target) 
    {
        *this = *static_cast<Chess*>(target);
    }
    
    uint64_t Chess::get_move_id(std::string move_string)
    {
        if (move_string.length() < 4 || move_string.length() > 5) {
            return UINT64_MAX;
        }
        int ox = move_string[0]-'a';
        int oy = move_string[1]-'1';
        int tx = move_string[2]-'a';
        int ty = move_string[3]-'1';
        PIECE_TYPE promotion = PIECE_TYPE_NONE;
        if (move_string.length() == 5) {
            // encode promotion
            switch (move_string[4]) {
                case 'q': {
                    promotion = PIECE_TYPE_QUEEN;
                } break;
                case 'r': {
                    promotion = PIECE_TYPE_ROOK;
                } break;
                case 'b': {
                    promotion = PIECE_TYPE_BISHOP;
                } break;
                case 'n': {
                    promotion = PIECE_TYPE_KNIGHT;
                } break;
            }
        }
        return (promotion<<16)|(ox<<12)|(oy<<8)|(tx<<4)|(ty);
    }

    std::string Chess::get_move_string(uint64_t move_id)
    {
        int ox = (move_id >> 12) & 0x0F;
        int oy = (move_id >> 8) & 0x0F;
        int tx = (move_id >> 4) & 0x0F;
        int ty = move_id & 0x0F;
        PIECE_TYPE promotion = static_cast<PIECE_TYPE>((move_id >> 16) & 0x0F);
        std::string move_string = "";
        move_string += ('a'+ox);
        move_string += ('1'+oy);
        move_string += ('a'+tx);
        move_string += ('1'+ty);
        if (promotion != PIECE_TYPE_NONE) {
            move_string += PIECE_TYPE_CHARS[promotion]+32;
        }
        return move_string;
    }

    void Chess::debug_print()
    {
        printf("##### debug print board #####\n");
        printf("castling rights: ");
        printf(castling_white_king ? "K" : "-");
        printf(castling_white_queen ? "Q" : "-");
        printf(castling_black_king ? "k" : "-");
        printf(castling_black_queen ? "q" : "-");
        printf("\n");
        printf("en passant target: ");
        if (enpassant_target == 0xFF) {
            printf("--");
        } else {
            printf("%c%c", 'a'+((enpassant_target>>4)&0x0F), '1'+(enpassant_target&0x0F));
        }
        printf("\n\n");
        for (int y = 7; y >= 0; y--) {
            printf("%c ", '1'+y);
            for (int x = 0; x < 8; x++) {
                if (board[y][x].player == PLAYER_NONE) {
                    printf("%c", PIECE_TYPE_CHARS[PIECE_TYPE_NONE]);
                    continue;
                }
                // add 32 to make piece chars lower case for black
                printf("%c", PIECE_TYPE_CHARS[board[y][x].type] + (board[y][x].player == PLAYER_BLACK ? 32 : 0));
            }
            printf("\n");
        }
        printf("\n  ");
        for (int x = 0; x < 8; x++) {
            printf("%c", 'a'+x);
        }
        printf("\n");
        printf("##### +++++ +++++ +++++ #####\n");
    }

    Chess::piece Chess::get_cell(int x, int y)
    {
        return board[y][x];
    }

    void Chess::set_cell(int x, int y, piece p)
    {
        board[y][x] = p;
    }

    void Chess::set_current_player(PLAYER p)
    {
        current_player = p;
    }

    void Chess::set_result(PLAYER p)
    {
        winning_player = p;
    }

    void Chess::apply_move_internal(uint64_t move_id, bool replace_castling_by_kings)
    {
        int ox = (move_id >> 12) & 0x0F;
        int oy = (move_id >> 8) & 0x0F;
        int tx = (move_id >> 4) & 0x0F;
        int ty = move_id & 0x0F;
        board[ty][tx] = board[oy][ox];
        board[oy][ox] = piece{PLAYER_NONE, PIECE_TYPE_NONE};
        // if move is enpassant capture, also remove the double pushed pawn
        if (enpassant_target == (move_id&0xFF) && board[ty][tx].type == PIECE_TYPE_PAWN) {
            board[oy][ox+(tx-ox)] = piece{PLAYER_NONE, PIECE_TYPE_NONE};
        }
        // enpassant caputure is only valid immediately after the double push
        enpassant_target = 0xFF;
        // set new enpassant target on double pushed pawn
        // piece is pawn and has moved vertically more than 1 square
        if (board[ty][tx].type == PIECE_TYPE_PAWN) {
            if (ty-oy > 1) {
                enpassant_target = (tx<<4)|(oy+1);
            }
            if (oy-ty > 1) {
                enpassant_target = (tx<<4)|(ty+1);
            }
        }
        // perform castling
        // piece is king and has moved horizontally more than one 
        if (board[ty][tx].type == PIECE_TYPE_KING) {
            if (tx-ox > 1) { // kingside
                board[ty][ox+1] = board[ty][7];
                if (replace_castling_by_kings) {
                    board[ty][ox+1] = piece{board[ty][7].player, PIECE_TYPE_KING};
                    board[oy][ox] = piece{board[ty][7].player, PIECE_TYPE_KING};
                }
                board[ty][7] = piece{PLAYER_NONE, PIECE_TYPE_NONE};
            }
            if (ox-tx > 1) { // queenside
                board[ty][ox-1] = board[ty][0];
                if (replace_castling_by_kings) {
                    board[ty][ox-1] = piece{board[ty][0].player, PIECE_TYPE_KING};
                    board[oy][ox] = piece{board[ty][0].player, PIECE_TYPE_KING};
                }
                board[ty][0] = piece{PLAYER_NONE, PIECE_TYPE_NONE};
            }
        }
        // pawn promotion
        // piece is pawn and target is y=0 or y=7, promote to supplied type
        if (board[ty][tx].type == PIECE_TYPE_PAWN && (ty == 0 || ty == 7)) {
            board[ty][tx].type = static_cast<PIECE_TYPE>((move_id >> 16) & 0x0F);
        }
        // revoke castling rights if any exist
        if (castling_white_king || castling_white_queen || castling_black_king || castling_black_queen) {
            if (board[ty][tx].type == PIECE_TYPE_KING) {
                if (current_player == PLAYER_WHITE) {
                    castling_white_king = false;
                    castling_white_queen = false;
                }
                if (current_player == PLAYER_BLACK) {
                    castling_black_king = false;
                    castling_black_queen = false;
                }
            }
            if (board[ty][tx].type == PIECE_TYPE_ROOK) {
                // check which side to revoke castling for
                if (ox == 0) {
                    if (current_player == PLAYER_WHITE) {
                        castling_white_queen = false;
                    }
                    if (current_player == PLAYER_BLACK) {
                        castling_black_queen = false;
                    }
                }
                if (ox == 7) {
                    if (current_player == PLAYER_WHITE) {
                        castling_white_king = false;
                    }
                    if (current_player == PLAYER_BLACK) {
                        castling_black_king = false;
                    }
                }
            }
        }
        // swap current player
        current_player = current_player == PLAYER_WHITE ? PLAYER_BLACK : PLAYER_WHITE;
    }

    std::vector<uint64_t> Chess::get_moves_pseudo_legal()
    {
        // directions are: N,S,W,E,NW,SE,NE,SW
        const int directions_x[8] = {0, 0, -1, 1, -1, 1, 1, -1};
        const int directions_y[8] = {1, -1, 0, 0, 1, -1, 1, -1};
        // vectors are clockwise from the top
        const int knight_vx[8] = {1, 2, 2, 1, -1, -2, -2, -1};
        const int knight_vy[8] = {2, 1, -1, -2, -2, -1, 1, 2};
        // vector are: WHITE{NW,N,NE,2N}, BLACK{SW,S,SE,2S}
        const int pawn_cvx[8] = {-1, 0, 1, 0, -1, 0, 1, 0};
        const int pawn_cvy[8] = {1, 1, 1, 2, -1, -1, -1, -2};
        std::vector<uint64_t> moves{};
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                piece current_piece = board[y][x];
                if (current_piece.player != current_player) {
                    continue;
                }
                // gen move for this piece
                switch (current_piece.type) {
                    case PIECE_TYPE_KING: {
                        for (int d = 0; d < 8; d++) {
                            int tx = x + directions_x[d];
                            int ty = y + directions_y[d];
                            if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                                continue;
                            }
                            piece target_piece = board[ty][tx];
                            if (target_piece.player == current_player) {
                                continue;
                            }
                            moves.push_back((x<<12)|(y<<8)|(tx<<4)|(ty));
                        }
                        // king is not allowed to move through attacked squares while castling, this is handled elsewhere for now
                        if (current_player == PLAYER_WHITE) {
                            if (castling_white_king && board[y][x+1].type == PIECE_TYPE_NONE && board[y][x+2].type == PIECE_TYPE_NONE &&
                                board[y][7].player == current_player && board[y][7].type == PIECE_TYPE_ROOK) {
                                moves.push_back((x<<12)|(y<<8)|((x+2)<<4)|(y));
                            }
                            if (castling_white_queen && board[y][x-1].type == PIECE_TYPE_NONE && board[y][x-2].type == PIECE_TYPE_NONE && board[y][x-3].type == PIECE_TYPE_NONE &&
                                board[y][0].player == current_player && board[y][0].type == PIECE_TYPE_ROOK) {
                                moves.push_back((x<<12)|(y<<8)|((x-2)<<4)|(y));
                            }
                        }
                        if (current_player == PLAYER_BLACK) {
                            if (castling_black_king && board[y][x+1].type == PIECE_TYPE_NONE && board[y][x+2].type == PIECE_TYPE_NONE &&
                                board[y][7].player == current_player && board[y][7].type == PIECE_TYPE_ROOK) {
                                moves.push_back((x<<12)|(y<<8)|((x+2)<<4)|(y));
                            }
                            if (castling_black_queen && board[y][x-1].type == PIECE_TYPE_NONE && board[y][x-2].type == PIECE_TYPE_NONE && board[y][x-3].type == PIECE_TYPE_NONE &&
                                board[y][0].player == current_player && board[y][0].type == PIECE_TYPE_ROOK) {
                                moves.push_back((x<<12)|(y<<8)|((x-2)<<4)|(y));
                            }
                        }
                    } break;
                    case PIECE_TYPE_QUEEN:
                    case PIECE_TYPE_ROOK:
                    case PIECE_TYPE_BISHOP: {
                        int dmin = current_piece.type == PIECE_TYPE_BISHOP ? 4 : 0;
                        int dmax = current_piece.type == PIECE_TYPE_ROOK ? 4 : 8;
                        for (int d = dmin; d < dmax; d++) {
                            for (int s = 1; true; s++) {
                                int tx = x + s*directions_x[d];
                                int ty = y + s*directions_y[d];
                                if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                                    break;
                                }
                                piece target_piece = board[ty][tx];
                                if (target_piece.player == current_player) {
                                    break;
                                }
                                moves.push_back((x<<12)|(y<<8)|(tx<<4)|(ty));
                                if (target_piece.player != current_piece.player && target_piece.type != PIECE_TYPE_NONE) {
                                    break;
                                }
                            }
                        }
                    } break;
                    case PIECE_TYPE_KNIGHT: {
                        for (int d = 0; d < 8; d++) {
                            int tx = x + knight_vx[d];
                            int ty = y + knight_vy[d];
                            if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                                continue;
                            }
                            piece target_piece = board[ty][tx];
                            if (target_piece.player == current_player) {
                                continue;
                            }
                            moves.push_back((x<<12)|(y<<8)|(tx<<4)|(ty));
                        }
                    } break;
                    case PIECE_TYPE_PAWN: {
                        int dmin = current_player == PLAYER_BLACK ? 4 : 0;
                        int dmax = current_player == PLAYER_WHITE ? 4 : 8;
                        for (int d = dmin; d < dmax; d++) {
                            int tx = x + pawn_cvx[d];
                            int ty = y + pawn_cvy[d];
                            if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                                continue;
                            }
                            piece target_piece = board[ty][tx];
                            if (target_piece.player == current_player) {
                                continue;
                            }
                            if (tx != x && target_piece.type == PIECE_TYPE_NONE && enpassant_target != ((tx<<4)|(ty))) {
                                // pawn would move diagonally, but there isnt any piece there to capture, and it also isnt an en passant target
                                continue;
                            }
                            if (y != 1 && y != 6 && (ty-y > 1 || y-ty > 1) ) {
                                // do not allow double pawn push if it isnt in its starting position
                                continue;
                            }
                            if ((y == 1 || y == 6) && (ty-y > 1 || y-ty > 1) && board[y+pawn_cvy[d-2]][tx].type != PIECE_TYPE_NONE) {
                                // double pawn push only allowed over empty squares
                                continue;
                            }
                            if (tx-x == 0 && target_piece.type != PIECE_TYPE_NONE) {
                                // can only advance straight forward into open spaces
                                continue;
                            }
                            if (ty == 0 || ty == 7) {
                                // pawn promotion, instead add 4 moves for the 4 types of promotions
                                moves.push_back((PIECE_TYPE_QUEEN<<16)|(x<<12)|(y<<8)|(tx<<4)|(ty));
                                moves.push_back((PIECE_TYPE_ROOK<<16)|(x<<12)|(y<<8)|(tx<<4)|(ty));
                                moves.push_back((PIECE_TYPE_BISHOP<<16)|(x<<12)|(y<<8)|(tx<<4)|(ty));
                                moves.push_back((PIECE_TYPE_KNIGHT<<16)|(x<<12)|(y<<8)|(tx<<4)|(ty));
                                continue;
                            }
                            moves.push_back((x<<12)|(y<<8)|(tx<<4)|(ty));
                        }
                    } break;
                    default: break;
                }
            }
        }
        return moves;
    }

    // this chess implementation is valid against all chessprogrammingwiki positions, tested up to depth 4
    uint64_t Chess::count_positions(int depth)
    {
        std::vector<uint64_t> moves = get_moves();
        if (depth == 1) {
            return moves.size(); // bulk counting since get_moves generates only legal moves
        }
        uint64_t positions = 0;
        surena::Chess test_game;
        for (int i = 0; i < moves.size(); i++) {
            test_game.copy_from(this);
            test_game.apply_move_internal(moves[i]);
            positions += test_game.count_positions(depth - 1);
        }
        return positions;
    }

}
