#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "surena/util/semver.h"
#include "surena/game.h"

static const uint64_t SURENA_ENGINE_API_VERSION = 1;



typedef uint32_t eevent_type;
// engine events
enum EE_TYPE {
    // special
    EE_TYPE_NULL = 0,
    EE_TYPE_EXIT,
    EE_TYPE_LOG, // engine outbound, also serves errors
    EE_TYPE_HEARTBEAT, // in/out keepalive check, can ALWAYS be issued and should be answered asap#
    EE_TYPE_ID,
    // game: just using these the engine should be able to wrap the supported games
    EE_TYPE_GAME_LOAD,
    EE_TYPE_GAME_UNLOAD,
    EE_TYPE_GAME_STATE,
    EE_TYPE_GAME_MOVE,
    EE_TYPE_GAME_SYNC,
    // engine:
    EE_TYPE_ENGINE_OPTION,
    EE_TYPE_ENGINE_START,
    EE_TYPE_ENGINE_STOP,
    EE_TYPE_ENGINE_SEARCHINFO,
    EE_TYPE_ENGINE_BESTMOVE, //TODO

    EE_TYPE_INTERNAL,

    EE_TYPE_COUNT,
};

typedef struct ee_log_s {
    error_code ec;
    char* text;
} ee_log;

typedef struct ee_id_s {
    char* name;
    char* author;
} ee_id;

typedef struct ee_game_load_s {
    game* game;
} ee_game_load;

typedef struct ee_game_state_s {
    char* str;
} ee_game_state;

typedef struct ee_game_move_s {
    player_id player;
    move_code code;
} ee_game_move;

typedef struct ee_game_sync_s {
    void* data_start;
    void* data_end;
} ee_game_sync;

typedef uint8_t eevent_option_type;

enum EE_OPTION_TYPE {
    EE_OPTION_TYPE_NONE = 0,

    EE_OPTION_TYPE_CHECK,
    EE_OPTION_TYPE_SPIN,
    EE_OPTION_TYPE_COMBO,
    EE_OPTION_TYPE_BUTTON,
    EE_OPTION_TYPE_STRING,

    EE_OPTION_TYPE_COUNT,
};

typedef struct ee_engine_option_s {
    char* name;
    eevent_option_type type;
    union {
        bool check;
        uint32_t spin;
        char* combo;
        bool button;
        char* str;
    } value; // this is the default value when sent by the engine, and the new value to use when sent by the gui
    union {
        struct {
            uint32_t min;
            uint32_t max;
        } mm;
        struct {
            uint32_t cnt;
            char** var;
        } v;
    }; // min,max / var, only engine outbound
} ee_engine_option;

typedef struct ee_engine_start_s {
    uint32_t timeout; // in ms
} ee_engine_start;

typedef struct engine_event_s {
    eevent_type type;
    union {
        ee_log log;
        ee_game_load load;
        ee_game_state state;
        ee_game_move move;
        ee_game_sync sync;
        ee_engine_option option;
        ee_engine_start start;
    };
} engine_event;

// see https://backscattering.de/chess/uci/ for some more general considerations so this will all be easily compatible with uci and tei

//TODO create methods for complex event types

void eevent_destroy(engine_event* e);



typedef struct eevent_queue_s eevent_queue; //TODO try to do this in cpp for easy deque and mutex

void eevent_queue_create(eevent_queue* q);

void eevent_queue_destroy(eevent_queue* q);

void eevent_queue_push(eevent_queue* q, engine_event* e);

void eevent_queue_pop(eevent_queue* q, engine_event* e, uint32_t t);



typedef struct engine_feature_flags_s {
    bool options : 1;
    bool options_bin : 1;
} engine_feature_flags;

typedef struct engine_s engine;

typedef struct engine_methods_s {

    // minimum length of 1 character, with allowed character set:
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" additionally '-' and '_' but not at the start or end
    const char* engine_name;
    const semver version;
    const engine_feature_flags features;

    const void* internal_methods;

    const char* (*get_last_error)(engine* self);

    error_code (*create_with_opts_str)(engine* self, eevent_queue* outbox, eevent_queue* inbox, const char* str);

    error_code (*create_with_opts_bin)(engine* self, eevent_queue* outbox, eevent_queue* inbox, void* options_struct);

    // sets outbox and inbox to the corresponding queues owned by the engine
    // use inbox to send things to the engine, check outbox for eevents from the engine
    error_code (*create_default)(engine* self, eevent_queue* outbox, eevent_queue* inbox);

    error_code (*destroy)(engine* self);

    error_code (*is_game_compatible)(engine* self, game* compat_game);

} engine_methods;

struct engine_s {
    const engine_methods* methods;
    void* data;
    //TODO what goes here?
};

#ifdef __cplusplus
}
#endif
