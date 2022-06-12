#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "surena/util/semver.h"
#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t SURENA_ENGINE_API_VERSION = 2;



typedef uint32_t eevent_type;
// engine events
enum EE_TYPE {
    // special
    EE_TYPE_NULL = 0,
    EE_TYPE_EXIT,
    EE_TYPE_LOG, // engine outbound, also serves errors
    EE_TYPE_HEARTBEAT, // in/out keepalive check, can ALWAYS be issued and should be answered asap (isready/readyok)
    // game: just using these the engine should be able to wrap the supported games
    EE_TYPE_GAME_LOAD,
    EE_TYPE_GAME_UNLOAD, //TODO maybe remove this in favor of load with a null game
    EE_TYPE_GAME_STATE,
    EE_TYPE_GAME_MOVE,
    EE_TYPE_GAME_SYNC,
    // engine:
    EE_TYPE_ENGINE_ID,
    EE_TYPE_ENGINE_OPTION,
    EE_TYPE_ENGINE_START,
    EE_TYPE_ENGINE_STOP,
    EE_TYPE_ENGINE_SEARCHINFO,
    EE_TYPE_ENGINE_BESTMOVE,

    // EE_TYPE_INTERNAL,

    EE_TYPE_COUNT,
};

typedef struct ee_log_s {
    error_code ec;
    char* text;
} ee_log;

