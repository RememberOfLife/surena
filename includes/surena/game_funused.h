#pragma once

#include "surena/game.h"

// provide typedefs for all the game functions
typedef const char* (*get_error_string_gf_t)(error_code err);
typedef error_code (*create_gf_t)(game* self);
typedef error_code (*destroy_gf_t)(game* self);
typedef error_code (*clone_gf_t)(game* self, game** ret_clone);
typedef error_code (*copy_from_gf_t)(game* self, game* other);
typedef error_code (*compare_gf_t)(game* self, game* other, bool* ret_equal);
typedef error_code (*import_state_gf_t)(game* self, const char* str);
typedef error_code (*export_state_gf_t)(game* self, size_t* ret_size, char* str);
typedef error_code (*get_player_count_gf_t)(game* self, uint8_t* ret_count);
typedef error_code (*players_to_move_gf_t)(game* self, uint8_t* ret_count, player_id* players);
typedef error_code (*get_concrete_moves_gf_t)(game* self, uint32_t* ret_count, move_code* moves, player_id player);
typedef error_code (*get_concrete_move_probabilities_gf_t)(game* self, uint32_t* ret_count, float* move_probabilities, player_id player);
typedef error_code (*get_concrete_moves_ordered_gf_t)(game* self, uint32_t* ret_count, move_code* moves, player_id player);
typedef error_code (*get_actions_gf_t)(game* self, uint32_t* ret_count, move_code* moves, player_id player);
typedef error_code (*is_legal_move_gf_t)(game* self, player_id player, move_code move, uint32_t sync_ctr);
typedef error_code (*move_to_action_gf_t)(game* self, move_code* ret_action, move_code move);
typedef error_code (*is_action_gf_t)(game* self, bool* ret_is_action, move_code move);
typedef error_code (*make_move_gf_t)(game* self, player_id player, move_code move);
typedef error_code (*get_results_gf_t)(game* self, uint8_t* ret_count, player_id* players);
typedef error_code (*id_gf_t)(game* self, uint64_t* ret_id);
typedef error_code (*eval_gf_t)(game* self, float* ret_eval, player_id player);
typedef error_code (*discretize_gf_t)(game* self, uint64_t seed);
typedef error_code (*playout_gf_t)(game* self, uint64_t seed);
typedef error_code (*redact_keep_state_gf_t)(game* self, player_id* players, uint32_t count);
typedef error_code (*get_move_code_gf_t)(game* self, move_code* ret_move, const char* str);
typedef error_code (*get_move_str_gf_t)(game* self, size_t* ret_size, char* str_buf, move_code move);
typedef error_code (*debug_print_gf_t)(game* self, size_t* ret_size, char* str_buf);

// unused game function macro so we can correctly set unimplemented features to NULL
#define GF_UNUSED(gfname) static void* _##gfname = NULL
