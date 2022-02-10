#include <array>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <string>

#include "surena/util/fast_prng.hpp"

#include "surena/games/havannah.hpp"

namespace surena {

        const char* Havannah::COLOR_CHARS[4] = {".", "O", "X", "-"}; // invalid, none, white, black

        Havannah::Tile_Id::Tile_Id(std::string an)
        {
            this->x = an[0]-'A';
            an.replace(0, 1, "");
            this->y = stoi(an);
        }

        Havannah::Tile_Id::Tile_Id(int x, int y):
            x(static_cast<uint_fast8_t>(x)),
            y(static_cast<uint_fast8_t>(y))
        {}

        Havannah::Tile_Id::Tile_Id(uint64_t move_code):
            x(static_cast<int>(move_code >> 8) & 0xFF),
            y(static_cast<int>(move_code & 0xFF))
        {}

        std::string Havannah::Tile_Id::an()
        {
            return std::to_string(this->y).insert(0, 1, this->x+'A');
        }

        uint64_t Havannah::Tile_Id::to_move_id()
        {
            return static_cast<uint64_t>((static_cast<uint64_t>(x) << 8) | y);
        }

        Havannah::Havannah(int size):
            size(size),
            board_sizer(2*size-1),
            remaining_tiles((board_sizer*board_sizer)-(size*(size-1))),
            current_player(COLOR_WHITE),
            winning_player(COLOR_INVALID),
            next_graph_id(1)
        {
            gameboard.reserve(board_sizer);
            for (int iy = 0; iy < board_sizer; iy++) {
                std::vector<Tile> tile_row_vector{};
                tile_row_vector.resize(board_sizer, Tile{COLOR_INVALID, 0});
                gameboard.push_back(std::move(tile_row_vector));
                for (int ix = 0; ix < board_sizer; ix++) {
                    if ((ix - iy < size) && (iy - ix < size)) { // magic formula for only enabling valid cells of the board
                        gameboard[iy][ix].color = COLOR_NONE;
                    }
                }
            }
        }

        uint8_t Havannah::player_to_move()
        {
            return current_player;
        }

        std::vector<uint64_t> Havannah::get_moves()
        {
            std::vector<uint64_t> free_tiles{};
            for (int iy = 0; iy < board_sizer; iy++) {
                for (int ix = 0; ix < board_sizer; ix++) {
                    if (gameboard[iy][ix].color == COLOR_NONE) {
                        // add the free tile to the return vector
                        free_tiles.push_back(Tile_Id(ix, iy).to_move_id());
                    }
                }
            }
            return free_tiles;
        }

