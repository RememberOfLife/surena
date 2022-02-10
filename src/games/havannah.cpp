#include <array>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <string>

#include "surena/util/fast_prng.hpp"

#include "surena/games/havannah.hpp"

namespace surena {

        const char COLOR_CHARS[5] = ".-OX"; // invalid, none, white, black

        TileId::TileId(std::string an)
        {
            this->x = an[0]-'A';
            an.replace(0, 1, "");
            this->y = stoi(an);
        }

        TileId::TileId(int x, int y):
            x(static_cast<uint_fast8_t>(x)),
            y(static_cast<uint_fast8_t>(y))
        {}

        TileId::TileId(uint64_t move_code):
            x(static_cast<int>(move_code >> 8) & 0xFF),
            y(static_cast<int>(move_code & 0xFF))
        {}

        std::string TileId::an()
        {
            return std::to_string(this->y).insert(0, 1, this->x+'A');
        }

        uint64_t TileId::to_move_id()
        {
            return static_cast<uint64_t>((static_cast<uint64_t>(x) << 8) | y);
        }

        Havannah::Havannah(int size):
            size(size),
            boardSizer(2*size-1),
            remainingTiles((boardSizer*boardSizer)-(size*(size-1))),
            currentPlayer(COLOR_WHITE),
            winningPlayer(COLOR_INVALID),
            nextGraphId(1)
        {
            gameboard.reserve(boardSizer);
            for (int iy = 0; iy < boardSizer; iy++) {
                std::vector<Tile> tileRowVector{};
                tileRowVector.resize(boardSizer, Tile{COLOR_INVALID, 0});
                gameboard.push_back(std::move(tileRowVector));
                for (int ix = 0; ix < boardSizer; ix++) {
                    if ((ix - iy < size) && (iy - ix < size)) { // magic formula for only enabling valid cells of the board
                        gameboard[iy][ix].color = COLOR_NONE;
                    }
                }
            }
        }

        uint8_t Havannah::player_to_move()
        {
            return currentPlayer;
        }

        std::vector<uint64_t> Havannah::get_moves()
        {
            std::vector<uint64_t> freeTiles{};
            for (int iy = 0; iy < boardSizer; iy++) {
                for (int ix = 0; ix < boardSizer; ix++) {
                    if (gameboard[iy][ix].color == COLOR_NONE) {
                        // add the free tile to the return vector
                        freeTiles.push_back(TileId(ix, iy).to_move_id());
                    }
                }
            }
            return freeTiles;
        }

