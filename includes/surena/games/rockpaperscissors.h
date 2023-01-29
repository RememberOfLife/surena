#pragma once

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint8_t ROCKPAPERSCISSORS_NONE = 0;
static const uint8_t ROCKPAPERSCISSORS_ANY = 1;
static const uint8_t ROCKPAPERSCISSORS_ROCK = 2;
static const uint8_t ROCKPAPERSCISSORS_PAPER = 3;
static const uint8_t ROCKPAPERSCISSORS_SCISSOR = 4;

typedef struct rockpaperscissors_internal_methods_s {

    error_code (*get_played)(game* self, player_id p, uint8_t* m);
    error_code (*calc_done)(game* self);

} rockpaperscissors_internal_methods;

extern const game_methods rockpaperscissors_standard_gbe;

#ifdef __cplusplus
}
#endif
