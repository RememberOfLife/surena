#ifndef ALHAMBRA_HPP
#define ALHAMBRA_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "surena/game.hpp"

namespace surena {

    class Alhambra : public ImperfectInformationGame {

        private:

            // state

        public:

            Alhambra();

            void discretize(uint64_t seed) override;

            uint8_t player_to_move() override;

            std::vector<uint64_t> get_moves() override;

            void apply_move(uint64_t move_id) override;

            uint8_t get_result() override;

            uint8_t perform_playout(uint64_t seed) override;
            
            PerfectInformationGame* clone() override;
            void copy_from(PerfectInformationGame* target) override;

            uint64_t get_move_id(std::string move_string) override;
            std::string get_move_string(uint64_t move_id) override;
            
            void debug_print() override;

        private:

            // helpers

    };

}

#endif
