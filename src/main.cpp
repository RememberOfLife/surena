#include <cstdio>
// #include <iostream>
// #include <string>

// #include "surena/engines/singlethreadedmcts.hpp"
// #include "surena/games/alhambra.hpp"
// #include "surena/games/caesar.hpp"
// #include "surena/games/chess.hpp"
// #include "surena/games/tictactoe_ultimate.hpp"
// #include "surena/games/tictactoe.hpp"
// #include "surena/games/havannah.hpp"
#include "surena/util/semver.h"
// #include "surena/engine.hpp"
#include "surena/game.h"

namespace surena {
    const semver version = {0, 5, 0};
}

// args https://github.com/p-ranav/argparse

int main(int argc, char* argv[])
{
    printf("surena v%d.%d.%d\n", surena::version.major, surena::version.minor, surena::version.patch);
    // surena::Engine* engine = new surena::SinglethreadedMCTS();
    // surena::Game* thegame = new surena::TicTacToe();
    // engine->set_gamestate(thegame);
    // thegame->debug_print();
    // while (engine->player_to_move() != 0) {
    //     if (engine->player_to_move() == 2) {
    //         engine->search_start(5000);
    //         engine->debug_print();
    //         engine->apply_move(engine->get_best_move());
    //         thegame->debug_print();
    //         continue;
    //     }
    //     printf("player to move %d: ", engine->player_to_move());
    //     std::string move_string;
    //     std::cin >> move_string;
    //     engine->apply_move(engine->get_move_id(move_string));
    //     thegame->debug_print();
    // }
    // printf("winning player: %d\n", engine->get_result());
    // delete engine;
    printf("done\n");
    return 0;
}
