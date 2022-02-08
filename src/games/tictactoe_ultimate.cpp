#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "surena/util/fast_prng.hpp"

#include "surena/games/tictactoe_ultimate.hpp"

namespace surena {

    TicTacToe_Ultimate::TicTacToe_Ultimate()
    {
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                board[y][x] = 0;
            }
        }
    }

    uint8_t TicTacToe_Ultimate::player_to_move()
    {
        return current_player;
    }

    std::vector<uint64_t> TicTacToe_Ultimate::get_moves()
    {
        std::vector<uint64_t> moves{};
        // if game is over, return empty move list
        if (player_to_move() == 0) {
            return moves;
        }
        if (global_target_x >= 0 && global_target_y >= 0) {
            // only give moves for the target local board
            for (int ly = global_target_y*3; ly < global_target_y*3+3; ly++) {
                for (int lx = global_target_x*3; lx < global_target_x*3+3; lx++) {
                    if (get_cell_local(lx, ly) == 0) {
                        moves.push_back((ly<<4)|lx);
                    }
                }
            }
        } else {
            for (int gy = 0; gy < 3; gy++) {
                for (int gx = 0; gx < 3; gx++) {
                    if (get_cell_global(gx, gy) == 0) {
                        for (int ly = gy*3; ly < gy*3+3; ly++) {
                            for (int lx = gx*3; lx < gx*3+3; lx++) {
                                if (get_cell_local(lx, ly) == 0) {
                                    moves.push_back((ly<<4)|lx);
                                }
                            }
                        }
                    }
                }
            }
        }
        return moves;
    }

    void TicTacToe_Ultimate::apply_move(uint64_t move_id)
    {
        // calc move and set cell
        int x = move_id & 0b1111;
        int y = (move_id >> 4) & 0b1111;
        set_cell_local(x, y, current_player);
        int gx = x/3;
        int gy = y/3;
        int lx = x%3;
        int ly = y%3;
        // calculate possible result of local board
        uint8_t local_result = check_result(board[gy][gx]);
        if (local_result > 0) {
            set_cell_global(gx, gy, local_result);
        }
        // set global target if applicable
        if (get_cell_global(lx, ly) == 0) {
            global_target_x = lx;
            global_target_y = ly;
        } else {
            global_target_x = -1;
            global_target_y = -1;
        }
        if (local_result == 0) {
            // if no local change happened, do not check global win, switch player
            current_player = (current_player == 1) ? 2 : 1;
            return;
        }
        // detect win for current player
        uint8_t global_result = check_result(global_board);
        if (global_result > 0) {
            winning_player = global_result;
            current_player = 0;
            return;
        }
        // switch player
        current_player = (current_player == 1) ? 2 : 1;
    }

    uint8_t TicTacToe_Ultimate::get_result()
    {
        return winning_player;
    }

    void TicTacToe_Ultimate::discretize(uint64_t seed)
    {}

    uint8_t TicTacToe_Ultimate::perform_playout(uint64_t seed)
    {
        fast_prng rng(seed);
        while (player_to_move() != 0) {
            std::vector<uint64_t> moves_available = get_moves();
            apply_move(moves_available[rng.rand()%moves_available.size()]);
        }
        return get_result();
    }

    surena::Game* TicTacToe_Ultimate::clone() 
    {
        return new TicTacToe_Ultimate(*this);
    }

    void TicTacToe_Ultimate::copy_from(Game* target) 
    {
        *this = *static_cast<TicTacToe_Ultimate*>(target);
    }
    
    uint64_t TicTacToe_Ultimate::get_move_id(std::string move_string)
    {
        if (move_string.length() != 2) {
            return UINT64_MAX;
        }
        uint64_t move_id = 0;
        move_id |= (move_string[0]-'A');
        move_id |= (move_string[1]-'0') << 4;
        return move_id;
    }

    std::string TicTacToe_Ultimate::get_move_string(uint64_t move_id)
    {
        int x = move_id & 0b1111;
        int y = (move_id >> 4) & 0b1111;
        std::string move_string = "";
        move_string += ('A'+x);
        move_string += ('0'+y);
        return move_string;
    }

    void TicTacToe_Ultimate::debug_print()
    {
        printf("##### debug print board #####\n");
        printf("global target: %s\n", (global_target_x >= 0 && global_target_y >= 0) ? get_move_string((global_target_y<<4)|global_target_x).c_str() : "--");
        for (int gy = 2; gy >= 0; gy--) {
            for (int ly = 2; ly >= 0; ly--) {
                printf("%c ", '0'+(gy*3+ly));
                for (int gx = 0; gx < 3; gx++) {
                    for (int lx = 0; lx < 3; lx++) {
                        uint8_t player_in_cell = get_cell_global(gx, gy);
                        if (player_in_cell == 0) {
                            player_in_cell = get_cell_local(gx*3+lx, gy*3+ly);
                        }
                        switch (player_in_cell) {
                            case (1): {
                                printf("X");
                            } break;
                            case (2):{ 
                                printf("O");
                            } break;
                            case (3):{ 
                                printf("=");
                            } break;
                            default: {
                                printf(".");
                            } break;
                        }
                    }
                    printf(" ");
                }
                printf("\n");
            }
            if (gy > 0) {
                printf("\n");
            }
        }
        printf("  ");
        for (int gx = 0; gx < 3; gx++) {
            for (int lx = 0; lx < 3; lx++) {
                printf("%c", 'A'+(gx*3+lx));
            }
            printf(" ");
        }
        printf("\n");
        printf("##### +++++ +++++ +++++ #####\n");
    }

    // return 0 if game running, 1-2 for player wins, 3 for draw
    uint8_t TicTacToe_Ultimate::check_result(uint32_t& state)
    {
        // detect win for current player
        uint8_t state_winner = 0;
        bool win = false;
        for (int i = 0; i < 3; i++) {
            if ((get_cell(state, 0, i) == get_cell(state, 1, i) && get_cell(state, 0, i) == get_cell(state, 2, i) && get_cell(state, 0, i) != 0) ||
            (get_cell(state, i, 0) == get_cell(state, i, 1) && get_cell(state, i, 0) == get_cell(state, i, 2) && get_cell(state, i, 0) != 0)) {
                win = true;
                state_winner = get_cell(state, i, i);
            }
        }
        if (((get_cell(state, 0, 0) == get_cell(state, 1, 1) && get_cell(state, 0, 0) == get_cell(state, 2, 2)) || (get_cell(state, 0, 2) == get_cell(state, 1, 1) && get_cell(state, 0, 2) == get_cell(state, 2, 0))) && get_cell(state, 1, 1) != 0) {
            win = true;
            state_winner = get_cell(state, 1, 1);
        }
        if (win) {
            return state_winner;
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
            return 3;
        }
        return 0;
    }

    uint8_t TicTacToe_Ultimate::get_cell(uint32_t& state, int x, int y)
    {
        // shift over the correct 2 bits representing the player at that position
        int v = (state >> (y*6+x*2)) & 0b11;
        return static_cast<uint8_t>(v);
    }

    void TicTacToe_Ultimate::set_cell(uint32_t& state, int x, int y, uint8_t p)
    {
        uint64_t v = get_cell(state, x, y);
        int offset = (y*6+x*2);
        // new_state = current_value xor (current_value xor new_value)
        v = v << offset;
        v ^= static_cast<uint64_t>(p) << offset;
        state ^= v;
    }

    uint8_t TicTacToe_Ultimate::get_cell_global(int x, int y)
    {
        return get_cell(global_board, x, y);
    }

    void TicTacToe_Ultimate::set_cell_global(int x, int y, uint8_t p)
    {
        set_cell(global_board, x, y, p);
    }

    uint8_t TicTacToe_Ultimate::get_cell_local(int x, int y)
    {
        return get_cell(board[y/3][x/3], x%3, y%3);
    }

    void TicTacToe_Ultimate::set_cell_local(int x, int y, uint8_t p)
    {
        set_cell(board[y/3][x/3], x%3, y%3, p);
    }

    uint8_t TicTacToe_Ultimate::get_global_target()
    {
        return ((global_target_y==-1?3:global_target_y)<<2)|(global_target_x==-1?3:global_target_x);
    }

}
