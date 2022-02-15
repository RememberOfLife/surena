#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "surena/game.hpp"

namespace surena {

    class TicTacToe_Ultimate : public Game {

        private:

            // origin bottom left, y upwards, x to the right
            uint32_t board[3][3];
            uint32_t global_board = 0; // 0 is ongoing, 3 is draw result
            // global target is the local board that the current player has to play to
            int global_target_x = -1;
            int global_target_y = -1;
            uint8_t current_player = 1;
            uint8_t winning_player = 0;

        public:

            TicTacToe_Ultimate();
            
            void import_state(const char* str) override;
            uint32_t export_state(char* str) override; 

            uint8_t player_to_move() override;

            std::vector<uint64_t> get_moves() override;

            void apply_move(uint64_t move_id) override;

            uint8_t get_result() override;

            void discretize(uint64_t seed) override;

            uint8_t perform_playout(uint64_t seed) override;
            
            Game* clone() override;
            void copy_from(Game* target) override;

            uint64_t get_move_id(std::string move_string) override;
            std::string get_move_string(uint64_t move_id) override;
            
            void debug_print() override;

            //#####
            // game specific exposed functions

            uint8_t check_result(uint32_t& state);
            uint8_t get_cell(uint32_t& state, int x, int y);
            void set_cell(uint32_t& state, int x, int y, uint8_t p);

            uint8_t get_cell_global(int x, int y);
            void set_cell_global(int x, int y, uint8_t p);

            uint8_t get_cell_local(int x, int y);
            void set_cell_local(int x, int y, uint8_t p);

            uint8_t get_global_target(); // yyxx in least significant bits, 3 is none
            void set_global_target(int x, int y); // any == -1 is none

            void set_current_player(uint8_t p);
            void set_result(uint8_t p);

    };

}
