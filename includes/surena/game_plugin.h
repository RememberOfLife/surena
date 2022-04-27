#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "surena/game.h"

// returns the capi version used to build the plugin
uint64_t plugin_get_game_capi_version();

// writes the plugin static pointers to the game methods this plugin brings to methods
// if methods is NULL then count returns the number of methods this may write
// otherwise count returns the number of methods written
void plugin_get_game_methods(uint32_t* count, game_methods* methods);

#ifdef __cplusplus
}
#endif
