#include <iostream>
#include <string>

#include "surena/game.hpp"
#include "surena/engine.hpp"
#include "surena/engines/singlethreadedmcts.hpp"
#include "surena/games/alhambra.hpp"
#include "surena/games/caesar.hpp"
#include "surena/games/tictactoe.hpp"
#include "surena/games/havannah.hpp"

namespace surena {
    const char* version = "0.1.0";
}

// args https://github.com/p-ranav/argparse

int main()
{
    surena::PerfectInformationEngine* engine = new surena::SinglethreadedMCTS();
    surena::PerfectInformationGame* thegame = new surena::Havannah(3);
    engine->set_gamestate(thegame);
    while (engine->player_to_move() != 0) {
        if (engine->player_to_move() == 2) {
            engine->search_start();
            engine->debug_print();
            engine->apply_move(engine->get_best_move());
            thegame->debug_print();
            continue;
        }
        printf("player to move: %d\n", engine->player_to_move());
        std::string move_string;
        std::cin >> move_string;
        engine->apply_move(engine->get_move_id(move_string));
        thegame->debug_print();
    }
    printf("winning player: %d\n", engine->get_result());
    delete engine;
    std::cout << "done\n";
}
