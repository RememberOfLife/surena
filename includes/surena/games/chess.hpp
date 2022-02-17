#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "surena/game.hpp"

namespace surena {

    class Chess : public Game {
        
        public:

            enum PLAYER : uint8_t {
                PLAYER_NONE = 0,
                PLAYER_WHITE,
                PLAYER_BLACK,
            };

            enum RESULT : uint8_t {
                RESULT_DRAW = 0,
                RESULT_WHITE,
                RESULT_BLACK,
            };

            enum PIECE_TYPE : uint8_t {
                PIECE_TYPE_NONE = 0,
                PIECE_TYPE_KING,
                PIECE_TYPE_QUEEN,
                PIECE_TYPE_ROOK,
                PIECE_TYPE_BISHOP,
                PIECE_TYPE_KNIGHT,
                PIECE_TYPE_PAWN,
            };

            static const char PIECE_TYPE_CHARS[7];

            struct piece {
                PLAYER player : 2;
                PIECE_TYPE type : 3;
            };

        private:

            uint8_t current_player = PLAYER_WHITE;
            uint8_t winning_player = RESULT_DRAW;
            piece board[8][8]; // board[y][x] starting with origin (0,0) on bottom left of the board
            uint32_t halfmove_clock = 0;
            uint32_t fullmove_clock = 1;
            uint8_t enpassant_target = 0xFF; // left nibble is x, right nibble is y
            bool castling_white_king : 1;
            bool castling_white_queen : 1;
            bool castling_black_king : 1;
            bool castling_black_queen : 1;

        public:

            Chess();
            
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

            //#####
            // game specific exposed functions
            
            // get piece value of cell (x grows right, y grows up)
            piece get_cell(int x, int y);
            void set_cell(int x, int y, piece p);
            void set_current_player(PLAYER p);
            void set_result(PLAYER p);

            uint64_t count_positions(int depth); // simple perft

        private:

            void apply_move_internal(uint64_t move_id, bool replace_castling_by_kings = false);

            std::vector<uint64_t> get_moves_pseudo_legal();

    };

}
