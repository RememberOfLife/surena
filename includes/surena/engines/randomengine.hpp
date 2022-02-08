#pragma once

#include "surena/util/fast_prng.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

namespace surena {

    class RandomEngine : public PerfectInformationEngine {

        private:

            Game* gamestate;
            fast_prng rng;

        public:

            RandomEngine();

            RandomEngine& operator=(RandomEngine&& copy) = delete;

            RandomEngine& operator=(RandomEngine& copy) = delete;

            RandomEngine(RandomEngine&& copy) = delete;

            RandomEngine(RandomEngine& copy) = delete;

            ~RandomEngine() override;

            void search_start() override;
            void search_stop() override;
            
            uint64_t get_best_move() override;

            void set_gamestate(Game* target_gamestate) override;

            uint8_t player_to_move() override;

            std::vector<uint64_t> get_moves() override;

            void apply_move(uint64_t move_id) override;

            uint8_t get_result() override;

            uint64_t get_move_id(std::string move_string) override;
            std::string get_move_string(uint64_t move_id) override;

            void debug_print() override;

    };

}