typedef struct ee_game_load_s {
    game* the_game;
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

typedef struct ee_engine_id_s {
    char* name;
    char* author;
} ee_engine_id;

typedef uint8_t eevent_option_type;

enum EE_OPTION_TYPE {
    EE_OPTION_TYPE_NONE = 0,

    EE_OPTION_TYPE_CHECK,
    EE_OPTION_TYPE_SPIN,
    EE_OPTION_TYPE_COMBO,
    EE_OPTION_TYPE_BUTTON,
    EE_OPTION_TYPE_STRING,
    EE_OPTION_TYPE_SPIND, // float64 spinner

    EE_OPTION_TYPE_COUNT,
};

typedef struct ee_engine_option_s {
    char* name;
    eevent_option_type type;
    union {
        bool check;
        uint64_t spin;
        char* combo;
        char* str;
        double spind;
    } value; // this is the default value when sent by the engine, and the new value to use when sent by the gui
    union {
        struct {
            uint64_t min;
            uint64_t max;
        } mm;
        struct {
            double min;
            double max;
        } mmd;
        struct {
            char* var; // separated by newlines
        } v;
    }; // min,max / var, only engine outbound
} ee_engine_option;

typedef struct ee_engine_start_s {
    uint32_t timeout; // in ms
    //TODO more constraints (e.g. moves), timectl, force win etc
} ee_engine_start;

typedef uint8_t ee_searchinfo_flags;

enum EE_SEARCHINFO_FLAG_TYPE {
    EE_SEARCHINFO_FLAG_TYPE_TIME = 1<<0,
    EE_SEARCHINFO_FLAG_TYPE_DEPTH = 1<<1,
    EE_SEARCHINFO_FLAG_TYPE_SCORE = 1<<2, // uses float as described by the game api instead of uci centipawns
    EE_SEARCHINFO_FLAG_TYPE_NODES = 1<<3,
    EE_SEARCHINFO_FLAG_TYPE_NPS = 1<<4,
    EE_SEARCHINFO_FLAG_TYPE_HASHFULL = 1<<5, // uses just a float [0,1] instead of uci permill
    EE_SEARCHINFO_FLAG_TYPE_PV = 1<<6,
    EE_SEARCHINFO_FLAG_TYPE_STRING = 1<<7,
};

typedef struct ee_engine_searchinfo_s {
    ee_searchinfo_flags flags;
    uint32_t time;
    uint32_t depth;
    player_id score_player;
    float score_eval;
    float hashfull;
    uint64_t nodes;
    uint64_t nps;
    uint32_t pc_c; // count of pv nodes
    player_id* pv_p;
    move_code* pv_m;
    char* str;
} ee_engine_searchinfo;

typedef struct ee_engine_bestmove_s {
    player_id player;
    move_code code;
} ee_engine_bestmove;

typedef struct engine_event_s {
    eevent_type type;
    uint32_t engine_id;
    union {
        ee_log log;
        ee_game_load load;
        ee_game_state state;
        ee_game_move move;
        ee_game_sync sync;
        ee_engine_id id;
        ee_engine_option option;
        ee_engine_start start;
        ee_engine_searchinfo searchinfo;
        ee_engine_bestmove bestmove;
    };
} engine_event;

//TODO internal event struct?

// eevent_create_* and eevent_set_* methods COPY/CLONE strings/binary/games into the event

//TODO should create call destroy if we're creating on top of an existing event? would need eevent_init then to set to NULL

void eevent_create(engine_event* e, uint32_t engine_id, eevent_type type);

void eevent_create_log(engine_event* e, uint32_t engine_id, error_code ec, const char* text);

void eevent_create_load(engine_event* e, uint32_t engine_id, game* the_game);

void eevent_create_state(engine_event* e, uint32_t engine_id, const char* state);

void eevent_create_move(engine_event* e, uint32_t engine_id, player_id player, move_code code);

void eevent_create_sync(engine_event* e, uint32_t engine_id, void* data_start, void* data_end);

void eevent_create_id(engine_event* e, uint32_t engine_id, const char* name, const char* author);

void eevent_create_option_check(engine_event* e, uint32_t engine_id, const char* name, bool check);

void eevent_create_option_spin(engine_event* e, uint32_t engine_id, const char* name, uint64_t spin, uint64_t min, uint64_t max);

void eevent_create_option_combo(engine_event* e, uint32_t engine_id, const char* name, const char* combo, const char* var);

void eevent_create_option_button(engine_event* e, uint32_t engine_id, const char* name);

void eevent_create_option_string(engine_event* e, uint32_t engine_id, const char* name, const char* str);

void eevent_create_option_spind(engine_event* e, uint32_t engine_id, const char* name, double spin, double min, double max);

void eevent_create_start(engine_event* e, uint32_t engine_id, uint32_t timeout);

void eevent_create_searchinfo(engine_event* e, uint32_t engine_id);

void eevent_set_searchinfo_time(engine_event* e, uint32_t time);

void eevent_set_searchinfo_depth(engine_event* e, uint32_t depth);

void eevent_set_searchinfo_score(engine_event* e, player_id player, float eval);

void eevent_set_searchinfo_nodes(engine_event* e, uint64_t nodes);

void eevent_set_searchinfo_nps(engine_event* e, uint64_t nps);

void eevent_set_searchinfo_hashfull(engine_event* e, float hashfull);

void eevent_set_searchinfo_pv(engine_event* e, player_id* pv_p, move_code* pv_m);

void eevent_set_searchinfo_string(engine_event* e, const char* str);

void eevent_create_bestmove(engine_event* e, uint32_t engine_id, player_id player, move_code code);

void eevent_destroy(engine_event* e);



typedef struct eevent_queue_s {
    char _padding[168];
} eevent_queue;

// the queue takes ownership of everything in the event and resets it to type NULL

void eevent_queue_create(eevent_queue* eq);

void eevent_queue_destroy(eevent_queue* eq);

void eevent_queue_push(eevent_queue* eq, engine_event* e);

void eevent_queue_pop(eevent_queue* eq, engine_event* e, uint32_t t);



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

    error_code (*create_with_opts_str)(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox, const char* str);

    error_code (*create_with_opts_bin)(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox, void* options_struct);

    // sets outbox and inbox to the corresponding queues owned by the engine
    // use inbox to send things to the engine, check outbox for eevents from the engine
    error_code (*create_default)(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox);

    error_code (*destroy)(engine* self);

    error_code (*is_game_compatible)(engine* self, game* compat_game);

} engine_methods;

struct engine_s {
    const engine_methods* methods;
    uint32_t engine_id;
    void* data1;
    void* data2;
};

#ifdef __cplusplus
}
#endif
