#pragma once

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

    class Havannah : public Game {

        public:

            enum COLOR : uint8_t {
                COLOR_NONE = 0,
                COLOR_WHITE,
                COLOR_BLACK,
                COLOR_INVALID,
            };
            static const char* COLOR_CHARS[4]; // in the same order as they appear in the enum of same name

            // assuming a flat topped board this is [letter * num]
            // such that num goes vertically on the left downwards
            // and letter goes ascending horizontally towards the right
            struct Tile_Id {
                uint_fast8_t x, y;
                Tile_Id(std::string an);
                Tile_Id(int x, int y);
                Tile_Id(uint64_t move_code);
                std::string an();
                uint64_t to_move_id();
            };

            struct Tile {
                COLOR color;
                uint8_t parent_graph_id;
                //TODO maybe store existing neighbor tiles as bitmap for every tile (preferred), or store feature contribution of every tile, this is a bijection
            };

            struct Graph {
                // a joined graph no longer is its own parent, instead it points to the true graph which it connects to
                uint8_t parent_graph_id;
                uint8_t connected_borders;
                uint8_t connected_corners;
            };

        private:
            int size;
            int board_sizer;
            int remaining_tiles;
            COLOR current_player;
            COLOR winning_player;
            uint8_t next_graph_id;
            std::unordered_map<uint8_t, Graph> graph_map;
            std::vector<std::vector<Tile>> gameboard;

        public:

            Havannah(int size);

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

            //#####
            // game specific exposed functions

            int get_size();
            COLOR get_cell(int x, int y);

    };
    
}
