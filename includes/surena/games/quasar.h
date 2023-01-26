#pragma once

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

static const move_code QUASAR_MOVE_BIG = 1 << 8;
static const move_code QUASAR_MOVE_PAYOUT = 1 << 9;
static const move_code QUASAR_MOVE_SMALL = 1 << 10;

typedef struct quasar_internal_methods_s {

    error_code (*get_num)(game* self, uint8_t* n);
    error_code (*get_score)(game* self, int* s);
    error_code (*can_big)(game* self, bool* r);
    error_code (*can_payout)(game* self, bool* r);
    error_code (*can_small)(game* self, bool* r);

} quasar_internal_methods;

extern const game_methods quasar_standard_gbe;

#ifdef __cplusplus
}
#endif
