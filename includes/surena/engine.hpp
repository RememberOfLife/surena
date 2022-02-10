#pragma once

#include "surena/game.hpp"

namespace surena {

    class Engine {

        public:

            virtual ~Engine() = default;

            //TODO does initialization of large memory operations automatically happen on class instantiation
            // or delay into a custom init function invoked by the command interface on isready?

            //TODO requires a compatiblity check function against the feature flags of the game

            //TODO sync some more methods from game into here

            // start search async
            // will search until stop or timeout is reached, will not timeout if timeout is 0
            virtual void search_start(uint64_t ms_timeout = 0) = 0;
            virtual void search_stop() = 0;
            //TODO constrained, also sync?

            // returns the playerID to move from this state, 0 if the game is over
            virtual uint8_t player_to_move() = 0;

            // get best move
            virtual uint64_t get_best_move() = 0;

            // engine takes ownership of target gamestate
            // target_gamestate may be NULL, commands used on an engine without a gamestate may return garbage, but may not crash
            virtual void set_gamestate(Game* target_gamestate) = 0;

            // returns an unordered list of available moves
            virtual std::vector<uint64_t> get_moves() = 0;

            // performs a move on the gamestate, non reversible
            // moves not in the available moves list cause undefined behaviour
            virtual void apply_move(uint64_t move_id) = 0;

            //TODO see game todo here
            virtual void apply_internal_update(uint64_t update_id) = 0;

            // returns the winning playerID, 0 if the game is not over yet or is a draw
            virtual uint8_t get_result() = 0;

            // move id transformation functions
            // uint64_t <-> string
            virtual uint64_t get_move_id(std::string move_string) = 0;
            virtual std::string get_move_string(uint64_t move_id) = 0;

            virtual void debug_print() = 0;

    };

}
