#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace surena {

    //TODO inherit from C_API struct
    //TODO expose all the C_API methods
    class Game {

        public:

            virtual ~Game() = default;

            //TODO should the game expose some functionality to see what type of special moves it has?
            // i.e. something like feature flags that returns a flat union for things like randomness, simultaneous moves, hidden information?
            
            //NOTE:
            // imperfect information can be collapsed into one possible legal state by using discretize, use as much gathered information about unknown players as possible when collapsing
            // random events are represented using the 0xFF player id
            // simultaneous move games present the union of all moves for all simultaneously moving players as available moves and the applied moves for every simultaneously moving player until all are gathered for processing

            // there are no state guarantees if this is passed an invalid state string, //TODO is this sane? maybe return a bool + no-crash guarantee?
            // load the gamestate from the given string, does nothing if NULL
            virtual void import_state(const char* str) = 0;
            // if data is NULL then this returns the minimum size of buffer to allocate for data
            // returns the length of the state string written, 0 if failure
            virtual uint32_t export_state(char* str) = 0; 

            //TODO state id (e.g. zobrist)
            //virtual uint64_t id() = 0;

            // returns the playerID to move from this state, 0 if the game is over
            virtual uint8_t player_to_move() = 0;

            // returns an unordered list of available moves
            virtual std::vector<uint64_t> get_moves() = 0;
            //TODO unordered iterator over all available moves

            // performs a move on the gamestate, non reversible
            // moves not in the available moves list cause undefined behaviour
            virtual void apply_move(uint64_t move_id) = 0;

            // returns the winning playerID, 0 if the game is not over yet or is a draw
            virtual uint8_t get_result() = 0;

            //TODO evaluation function

            // discretize the gamestate into a random legal state influenced by the seed and internal state set
            // only to be called once, irreversible
            virtual void discretize(uint64_t seed) = 0;

            // playouts the game by randomly playing legal moves until the game is over, returns get_result
            virtual uint8_t perform_playout(uint64_t seed) = 0;

            virtual Game* clone() = 0;
            virtual void copy_from(Game* target) = 0;

            //TODO redo these to be like import/export state on char*
            // move id transformation functions
            // uint64_t <-> string
            virtual uint64_t get_move_id(std::string move_string) = 0;
            virtual std::string get_move_string(uint64_t move_id) = 0;

            virtual void debug_print() = 0;

            //TODO general description string?

    };

}
