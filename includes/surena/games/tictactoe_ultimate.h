#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "surena/game.h"

typedef struct tictactoe_ultimate_internal_methods_s {

    // x grows right, y grows up
    error_code (*check_result)(game* self, uint32_t state, player_id* ret_p);
    error_code (*get_cell)(game* self, uint32_t state, int x, int y, player_id* ret_p);
    error_code (*set_cell)(game* self, uint32_t* state, int x, int y, player_id p);

    error_code (*get_cell_global)(game* self, int x, int y, player_id* ret_p);
    error_code (*set_cell_global)(game* self, int x, int y, player_id p);

    error_code (*get_cell_local)(game* self, int x, int y, player_id* ret_p);
    error_code (*set_cell_local)(game* self, int x, int y, player_id p);

    error_code (*get_global_target)(game* self, uint8_t* ret); // yyxx in least significant bits, 3 is none
    error_code (*set_global_target)(game* self, int x, int y); // any == -1 is none

    error_code (*set_current_player)(game* self, player_id p);
    error_code (*set_result)(game* self, player_id p);

} tictactoe_ultimate_internal_methods;

extern const game_methods tictactoe_ultimate_gbe;

#ifdef __cplusplus
}
#endif
