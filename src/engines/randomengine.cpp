#include "surena/util/fast_prng.hpp"

#include "surena/engines/randomengine.hpp"

namespace surena {

    RandomEngine::RandomEngine():
        gamestate(NULL),
        rng(42),
        rand_ctr(rng.rand())
    {}

    RandomEngine::~RandomEngine()
    {
        delete gamestate;
    }

    void RandomEngine::search_start(uint64_t ms_timeout)
    {}


    void RandomEngine::search_stop()
    {}
    
    uint64_t RandomEngine::get_best_move()
    {
        if (!gamestate) {
            return 0;
        }
        std::vector<uint64_t> available_moves = this->get_moves();
        return available_moves[rand_ctr%available_moves.size()];
    }

    void RandomEngine::set_gamestate(Game* target_gamestate)
    {
        gamestate = target_gamestate;
        rand_ctr = rng.rand();
    }

    uint8_t RandomEngine::player_to_move()
    {
        if (!gamestate) {
            return 0;
        }
        return gamestate->player_to_move();
    }

    std::vector<uint64_t> RandomEngine::get_moves()
    {
        if (!gamestate) {
            return std::vector<uint64_t>{};
        }
        return gamestate->get_moves();
    }

    void RandomEngine::apply_move(uint64_t move_id)
    {
        if (!gamestate) {
            return;
        }
        gamestate->apply_move(move_id);
        rand_ctr = rng.rand();
    }

    uint8_t RandomEngine::get_result()
    {
        if (!gamestate) {
            return 0;
        }
        return gamestate->get_result();
    }

    uint64_t RandomEngine::get_move_id(std::string move_string)
    {
        if (!gamestate) {
            return 0;
        }
        return gamestate->get_move_id(move_string);
    }

    std::string RandomEngine::get_move_string(uint64_t move_id)
    {
        if (!gamestate) {
            return "-";
        }
        return gamestate->get_move_string(move_id);
    }

    void RandomEngine::debug_print()
    {}

}