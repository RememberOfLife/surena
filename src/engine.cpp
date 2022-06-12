#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>

#include "surena/engine.h"
#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

void eevent_create(engine_event* e, uint32_t engine_id, eevent_type type)
{
    *e = (engine_event){
        .type = type,
        .engine_id = engine_id,
    };
}

void eevent_create_log(engine_event* e, uint32_t engine_id, error_code ec, const char* text)
{
    *e = (engine_event){
        .type = EE_TYPE_LOG,
        .engine_id = engine_id,
        .log = (ee_log){
            .ec = ec,
            .text = text ? strdup(text) : NULL,
        },
    };
}

void eevent_create_load(engine_event* e, uint32_t engine_id, game* the_game)
{
    *e = (engine_event){
        .type = EE_TYPE_GAME_LOAD,
        .engine_id = engine_id,
        .load = (ee_game_load){
            .the_game = (game*)malloc(sizeof(game)),
        },
    };
    the_game->methods->clone(the_game, e->load.the_game);
}

void eevent_create_state(engine_event* e, uint32_t engine_id, const char* state)
{
    *e = (engine_event){
        .type = EE_TYPE_GAME_STATE,
        .engine_id = engine_id,
        .state = (ee_game_state){
            .str = state ? strdup(state) : NULL,
        },
    };
}

void eevent_create_move(engine_event* e, uint32_t engine_id, player_id player, move_code code)
{
    *e = (engine_event){
        .type = EE_TYPE_GAME_MOVE,
        .engine_id = engine_id,
        .move = (ee_game_move){
            .player = player,
            .code = code,
        },
    };
}

void eevent_create_sync(engine_event* e, uint32_t engine_id, void* data_start, void* data_end)
{
    size_t data_size = (char*)data_end - (char*)data_start;
    *e = (engine_event){
        .type = EE_TYPE_GAME_SYNC,
        .engine_id = engine_id,
        .sync = (ee_game_sync){
            .data_start = malloc(data_size),
        },
    };
    e->sync.data_end = (char*)e->sync.data_start + data_size;
    memcpy(e->sync.data_start, data_start, data_size);
}

void eevent_create_id(engine_event* e, uint32_t engine_id, const char* name, const char* author)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_ID,
        .engine_id = engine_id,
        .id = (ee_engine_id){
            .name = name ? strdup(name) : NULL,
            .author = author ? strdup(author) : NULL,
        },
    };
}

void eevent_create_option_check(engine_event* e, uint32_t engine_id, const char* name, bool check)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_OPTION,
        .engine_id = engine_id,
        .option = (ee_engine_option){
            .name = name ? strdup(name) : NULL,
            .type = EE_OPTION_TYPE_CHECK,
            .value = {.check = check},
        },
    };
}

void eevent_create_option_spin(engine_event* e, uint32_t engine_id, const char* name, uint64_t spin, uint64_t min, uint64_t max)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_OPTION,
        .engine_id = engine_id,
        .option = (ee_engine_option){
            .name = name ? strdup(name) : NULL,
            .type = EE_OPTION_TYPE_SPIN,
            .value = {
                .spin = spin,
            },
            .mm = {
                .min = min,
                .max = max,
            },
        },
    };
}

void eevent_create_option_combo(engine_event* e, uint32_t engine_id, const char* name, const char* combo, const char* var)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_OPTION,
        .engine_id = engine_id,
        .option = (ee_engine_option){
            .name = name ? strdup(name) : NULL,
            .type = EE_OPTION_TYPE_COMBO,
            .value = {
                .combo = combo ? strdup(combo) : NULL,
            },
            .v = {
                .var = var ? strdup(var) : NULL,
            },
        },
    };
}

void eevent_create_option_button(engine_event* e, uint32_t engine_id, const char* name)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_OPTION,
        .engine_id = engine_id,
        .option = (ee_engine_option){
            .name = name ? strdup(name) : NULL,
            .type = EE_OPTION_TYPE_BUTTON,
        },
    };
}

void eevent_create_option_string(engine_event* e, uint32_t engine_id, const char* name, const char* str)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_OPTION,
        .engine_id = engine_id,
        .option = (ee_engine_option){
            .name = name ? strdup(name) : NULL,
            .type = EE_OPTION_TYPE_STRING,
            .value = {
                .str = str ? strdup(str) : NULL,
            },
        },
    };
}

void eevent_create_option_spind(engine_event* e, uint32_t engine_id, const char* name, double spin, double min, double max)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_OPTION,
        .engine_id = engine_id,
        .option = (ee_engine_option){
            .name = name ? strdup(name) : NULL,
            .type = EE_OPTION_TYPE_SPIN,
            .value = {
                .spind = spin,
            },
            .mmd = {
                .min = min,
                .max = max,
            },
        },
    };
}

void eevent_create_start(engine_event* e, uint32_t engine_id, uint32_t timeout)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_START,
        .engine_id = engine_id,
        .start = (ee_engine_start){
            .timeout = timeout,
        },
    };
}

void eevent_create_searchinfo(engine_event* e, uint32_t engine_id)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_SEARCHINFO,
        .engine_id = engine_id,
        .searchinfo = (ee_engine_searchinfo){
            .flags = 0,
        },
    };
}

void eevent_set_searchinfo_time(engine_event* e, uint32_t time)
{
    e->searchinfo.flags |= EE_SEARCHINFO_FLAG_TYPE_TIME;
    e->searchinfo.time = time;
}

