#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "surena/game.hpp"

namespace surena {

    class Alhambra : public Game {

        private:

            // state

        public:

            Alhambra();

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

        private:

            // helpers

    };

}
