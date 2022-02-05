#include "surena/util/fast_prng.hpp"

#include "surena/engines/randomengine.hpp"

namespace surena {

    RandomEngine::RandomEngine():
        rng(42)
    {}

    RandomEngine::~RandomEngine()
    {
        delete gamestate;
    }

    void RandomEngine::search_start() {}


    void RandomEngine::search_stop() {}
    
    uint64_t RandomEngine::get_best_move()
    {
        std::vector<uint64_t> available_moves = this->get_moves();
        return available_moves[rng.rand()%available_moves.size()];
    }

    void RandomEngine::set_gamestate(PerfectInformationGame* target_gamestate)
    {
        gamestate = target_gamestate;
    }

    uint8_t RandomEngine::player_to_move()
    {
        return gamestate->player_to_move();
    }

    std::vector<uint64_t> RandomEngine::get_moves()
    {
        return gamestate->get_moves();
    }

    void RandomEngine::apply_move(uint64_t move_id)
    {
        gamestate->apply_move(move_id);
    }

    uint8_t RandomEngine::get_result()
    {
        return gamestate->get_result();
    }

    uint64_t RandomEngine::get_move_id(std::string move_string)
    {
        return gamestate->get_move_id(move_string);
    }

    std::string RandomEngine::get_move_string(uint64_t move_id)
    {
        return gamestate->get_move_string(move_id);
    }

    void RandomEngine::debug_print()
    {}

}