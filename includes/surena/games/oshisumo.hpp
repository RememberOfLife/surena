#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "surena/game.h"

const uint8_t OSHISUMO_NONE = UINT8_MAX;
const uint8_t OSHISUMO_ANY = UINT8_MAX - 1;

typedef struct oshisumo_options_s {
    uint8_t size; // [2, 120] //TODO find proper max
    uint8_t tokens; // [size, 253]
} oshisumo_options;

typedef struct oshisumo_internal_methods_s {

    error_code (*get_tokens)(game* self, player_id p, uint8_t* t);
    error_code (*set_tokens)(game* self, player_id p, uint8_t t);
    error_code (*get_cell)(game* self, int8_t* c);
    error_code (*set_cell)(game* self, int8_t c);
    error_code (*get_sm_tokens)(game* self, player_id p, uint8_t* t);
    //TODO need set_sm_tokens ?

} oshisumo_internal_methods;

extern const game_methods oshisumo_gbe;

#ifdef __cplusplus
}
#endif