#pragma once

#include "surena/game.h"

// provide typedefs for all the game functions
typedef const char* (*get_last_error_gf_t)(game* self);
typedef error_code (*create_with_opts_str_gf_t)(game* self, const char* str);
typedef error_code (*create_with_opts_bin_gf_t)(game* self, void* options_struct);
typedef error_code (*create_deserialize_gf_t)(game* self, char* buf);
typedef error_code (*create_default_gf_t)(game* self);
typedef error_code (*export_options_str_gf_t)(game* self, size_t* ret_size, char* str);
typedef error_code (*get_options_bin_ref_gf_t)(game* self, void** ret_bin_ref);
typedef error_code (*destroy_gf_t)(game* self);
typedef error_code (*clone_gf_t)(game* self, game* clone_target);
typedef error_code (*copy_from_gf_t)(game* self, game* other);
typedef error_code (*compare_gf_t)(game* self, game* other, bool* ret_equal);
typedef error_code (*import_state_gf_t)(game* self, const char* str);
typedef error_code (*export_state_gf_t)(game* self, size_t* ret_size, char* str);
typedef error_code (*serialize_gf_t)(game* self, size_t* ret_size, char* buf);
typedef error_code (*players_to_move_gf_t)(game* self, uint8_t* ret_count, player_id* players);
typedef error_code (*get_concrete_moves_gf_t)(game* self, player_id player, uint32_t* ret_count, move_code* moves);
typedef error_code (*get_concrete_move_probabilities_gf_t)(game* self, player_id player, uint32_t* ret_count, float* move_probabilities);
typedef error_code (*get_concrete_moves_ordered_gf_t)(game* self, player_id player, uint32_t* ret_count, move_code* moves);
typedef error_code (*get_actions_gf_t)(game* self, player_id player, uint32_t* ret_count, move_code* moves);
typedef error_code (*is_legal_move_gf_t)(game* self, player_id player, move_code move, sync_counter sync);
typedef error_code (*move_to_action_gf_t)(game* self, move_code move, move_code* ret_action);
typedef error_code (*is_action_gf_t)(game* self, move_code move, bool* ret_is_action);
typedef error_code (*make_move_gf_t)(game* self, player_id player, move_code move);
typedef error_code (*get_results_gf_t)(game* self, uint8_t* ret_count, player_id* players);
typedef error_code (*get_sync_counter_gf_t)(game* self, sync_counter* ret_sync);
typedef error_code (*id_gf_t)(game* self, uint64_t* ret_id);
typedef error_code (*eval_gf_t)(game* self, player_id player, float* ret_eval);
typedef error_code (*discretize_gf_t)(game* self, uint64_t seed);
typedef error_code (*playout_gf_t)(game* self, uint64_t seed);
typedef error_code (*redact_keep_state_gf_t)(game* self, uint8_t count, player_id* players);
typedef error_code (*export_sync_data_gf_t)(game* self, sync_data** sync_data_start, sync_data** sync_data_end);
typedef error_code (*release_sync_data_gf_t)(game* self, sync_data* sync_data_start, sync_data* sync_data_end);
typedef error_code (*import_sync_data_gf_t)(game* self, void* data_start, void* data_end);
typedef error_code (*get_move_code_gf_t)(game* self, player_id player, const char* str, move_code* ret_move);
typedef error_code (*get_move_str_gf_t)(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
typedef error_code (*debug_print_gf_t)(game* self, size_t* ret_size, char* str_buf);

// unused game function macro so we can correctly set unimplemented features to NULL
#define GF_UNUSED(gfname) static void* gfname = NULL
