#include <cstdint>

#include "surena/game.h"

#include "surena/game.hpp"

namespace surena {

    const char* Game::get_error_string(error_code err)
    {
        return gbe.methods->get_error_string(err);
    }

    error_code Game::init() 
    {
        return gbe.methods->init(&gbe);
    }

    error_code Game::free() 
    {
        return gbe.methods->free(&gbe);
    }

    error_code Game::clone(game** ret_clone) 
    {
        return gbe.methods->clone(&gbe, ret_clone);
    }

    error_code Game::copy_from(game* other) 
    {
        return gbe.methods->copy_from(&gbe, other);
    }

    error_code Game::compare(game* other, bool ret_equal) 
    {
        return gbe.methods->compare(&gbe, other, ret_equal);
    }

    error_code Game::import_state(const char* str) 
    {
        return gbe.methods->import_state(&gbe, str);
    }

    error_code Game::export_state(size_t* ret_size, char* str) 
    {
        return gbe.methods->export_state(&gbe, ret_size, str);
    }

    error_code Game::get_player_count(uint8_t* ret_count) 
    {
        return gbe.methods->get_player_count(&gbe, ret_count);
    }

    error_code Game::players_to_move(uint8_t* ret_count, player_id* players) 
    {
        return gbe.methods->players_to_move(&gbe, ret_count, players);
    }

    error_code Game::get_concrete_moves(uint32_t* ret_count, move_code* moves, player_id player) 
    {
        return gbe.methods->get_concrete_moves(&gbe, ret_count, moves, player);
    }

    error_code Game::get_concrete_move_probabilities(uint32_t* ret_count, float* move_probabilities, player_id player) 
    {
        return gbe.methods->get_concrete_move_probabilities(&gbe, ret_count, move_probabilities, player);
    }

    error_code Game::get_concrete_moves_ordered(uint32_t* ret_count, move_code* moves, player_id player) 
    {
        return gbe.methods->get_concrete_moves_ordered(&gbe, ret_count, moves, player);
    }

    error_code Game::get_actions(uint32_t* ret_count, move_code* moves, player_id player) 
    {
        return gbe.methods->get_actions(&gbe, ret_count, moves, player);
    }

    error_code Game::is_legal_move(player_id player, move_code move, uint32_t sync_ctr) 
    {
        return gbe.methods->is_legal_move(&gbe, player, move, sync_ctr);
    }

    error_code Game::move_to_action(move_code* ret_action, move_code move) 
    {
        return gbe.methods->move_to_action(&gbe, ret_action, move);
    }

    error_code Game::is_action(bool* ret_is_action, move_code move) 
    {
        return gbe.methods->is_action(&gbe, ret_is_action, move);
    }
    error_code Game::make_move(player_id player, move_code move) 
    {
        return gbe.methods->make_move(&gbe, player, move);
    }

    error_code Game::get_results(uint8_t* ret_count, player_id* players) 
    {
        return gbe.methods->get_results(&gbe, ret_count, players);
    }

    error_code Game::id(uint64_t* ret_id) 
    {
        return gbe.methods->id(&gbe, ret_id);
    }

    error_code Game::eval(float* ret_eval, player_id player) 
    {
        return gbe.methods->eval(&gbe, ret_eval, player);
    }

    error_code Game::discretize(uint64_t seed) 
    {
        return gbe.methods->discretize(&gbe, seed);
    }

    error_code Game::playout(uint64_t seed) 
    {
        return gbe.methods->playout(&gbe, seed);
    }

    error_code Game::redact_keep_state(player_id* players, uint32_t count) 
    {
        return gbe.methods->redact_keep_state(&gbe, players, count);
    }

    error_code Game::get_move_code(move_code* ret_move, const char* str) 
    {
        return gbe.methods->get_move_code(&gbe, ret_move, str);
    }

    error_code Game::get_move_str(size_t* ret_size, char* str_buf, move_code move) 
    {
        return gbe.methods->get_move_str(&gbe, ret_size, str_buf, move);
    }

    error_code Game::debug_print(size_t* ret_size, char* str_buf) 
    {
        return gbe.methods->debug_print(&gbe, ret_size, str_buf);
    }

}
