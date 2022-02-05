#ifndef HAVANNAH_HPP
#define HAVANNAH_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "surena/game.hpp"

namespace surena {

    /*
    Havannah: (perfect information) [2P]
    played on a hexagonal board with hexagonal tiles, edge length n of (10/8/6), tile count = 3 * n**2 - ( 3n - 1 )
    black and white sequentially each place a piece of their color on any empty tile on the board
    the player wins that first creates either a ring, bridge or fork structure from unbroken lines of connected stones of their color
    a ring is a loop around one or more cells the occupation status of whom does not matter
    a bride connects any two of the six corner cells of the board
    a fork connects any three edges of the board, corner points are not parts of any edge
    because the first move has advantage, the opponent may either switch the colors or let the move stand (switch rule)
    the weaker player may be allowed to place more than 1 stone on their first turn if desired, as compensation
    */
    
    enum Color : uint8_t {
        COLOR_NONE = 0,
        COLOR_INVALID = 1,
        COLOR_WHITE = 2,
        COLOR_BLACK = 3,
    };
    extern const char COLOR_CHARS[5]; // in the same order as they appear in the enum of same name

    // assuming a flat topped board this is [letter * num]
    // such that num goes vertically on the left downwards
    // and letter goes ascending horizontally towards the right
    typedef struct TileId {
        uint_fast8_t x, y;
        TileId(std::string an);
        TileId(int x, int y);
        TileId(uint64_t move_code);
        std::string an();
        uint64_t to_move_id();
    } TileId;

    typedef struct Tile {
        Color color;
        uint8_t parentGraphId;
        //TODO maybe store existing neighbor tiles as bitmap for every tile (preferred), or store feature contribution of every tile, this is a bijection
    } Tile;

    class Havannah : public PerfectInformationGame {

        struct Graph {
            // a joined graph no longer is its own parent, instead it points to the true graph which it connects to
            uint8_t parentGraphId;
            uint8_t connectedBorders;
            uint8_t connectedCorners;
        };

        private:
            int size;
            int boardSizer;
            int remainingTiles;
            Color currentPlayer;
            Color winningPlayer;
            uint8_t nextGraphId;
            std::unordered_map<uint8_t, Graph> graphMap;
            std::vector<std::vector<Tile>> gameboard;

        public:

            Havannah(int size);

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

    };
    
}

#endif
