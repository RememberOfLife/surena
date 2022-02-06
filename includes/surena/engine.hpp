#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "surena/game.hpp"

namespace surena {

    class PerfectInformationEngine {

        public:

            virtual ~PerfectInformationEngine() = default;

            //TODO does initialization of large memory operations automatically happen on class instanziation
            // or delay into a custom init function invoked by the command interface on isready?

            // search
            virtual void search_start() = 0;
            virtual void search_stop() = 0;
            // constrained

            // returns the playerID to move from this state, 0 if the game is over
            virtual uint8_t player_to_move() = 0;

            // get best move
            virtual uint64_t get_best_move() = 0;

            // engine takes ownership of target gamestate
            virtual void set_gamestate(PerfectInformationGame* target_gamestate) = 0;

            // returns an unordered list of available moves
            virtual std::vector<uint64_t> get_moves() = 0;

            // performs a move on the gamestate, non reversible
            // moves not in the available moves list cause undefined behaviour
            virtual void apply_move(uint64_t move_id) = 0;

            // returns the winning playerID, 0 if the game is not over yet or is a draw
            virtual uint8_t get_result() = 0;

            // move id transformation functions
            // uint64_t <-> string
            virtual uint64_t get_move_id(std::string move_string) = 0;
            virtual std::string get_move_string(uint64_t move_id) = 0;

            virtual void debug_print() = 0;

    };

}

#endif
