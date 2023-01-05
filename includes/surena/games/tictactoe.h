#pragma once

#include <stdint.h>

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

//TODO fix X/O enum

typedef struct tictactoe_internal_methods_s {

    // x grows right, y grows up
    error_code (*get_cell)(game* self, int x, int y, player_id* p);
    error_code (*set_cell)(game* self, int x, int y, player_id p);
    error_code (*set_current_player)(game* self, player_id p);
    error_code (*set_result)(game* self, player_id p);

} tictactoe_internal_methods;

extern const game_methods tictactoe_standard_gbe;

#ifdef __cplusplus
}
#endif
