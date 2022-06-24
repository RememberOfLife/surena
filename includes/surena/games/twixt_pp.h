#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "surena/game.h"

/*

TODO interesting evals to try:

1. classic distance to win:
    count the required number of virtual/real connections for a win for that player
    possibly only count unblockable (unobstructed distances)?
    look at D*

2. largest directional spread:
    .. of a players graphs (horizontal for horizontal player, vice-versa for vertical)
    i.e. maximum of all graph widths
    for each graph that contains multiple nodes this is (point most to the right X) minus (point most to the left X)
    (and Y with high and low for vertical player)
    - need to do this using virtual connections too

3. minimum open connection space:
    e.g. for the vertical player
    for each column, once starting from the top and once starting from the bottom
    e.g. for starting from the top, start counting at 0
        step one node lower and check its type
            free: count up 1 and step lower
            color of the player that wants to win in this direction: stop and return count
            color of the player trying to block a connection in this direction: stop and return count.max
        for all the columns, get the top count and bottom count, (min of top counts) + (min of bot counts) = eval score

*/

enum TWIXT_PP_PLAYER : player_id {
    TWIXT_PP_PLAYER_NONE = 0,
    TWIXT_PP_PLAYER_WHITE,
    TWIXT_PP_PLAYER_BLACK,
    TWIXT_PP_PLAYER_INVALID,
};

typedef struct twixt_pp_node_s {
    TWIXT_PP_PLAYER player : 2;
    uint16_t graph_id : 14; // 14 bits are enough for boards up to 128x128
    // order for these: 0b0000XYZW
    // X right top, Y right bottom, Z bottom right, W bottom left
    uint8_t connections : 4; // stores the right and downward facing connections that exist
    uint8_t collisions : 4; // stores the right and downward facing connection paths that are blocked by collisions
} twixt_pp_node;

typedef struct twixt_pp_graph_s {
    uint16_t graph_id : 14; // joined graphs point to the true graph which connects this node
    bool connect_low : 1; // left / down
    bool connect_high : 1; // right / up
} twixt_pp_graph;

typedef struct twixt_pp_options_s {
    // this NOT only defines the playable area, so it includes the 1 wide borders on each side, max 128, min 3
    uint8_t wx;
    uint8_t wy;
    bool pie_swap; //TODO this is a move, swap colors / swap move ?
} twixt_pp_options;

typedef struct twixt_pp_internal_methods_s {

    error_code (*get_node)(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER* p);
    error_code (*set_node)(game* self, uint8_t x, uint8_t y, TWIXT_PP_PLAYER p, bool* wins);
    error_code (*get_node_connections)(game* self, uint8_t x, uint8_t y, uint8_t* conn); // right and downward facing existing connections

} twixt_pp_internal_methods;

extern const game_methods twixt_pp_gbe;

#ifdef __cplusplus
}
#endif
