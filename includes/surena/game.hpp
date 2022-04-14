#pragma once

#include <cstdint>

#include "surena/game.h"

namespace surena {

    class Game {

        public:
            
            game gbe; // game backend

            // game names version and features are directly accessed from the game backend

            virtual ~Game() = default;

            const char* get_error_string(error_code err);

            error_code init();

            error_code free();

            error_code clone(game** ret_clone);

            error_code copy_from(game* other);

            error_code compare(game* other, bool ret_equal);

            error_code import_state(const char* str);

            error_code export_state(size_t* ret_size, char* str);

            error_code get_player_count(uint8_t* ret_count);

            error_code players_to_move(uint8_t* ret_count, player_id* players);

            error_code get_concrete_moves(uint32_t* ret_count, move_code* moves, player_id player);

            error_code get_concrete_move_probabilities(uint32_t* ret_count, float* move_probabilities, player_id player);

            error_code get_concrete_moves_ordered(uint32_t* ret_count, move_code* moves, player_id player);

            error_code get_actions(uint32_t* ret_count, move_code* moves, player_id player);

            error_code is_legal_move(player_id player, move_code move, uint32_t sync_ctr);

            error_code move_to_action(move_code* ret_action, move_code move);

            error_code is_action(bool* ret_is_action, move_code move);
            error_code make_move(player_id player, move_code move);

            error_code get_results(uint8_t* ret_count, player_id* players);

            error_code id(uint64_t* ret_id);

            error_code eval(float* ret_eval, player_id player);

            error_code discretize(uint64_t seed);

            error_code playout(uint64_t seed);

            error_code redact_keep_state(player_id* players, uint32_t count);

            error_code get_move_code(move_code* ret_move, const char* str);

            error_code get_move_str(size_t* ret_size, char* str_buf, move_code move);

            error_code debug_print(size_t* ret_size, char* str_buf);

            // internal methods are exposed by every Game on its own, or accessed through the game backend

    };

}
