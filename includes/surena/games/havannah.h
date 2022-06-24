#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "surena/game.h"

/*
Havannah: (perfect information) [2P]
played on a hexagonal board with hexagonal tiles, edge length n of (10/8/6), tile count = 3 * n**2 - ( 3n - 1 )
black and white sequentially each place a piece of their color on any empty tile on the board
the player wins that first creates either a ring, bridge or fork structure from unbroken lines of connected stones of their color
a ring is a loop around one or more cells the occupation status of whom does not matter
a bride connects any two of the six corner cells of the board
a fork connects any three edges of the board, corner points are not parts of any edge
because the first move has advantage, the opponent may either switch the colors or let the move stand (switch rule)
the weaker player may be allowed to place more than 1 stone on their first turn if desired, as compensation
*/

enum HAVANNAH_PLAYER : player_id {
    HAVANNAH_PLAYER_NONE = 0,
    HAVANNAH_PLAYER_WHITE,
    HAVANNAH_PLAYER_BLACK,
    HAVANNAH_PLAYER_INVALID,
};

extern const char HAVANNAH_PLAYER_CHARS[4];

typedef struct havannah_tile_s {
    HAVANNAH_PLAYER color : 2;
    uint8_t parent_graph_id;
    //TODO maybe store existing neighbor tiles as bitmap for every tile (preferred), or store feature contribution of every tile, this is a bijection
} havannah_tile;

typedef struct havannah_graph_s {
    // a joined graph no longer is its own parent, instead it points to the true graph which it connects to
    uint8_t parent_graph_id;
    uint8_t connected_borders;
    uint8_t connected_corners;
} havannah_graph;

//TODO cap size to min and max limits for this impl / error code?
typedef struct havannah_options_s {
    int size;
    int board_sizer; // 2 * size - 1 //TODO this shouldn't really be in the public options struct
    //TODO option for pie rule
    //TODO option for letting one player play n many stones on their first turn instead of just 1
} havannah_options;

typedef struct havannah_internal_methods_s {

    error_code (*get_cell)(game* self, int x, int y, HAVANNAH_PLAYER* p);
    error_code (*set_cell)(game* self, int x, int y, HAVANNAH_PLAYER p, bool* wins);

    error_code (*get_size)(game* self, int* size);

} havannah_internal_methods;

extern const game_methods havannah_gbe;

#ifdef __cplusplus
}
#endif