        void Havannah::apply_move(uint64_t m)
        {
            Tile_Id target_tile = Tile_Id(m);

            bool winner = false; // if this is true by the end, current player wins

            uint8_t contribution_border = 0b00111111; // begin on the north-west corner and it's left border, going counter-clockwise
            uint8_t contribution_corner = 0b00111111; 
            std::array<uint8_t, 3> adjacent_graphs = {0, 0, 0}; // there can only be a maximum of 3 graphs with gaps in between
            int adjacent_graph_count = 0;
            bool empty_start = true; // true if the first neighbor tile is non-same color
            uint8_t current_graph_id = 0; // 0 represents a gap, any value flags an ongoing streak which has to be saved before resetting the flag to 0 for a gap

            // discover north-west neighbor
            if (target_tile.y > 0 && target_tile.x > 0) {
                // exists
                if (gameboard[target_tile.y-1][target_tile.x-1].color == current_player) {
                    current_graph_id = gameboard[target_tile.y-1][target_tile.x-1].parent_graph_id;
                    empty_start = false;
                }/* else {
                    // not a same colored piece, save previous streak and set gap
                    if (currentGraphId != 0) {
                        adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                    }
                    currentGraphId = 0;
                }*/
                contribution_border &= 0b00011110;
                contribution_corner &= 0b00001110;
            } else {
                // invalid, narrow possible contribution features and save previous streak
                contribution_border &= 0b00100001;
                contribution_corner &= 0b00110001;
                /*if (currentGraphId != 0) {
                    adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                }
                currentGraphId = 0;*/
            }

            // discover west neighbor
            if (target_tile.x > 0 && (target_tile.y - target_tile.x) < size-1) {
                if (gameboard[target_tile.y][target_tile.x-1].color == current_player) {
                    current_graph_id = gameboard[target_tile.y][target_tile.x-1].parent_graph_id;
                } else {
                    if (current_graph_id != 0) {
                        adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                    }
                    current_graph_id = 0;
                }
                contribution_border &= 0b00001111;
                contribution_corner &= 0b00000111;
            } else {
                contribution_border &= 0b00110000;
                contribution_corner &= 0b00111000;
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }

            // discover south-west neighbor
            if (target_tile.y < board_sizer-1 && (target_tile.y - target_tile.x < size-1)) {
                if (gameboard[target_tile.y+1][target_tile.x].color == current_player) {
                    current_graph_id = gameboard[target_tile.y+1][target_tile.x].parent_graph_id;
                } else {
                    if (current_graph_id != 0) {
                        adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                    }
                    current_graph_id = 0;
                }
                contribution_border &= 0b00100111;
                contribution_corner &= 0b00100011;
            } else {
                contribution_border &= 0b00011000;
                contribution_corner &= 0b00011100;
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }
            
            // discover south-east neighbor
            if (target_tile.y < board_sizer-1 && target_tile.x < board_sizer-1) {
                if (gameboard[target_tile.y+1][target_tile.x+1].color == current_player) {
                    current_graph_id = gameboard[target_tile.y+1][target_tile.x+1].parent_graph_id;
                } else {
                    if (current_graph_id != 0) {
                        adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                    }
                    current_graph_id = 0;
                }
                contribution_border &= 0b00110011;
                contribution_corner &= 0b00110001;
            } else {
                contribution_border &= 0b00001100;
                contribution_corner &= 0b00001110;
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }

            // discover east neighbor
            if (target_tile.x < board_sizer-1 && (target_tile.x - target_tile.y < size-1)) {
                if (gameboard[target_tile.y][target_tile.x+1].color == current_player) {
                    current_graph_id = gameboard[target_tile.y][target_tile.x+1].parent_graph_id;
                } else {
                    if (current_graph_id != 0) {
                        adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                    }
                    current_graph_id = 0;
                }
                contribution_border &= 0b00111001;
                contribution_corner &= 0b00111000;
            } else {
                contribution_border &= 0b00000110;
                contribution_corner &= 0b00000111;
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }

            // discover north-east neighbor
            if (target_tile.y > 0 && (target_tile.x - target_tile.y < size-1)) {
                if (gameboard[target_tile.y-1][target_tile.x].color == current_player) {
                    current_graph_id = gameboard[target_tile.y-1][target_tile.x].parent_graph_id;
                } else {
                    if (current_graph_id != 0) {
                        adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                    }
                    current_graph_id = 0;
                }
                contribution_border &= 0b00111100;
                contribution_corner &= 0b00011100;
            } else {
                contribution_border &= 0b00000011;
                contribution_corner &= 0b00100011;
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }

            // simulate a gap, to properly save last streak if requird by empty start
            // non-empty start will just discard the last streak which would be a dupe of itself anyway
            if (empty_start && current_graph_id != 0) {
                adjacent_graphs[adjacent_graph_count++] = current_graph_id;
            }

            //##### evaluate neighbor scanning results to update state

            // resolve all existing adjacent graphs to their true parentGraphId
            for (int i = 0; i < adjacent_graph_count; i++) {
                while (adjacent_graphs[i] != graph_map[adjacent_graphs[i]].parent_graph_id) {
                    //TODO any good way to keep the indirections down, without looping over the whole graphMap when reparenting?
                    adjacent_graphs[i] = graph_map[adjacent_graphs[i]].parent_graph_id;
                }
            }
            
            // set current graph to the first discovered, if any
            if (adjacent_graph_count > 0) {
                current_graph_id = adjacent_graphs[0];
            }

            // check for ring creation amongst adjacent graphs
            if (
                (adjacent_graphs[0] != 0 && adjacent_graphs[1] != 0 && adjacent_graphs[0] == adjacent_graphs[1]) ||
                (adjacent_graphs[0] != 0 && adjacent_graphs[2] != 0 && adjacent_graphs[0] == adjacent_graphs[2]) ||
                (adjacent_graphs[2] != 0 && adjacent_graphs[1] != 0 && adjacent_graphs[2] == adjacent_graphs[1])
            ) {
                // ring detected, game has been won by currentPlayer
                winner = true;
            }

            // no adjacent graphs, create a new one
            if (adjacent_graph_count == 0) {
                    current_graph_id = next_graph_id++;
                    graph_map[current_graph_id].parent_graph_id = current_graph_id;
            }

            // 2 or 3 adjacent graphs, all get reparented and contribute their features
            for (int i = 1; i < adjacent_graph_count; i++) {
                    graph_map[adjacent_graphs[i]].parent_graph_id = current_graph_id;
                    graph_map[current_graph_id].connected_borders |= graph_map[adjacent_graphs[i]].connected_borders;
                    graph_map[current_graph_id].connected_corners |= graph_map[adjacent_graphs[i]].connected_corners;
            }

            // contribute target tile features
            if (contribution_border == 0b00000000) {
                graph_map[current_graph_id].connected_corners |= contribution_corner;
            } else {
                graph_map[current_graph_id].connected_borders |= contribution_border;
            }

            // check if the current graph has amassed a winning number of borders or corners
            if (std::bitset<6>(graph_map[current_graph_id].connected_borders).count() >= 3 || std::bitset<6>(graph_map[current_graph_id].connected_corners).count() >= 2) {
                // fork or bridge detected, game has been won by currentPlayer
                winner = true;
            }

            // perform actual move
            gameboard[target_tile.y][target_tile.x].color = current_player;
            gameboard[target_tile.y][target_tile.x].parent_graph_id = current_graph_id;

            // if current player is winner set appropriate state
            if (winner) {
                winning_player = current_player;
                current_player = COLOR_NONE;
            } else if (--remaining_tiles == 0) {
                winning_player = COLOR_NONE;
                current_player = COLOR_NONE;
            } else {
                // only really switch colors if the game is still going
                current_player = current_player == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE;
            }

            /*/ debug printing
            std::cout << "adj. graph count: " << std::to_string(adjacentGraphCount) << "\n";
            std::cout << "connected graphs: [" << std::to_string(adjacentGraphs[0]) << "] [" << std::to_string(adjacentGraphs[1]) << "] [" << std::to_string(adjacentGraphs[2]) << "]\n";
            std::cout << "started empty: " << std::to_string(emptyStart) << "\n";
            std::cout << "current graph id: " << std::to_string(currentGraphId) << "\n";
            std::cout << "tile contributing features: B[" << std::bitset<6>(contributionBorder) << "] C[" << std::bitset<6>(contributionCorner) << "]\n";
            std::cout << "current graph features: B[" << std::bitset<6>(graphMap[currentGraphId].connectedBorders) << "] C[" << std::bitset<6>(graphMap[currentGraphId].connectedCorners) << "]\n";*/
        }

