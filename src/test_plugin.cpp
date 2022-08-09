/*
    compile from the project root with:
    g++ -g -fPIC -shared src/test_plugin.cpp src/games/tictactoe.cpp src/util/fast_prng.cpp src/util/noise.cpp src/util/semver.c src/game.c -Iincludes -o build/test_plugin.so
    this provides standard tictactoe as a test for the plugin loading system
*/

#include "surena/games/tictactoe.h"
#include "surena/game_plugin.h"
#include "surena/game.h"

uint64_t plugin_get_game_capi_version()
{
    return SURENA_GAME_API_VERSION;
}

void plugin_init_game()
{}

void plugin_get_game_methods(uint32_t* count, const game_methods** methods)
{
    *count = 1;
    if (methods == NULL) {
        return;
    }
    methods[0] = &tictactoe_gbe;
}

void plugin_cleanup_game()
{}
