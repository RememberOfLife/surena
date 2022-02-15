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
        //TODO
        return 0;
    }

    uint8_t Chess::player_to_move()
    {
        return current_player;
    }

    std::vector<uint64_t> Chess::get_moves()
    {
        std::vector<uint64_t> moves{};
        // if game is over, return empty move list
        if (player_to_move() == 0) {
            return moves;
        }
        //TODO encode things like check in the move, for move string printing in ptn?
        //TODO
        return moves;
    }

    void Chess::apply_move(uint64_t move_id)
    {
        int ox = (move_id >> 12) & 0x0F;
        int oy = (move_id >> 8) & 0x0F;
        int tx = (move_id >> 4) & 0x0F;
        int ty = move_id & 0x0F;
        board[ty][tx] = board[oy][ox];
        board[oy][ox] = piece{PLAYER_NONE, PIECE_TYPE_NONE};
        //TODO manage castling moves
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
        //TODO
        return 0;
    }

    std::string Chess::get_move_string(uint64_t move_id)
    {
        //TODO
        return "----";
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
            printf("%c%c", 'a'+(enpassant_target&0xF0), '1'+(enpassant_target&0x0F));
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

}