void eevent_set_searchinfo_depth(engine_event* e, uint32_t depth)
{
    e->searchinfo.flags |= EE_SEARCHINFO_FLAG_TYPE_DEPTH;
    e->searchinfo.depth = depth;
}

void eevent_set_searchinfo_score(engine_event* e, player_id player, float eval)
{
    e->searchinfo.flags |= EE_SEARCHINFO_FLAG_TYPE_SCORE;
    e->searchinfo.score_player = player;
    e->searchinfo.score_eval = eval;
}

void eevent_set_searchinfo_nodes(engine_event* e, uint64_t nodes)
{
    e->searchinfo.flags |= EE_SEARCHINFO_FLAG_TYPE_NODES;
    e->searchinfo.nodes = nodes;
}

void eevent_set_searchinfo_nps(engine_event* e, uint64_t nps)
{
    e->searchinfo.flags |= EE_SEARCHINFO_FLAG_TYPE_NPS;
    e->searchinfo.nps = nps;
}

void eevent_set_searchinfo_hashfull(engine_event* e, float hashfull)
{
    e->searchinfo.flags |= EE_SEARCHINFO_FLAG_TYPE_HASHFULL;
    e->searchinfo.hashfull = hashfull;
}

void eevent_set_searchinfo_pv(engine_event* e, player_id* pv_p, move_code* pv_m)
{
    e->searchinfo.flags |= EE_SEARCHINFO_FLAG_TYPE_PV;
    uint32_t cnt = 0;
    for (player_id* pv_p_cnt = pv_p; *pv_p_cnt != PLAYER_NONE; pv_p_cnt++) {
        cnt++;
    }
    e->searchinfo.pc_c = cnt;
    e->searchinfo.pv_p = (player_id*)malloc(sizeof(player_id) * cnt);
    e->searchinfo.pv_m = (move_code*)malloc(sizeof(move_code) * cnt);
    memcpy(e->searchinfo.pv_p, pv_p, sizeof(player_id) * cnt);
    memcpy(e->searchinfo.pv_m, pv_m, sizeof(move_code) * cnt);
}

void eevent_set_searchinfo_string(engine_event* e, const char* str)
{
    e->searchinfo.flags |= EE_SEARCHINFO_FLAG_TYPE_STRING;
    e->searchinfo.str = str ? strdup(str) : NULL;
}

void eevent_create_bestmove(engine_event* e, uint32_t engine_id, player_id player, move_code code)
{
    *e = (engine_event){
        .type = EE_TYPE_ENGINE_BESTMOVE,
        .engine_id = engine_id,
        .bestmove = (ee_engine_bestmove){
            .player = player,
            .code = code,
        },
    };
}

void eevent_destroy(engine_event* e)
{
    switch (e->type) {
        case EE_TYPE_LOG: {
            free(e->log.text);
        } break;
        case EE_TYPE_GAME_LOAD: {
            e->load.the_game->methods->destroy(e->load.the_game);
            free(e->load.the_game);
        } break;
        case EE_TYPE_GAME_STATE: {
            free(e->state.str);
        } break;
        case EE_TYPE_GAME_SYNC: {
            free(e->sync.data_start);
        } break;
        case EE_TYPE_ENGINE_ID: {
            free(e->id.name);
            free(e->id.author);
        } break;
        case EE_TYPE_ENGINE_OPTION: {
            free(e->option.name);
            if (e->option.type == EE_OPTION_TYPE_COMBO) {
                free(e->option.value.combo);
                free(e->option.v.var);
            }
            if (e->option.type == EE_OPTION_TYPE_STRING) {
                free(e->option.value.str);
            }
        } break;
        case EE_TYPE_ENGINE_SEARCHINFO: {
            if (e->searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_PV) {
                free(e->searchinfo.pv_p);
                free(e->searchinfo.pv_m);
            }
            if (e->searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_STRING) {
                free(e->searchinfo.str);
            }
        } break;
    }
    e->type = EE_TYPE_NULL;
}

struct eevent_queue_impl {
    std::mutex m;
    std::deque<engine_event> q;
    std::condition_variable cv;
};

static_assert(sizeof(eevent_queue) == sizeof(eevent_queue_impl), "eevent_queue impl size missmatch");

void eevent_queue_create(eevent_queue* eq)
{
    eevent_queue_impl* eqi = (eevent_queue_impl*)eq;
    new(eqi) eevent_queue();
}

void eevent_queue_destroy(eevent_queue* eq)
{
    eevent_queue_impl* eqi = (eevent_queue_impl*)eq;
    delete eqi;
}

void eevent_queue_push(eevent_queue* eq, engine_event* e)
{
    eevent_queue_impl* eqi = (eevent_queue_impl*)eq;
    eqi->m.lock();
    eqi->q.push_back(*e);
    eqi->cv.notify_all();
    eqi->m.unlock();
    e->type = EE_TYPE_NULL;
}

void eevent_queue_pop(eevent_queue* eq, engine_event* e, uint32_t t)
{
    eevent_queue_impl* eqi = (eevent_queue_impl*)eq;
    std::unique_lock<std::mutex> lock(eqi->m);
    if (eqi->q.size() == 0) {
        if (t > 0) {
            eqi->cv.wait_for(lock, std::chrono::milliseconds(t));
        }
        if (eqi->q.size() == 0) {
            // queue has no available events after timeout, return null event
            e->type = EE_TYPE_NULL;
            return;
        }
        // go on to output an available event if one has become available
    }
    *e = eqi->q.front();
    eqi->q.pop_front();
}

#ifdef __cplusplus
}
#endif