        void Havannah::apply_move(uint64_t m)
        {
            TileId targetTile = TileId(m);

            bool winner = false; // if this is true by the end, current player wins

            uint8_t contributionBorder = 0b00111111; // begin on the north-west corner and it's left border, going counter-clockwise
            uint8_t contributionCorner = 0b00111111; 
            std::array<uint8_t, 3> adjacentGraphs = {0, 0, 0}; // there can only be a maximum of 3 graphs with gaps in between
            int adjacentGraphCount = 0;
            bool emptyStart = true; // true if the first neighbor tile is non-same color
            uint8_t currentGraphId = 0; // 0 represents a gap, any value flags an ongoing streak which has to be saved before resetting the flag to 0 for a gap

            // discover north-west neighbor
            if (targetTile.y > 0 && targetTile.x > 0) {
                // exists
                if (gameboard[targetTile.y-1][targetTile.x-1].color == currentPlayer) {
                    currentGraphId = gameboard[targetTile.y-1][targetTile.x-1].parentGraphId;
                    emptyStart = false;
                }/* else {
                    // not a same colored piece, save previous streak and set gap
                    if (currentGraphId != 0) {
                        adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                    }
                    currentGraphId = 0;
                }*/
                contributionBorder &= 0b00011110;
                contributionCorner &= 0b00001110;
            } else {
                // invalid, narrow possible contribution features and save previous streak
                contributionBorder &= 0b00100001;
                contributionCorner &= 0b00110001;
                /*if (currentGraphId != 0) {
                    adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                }
                currentGraphId = 0;*/
            }

            // discover west neighbor
            if (targetTile.x > 0 && (targetTile.y - targetTile.x) < size-1) {
                if (gameboard[targetTile.y][targetTile.x-1].color == currentPlayer) {
                    currentGraphId = gameboard[targetTile.y][targetTile.x-1].parentGraphId;
                } else {
                    if (currentGraphId != 0) {
                        adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                    }
                    currentGraphId = 0;
                }
                contributionBorder &= 0b00001111;
                contributionCorner &= 0b00000111;
            } else {
                contributionBorder &= 0b00110000;
                contributionCorner &= 0b00111000;
                if (currentGraphId != 0) {
                    adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                }
                currentGraphId = 0;
            }

            // discover south-west neighbor
            if (targetTile.y < boardSizer-1 && (targetTile.y - targetTile.x < size-1)) {
                if (gameboard[targetTile.y+1][targetTile.x].color == currentPlayer) {
                    currentGraphId = gameboard[targetTile.y+1][targetTile.x].parentGraphId;
                } else {
                    if (currentGraphId != 0) {
                        adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                    }
                    currentGraphId = 0;
                }
                contributionBorder &= 0b00100111;
                contributionCorner &= 0b00100011;
            } else {
                contributionBorder &= 0b00011000;
                contributionCorner &= 0b00011100;
                if (currentGraphId != 0) {
                    adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                }
                currentGraphId = 0;
            }
            
            // discover south-east neighbor
            if (targetTile.y < boardSizer-1 && targetTile.x < boardSizer-1) {
                if (gameboard[targetTile.y+1][targetTile.x+1].color == currentPlayer) {
                    currentGraphId = gameboard[targetTile.y+1][targetTile.x+1].parentGraphId;
                } else {
                    if (currentGraphId != 0) {
                        adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                    }
                    currentGraphId = 0;
                }
                contributionBorder &= 0b00110011;
                contributionCorner &= 0b00110001;
            } else {
                contributionBorder &= 0b00001100;
                contributionCorner &= 0b00001110;
                if (currentGraphId != 0) {
                    adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                }
                currentGraphId = 0;
            }

            // discover east neighbor
            if (targetTile.x < boardSizer-1 && (targetTile.x - targetTile.y < size-1)) {
                if (gameboard[targetTile.y][targetTile.x+1].color == currentPlayer) {
                    currentGraphId = gameboard[targetTile.y][targetTile.x+1].parentGraphId;
                } else {
                    if (currentGraphId != 0) {
                        adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                    }
                    currentGraphId = 0;
                }
                contributionBorder &= 0b00111001;
                contributionCorner &= 0b00111000;
            } else {
                contributionBorder &= 0b00000110;
                contributionCorner &= 0b00000111;
                if (currentGraphId != 0) {
                    adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                }
                currentGraphId = 0;
            }

            // discover north-east neighbor
            if (targetTile.y > 0 && (targetTile.x - targetTile.y < size-1)) {
                if (gameboard[targetTile.y-1][targetTile.x].color == currentPlayer) {
                    currentGraphId = gameboard[targetTile.y-1][targetTile.x].parentGraphId;
                } else {
                    if (currentGraphId != 0) {
                        adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                    }
                    currentGraphId = 0;
                }
                contributionBorder &= 0b00111100;
                contributionCorner &= 0b00011100;
            } else {
                contributionBorder &= 0b00000011;
                contributionCorner &= 0b00100011;
                if (currentGraphId != 0) {
                    adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                }
                currentGraphId = 0;
            }

            // simulate a gap, to properly save last streak if requird by empty start
            // non-empty start will just discard the last streak which would be a dupe of itself anyway
            if (emptyStart && currentGraphId != 0) {
                adjacentGraphs[adjacentGraphCount++] = currentGraphId;
            }

            //##### evaluate neighbor scanning results to update state

            // resolve all existing adjacent graphs to their true parentGraphId
            for (int i = 0; i < adjacentGraphCount; i++) {
                while (adjacentGraphs[i] != graphMap[adjacentGraphs[i]].parentGraphId) {
                    //TODO any good way to keep the indirections down, without looping over the whole graphMap when reparenting?
                    adjacentGraphs[i] = graphMap[adjacentGraphs[i]].parentGraphId;
                }
            }
            
            // set current graph to the first discovered, if any
            if (adjacentGraphCount > 0) {
                currentGraphId = adjacentGraphs[0];
            }

            // check for ring creation amongst adjacent graphs
            if (
                (adjacentGraphs[0] != 0 && adjacentGraphs[1] != 0 && adjacentGraphs[0] == adjacentGraphs[1]) ||
                (adjacentGraphs[0] != 0 && adjacentGraphs[2] != 0 && adjacentGraphs[0] == adjacentGraphs[2]) ||
                (adjacentGraphs[2] != 0 && adjacentGraphs[1] != 0 && adjacentGraphs[2] == adjacentGraphs[1])
            ) {
                // ring detected, game has been won by currentPlayer
                winner = true;
            }

            // no adjacent graphs, create a new one
            if (adjacentGraphCount == 0) {
                    currentGraphId = nextGraphId++;
                    graphMap[currentGraphId].parentGraphId = currentGraphId;
            }

            // 2 or 3 adjacent graphs, all get reparented and contribute their features
            for (int i = 1; i < adjacentGraphCount; i++) {
                    graphMap[adjacentGraphs[i]].parentGraphId = currentGraphId;
                    graphMap[currentGraphId].connectedBorders |= graphMap[adjacentGraphs[i]].connectedBorders;
                    graphMap[currentGraphId].connectedCorners |= graphMap[adjacentGraphs[i]].connectedCorners;
            }

            // contribute target tile features
            if (contributionBorder == 0b00000000) {
                graphMap[currentGraphId].connectedCorners |= contributionCorner;
            } else {
                graphMap[currentGraphId].connectedBorders |= contributionBorder;
            }

            // check if the current graph has amassed a winning number of borders or corners
            if (std::bitset<6>(graphMap[currentGraphId].connectedBorders).count() >= 3 || std::bitset<6>(graphMap[currentGraphId].connectedCorners).count() >= 2) {
                // fork or bridge detected, game has been won by currentPlayer
                winner = true;
            }

            // perform actual move
            gameboard[targetTile.y][targetTile.x].color = currentPlayer;
            gameboard[targetTile.y][targetTile.x].parentGraphId = currentGraphId;

            // if current player is winner set appropriate state
            if (winner) {
                winningPlayer = currentPlayer;
                currentPlayer = COLOR_NONE;
            } else if (--remainingTiles == 0) {
                winningPlayer = COLOR_NONE;
                currentPlayer = COLOR_NONE;
            } else {
                // only really switch colors if the game is still going
                currentPlayer = currentPlayer == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE;
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
            return winningPlayer;
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
            return TileId(move_string).to_move_id();
        }

        std::string Havannah::get_move_string(uint64_t move_id)
        {
            return TileId(move_id).an();
        }

        void Havannah::debug_print()
        {
            switch (1) {
                case 0:
                    // square printing of the full gameboard matrix
                    for (int iy = 0; iy < boardSizer; iy++) {
                        for (int ix = 0; ix < boardSizer; ix++) {
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
                    for (int iy = 0; iy < boardSizer; iy++) {
                        int paddingCount = (iy <= size-1) ? (size-1)-iy: iy-(size-1);
                        for (int ip = 0; ip < paddingCount; ip++) {
                            std::cout << " ";
                        }
                        for (int ix = 0; ix < boardSizer; ix++) {
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
                    for (int sum = 0; sum <= 2*(boardSizer-1); sum++) {
                        int r_end = std::min(sum, boardSizer-1);
                        int r = sum - std::min(sum, boardSizer-1);
                        int paddingCount = (sum < boardSizer) ? (boardSizer-r_end)-1: r;
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
    
}