        void Havannah::apply_internal_update(uint64_t update_id)
        {
            //TODO
        }

        uint8_t Havannah::get_result()
        {
            return winning_player;
        }

        void Havannah::discretize(uint64_t seed)
        {}
        
        uint8_t Havannah::perform_playout(uint64_t seed)
        {
            fast_prng rng(seed);
            while (player_to_move() != 0) {
                std::vector<uint64_t> moves_available = get_moves();
                apply_move(moves_available[rng.rand()%moves_available.size()]);
            }
            return get_result();
        }
        
        Game* Havannah::clone()
        {
            return new Havannah(*this);
        }
        
        void Havannah::copy_from(Game* target)
        {
            *this = *static_cast<Havannah*>(target);
        }
        
        uint64_t Havannah::get_move_id(std::string move_string)
        {
            return Tile_Id(move_string).to_move_id();
        }

        std::string Havannah::get_move_string(uint64_t move_id)
        {
            return Tile_Id(move_id).an();
        }

        void Havannah::debug_print()
        {
            switch (1) {
                case 0:
                    // square printing of the full gameboard matrix
                    for (int iy = 0; iy < board_sizer; iy++) {
                        for (int ix = 0; ix < board_sizer; ix++) {
                            std::cout << COLOR_CHARS[gameboard[iy][ix].color];
                        }
                        std::cout << "\n";
                    }
                    break;
                case 1:
                    // horizontally formatted printing
                    /*
                     x x
                    x x x
                     x x
                    */
                    for (int iy = 0; iy < board_sizer; iy++) {
                        int padding_count = (iy <= size-1) ? (size-1)-iy: iy-(size-1);
                        for (int ip = 0; ip < padding_count; ip++) {
                            std::cout << " ";
                        }
                        for (int ix = 0; ix < board_sizer; ix++) {
                            if ((ix - iy < size) && (iy - ix < size)) {
                                std::cout << " " << COLOR_CHARS[gameboard[iy][ix].color];
                            }
                        }
                        std::cout << "\n";
                    }
                    break;
                case 2:
                    // vertically formatted printing
                    /*
                        x
                    x       x
                        x
                    x       x
                        x
                    */
                    // from: https://www.techiedelight.com/print-matrix-diagonally-positive-slope/#comment-3071
                    for (int sum = 0; sum <= 2*(board_sizer-1); sum++) {
                        int r_end = std::min(sum, board_sizer-1);
                        int r = sum - std::min(sum, board_sizer-1);
                        int paddingCount = (sum < board_sizer) ? (board_sizer-r_end)-1: r;
                        for (int ip = 0; ip < paddingCount; ip++) {
                            std::cout << "    ";
                        }
                        while(r <= r_end) {
                            if (gameboard[r][sum-r].color == COLOR_INVALID) {
                                std::cout << "        ";
                                r++;
                                continue;
                            }
                            std::cout << COLOR_CHARS[gameboard[r++][sum-r].color] << "       ";
                        }
                        std::cout << "\n";
                    }
                    break;
            }
            
        }

        int Havannah::get_size()
        {
            return size;
        }
        
        Havannah::COLOR Havannah::get_cell(int x, int y)
        {
            return gameboard[y][x].color;
        }
    
}
