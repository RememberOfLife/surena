#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "surena/game.hpp"

namespace surena {

    class TicTacToe : public Game {

        private:

            /*
            board as:
            789
            456
            123
            state bits: ... RR CC 998877 665544 332211
            where RR is results and CC is current player
            */
            uint32_t state;

        public:

            TicTacToe();

            uint8_t player_to_move() override;

            std::vector<uint64_t> get_moves() override;

            void apply_move(uint64_t move_id) override;

            void apply_internal_update(uint64_t update_id) override;

            uint8_t get_result() override;

            void discretize(uint64_t seed) override;

            uint8_t perform_playout(uint64_t seed) override;
            
            Game* clone() override;
            void copy_from(Game* target) override;

            uint64_t get_move_id(std::string move_string) override;
            std::string get_move_string(uint64_t move_id) override;
            
            void debug_print() override;

            // get player value of cell (x grows right, y grows up)
            uint8_t get_cell(int x, int y);
            void set_cell(int x, int y, uint8_t p);

    };

}
