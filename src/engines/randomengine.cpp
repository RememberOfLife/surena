#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "surena/util/noise.hpp"
#include "surena/engine.h"
#include "surena/game.h"

#include "surena/engines/randomengine.h"

namespace surena {

    static error_code _return_errorf(engine* self, error_code ec, const char* fmt, ...)
    {
        if (self->data2 == NULL) {
            self->data2 = malloc(1024); //TODO correct size from where?
        }
        va_list args;
        va_start(args, fmt);
        vsprintf((char*)self->data2, fmt, args);
        va_end(args);
        return ec;
    }

    typedef struct data_repr {
        eevent_queue* outbox;
        eevent_queue inbox;
        std::thread* runner; // somehow does not work with a non pointer?
        game the_game;
        uint32_t rng_seed;
        uint32_t rng_counter;
        bool searching;
    } data_repr;

    static data_repr& _get_repr(engine* self)
    {
        return *((data_repr*)(self->data1));
    }

    // forward declares

    // engine wrapper
    const char* _get_last_error(engine* self);
    error_code _create_default(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox);
    error_code _destroy(engine* self);
    error_code _is_game_compatible(engine* self, game* compat_game);
    // engine loop2222
    void _engine_loop(engine* self);

    // implementation

    const char* _get_last_error(engine* self)
    {
        return (char*)self->data2;
    }
    
    error_code _create_default(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        self->engine_id = engine_id;
        data_repr& data = _get_repr(self);
        data.outbox = outbox;
        eevent_queue_create(&data.inbox);
        *inbox = &data.inbox;
        data.runner = new std::thread(_engine_loop, self);
        data.the_game.methods = NULL;
        data.rng_seed = 42;
        data.rng_counter = 0;
        data.searching = false;
        return ERR_OK;
    }
    
    error_code _destroy(engine* self)
    {
        data_repr& data = _get_repr(self);
        engine_event e_quit = (engine_event){
            .type = EE_TYPE_EXIT
        };
        eevent_queue_push(&data.inbox, &e_quit);
        data.runner->join();
        eevent_queue_destroy(&data.inbox);
        delete data.runner;
        free(self->data1);
        free(self->data2);
        return ERR_OK;
    }
    
    error_code _is_game_compatible(engine* self, game* compat_game)
    {
        if (compat_game->methods->features.random_moves | compat_game->methods->features.hidden_information | compat_game->methods->features.simultaneous_moves) {
            return ERR_INVALID_INPUT;
        }
        return ERR_OK;
    }
    
    // engine loop

    void _engine_loop(engine* self)
    {
        data_repr& data = _get_repr(self);
        engine_event e;

        // send id and options
        eevent_create_id(&e, self->engine_id, "Random", "surena_default");
        eevent_queue_push(data.outbox, &e);
        eevent_destroy(&e);
        eevent_create_option_u64_mm(&e, self->engine_id, "rng seed", 42, 0, UINT64_MAX);
        eevent_queue_push(data.outbox, &e);
        eevent_destroy(&e);

        bool quit = false;
        while (!quit) {
            eevent_queue_pop(&data.inbox, &e, 1000);
            if (e.engine_id != self->engine_id) {
                eevent_create_log(&e, self->engine_id, ERR_INVALID_INPUT, "engine id mismatch");
                eevent_queue_push(data.outbox, &e);
                eevent_destroy(&e);
                continue;
            }
            switch (e.type) {
                case EE_TYPE_NULL: {
                    if (data.searching) {
                        //TODO properly send this periodically
                        //TODO offer some get tick time in the engine api
                        eevent_create_searchinfo(&e, self->engine_id);
                        eevent_set_searchinfo_nps(&e, 1);
                        eevent_queue_push(data.outbox, &e);
                    }
                } break;
                case EE_TYPE_EXIT: {
                    quit = true;
                } break;
                case EE_TYPE_LOG: {
                    assert(0);
                } break;
                case EE_TYPE_HEARTBEAT: {
                    eevent_create_heartbeat(&e, self->engine_id, e.heartbeat.id);
                    eevent_queue_push(data.outbox, &e);
                } break;
                case EE_TYPE_GAME_LOAD: {
                    data.rng_counter = 0;
                    data.searching = false;
                    if (data.the_game.methods != NULL) {
                        data.the_game.methods->destroy(&data.the_game);
                    }
                    e.load.the_game->methods->clone(e.load.the_game, &data.the_game);
                } break;
                case EE_TYPE_GAME_UNLOAD: {
                    if (data.the_game.methods == NULL) {
                        break;
                    }
                    data.the_game.methods->destroy(&data.the_game);
                    data.the_game.methods = NULL;
                    data.searching = false;
                } break;
                case EE_TYPE_GAME_STATE: {
                    if (data.the_game.methods == NULL) {
                        eevent_create_log(&e, self->engine_id, ERR_INVALID_INPUT, "missing game");
                        eevent_queue_push(data.outbox, &e);
                        break;
                    }
                    data.the_game.methods->import_state(&data.the_game, e.state.str);
                    data.rng_counter = 0;
                    data.searching = false;
                } break;
                case EE_TYPE_GAME_MOVE: {
                    //TODO move and search on games that have not been set to at least a default board state should error, how to even test this?
                    if (data.the_game.methods == NULL) {
                        eevent_create_log(&e, self->engine_id, ERR_INVALID_INPUT, "missing game");
                        eevent_queue_push(data.outbox, &e);
                        break;
                    }
                    if (ERR_OK != data.the_game.methods->is_legal_move(&data.the_game, e.move.player, e.move.move, e.move.sync)) {
                        eevent_create_log(&e, self->engine_id, ERR_INVALID_INPUT, "illegal move");
                        eevent_queue_push(data.outbox, &e);
                        break;
                    }
                    data.the_game.methods->make_move(&data.the_game, e.move.player, e.move.move);
                    data.rng_counter++;
                    uint8_t ptm_c;
                    player_id* ptm = (player_id*)malloc(sizeof(player_id) * data.the_game.sizer.max_players_to_move);
                    data.the_game.methods->players_to_move(&data.the_game, &ptm_c, ptm);
                    if (ptm_c == 0) {
                        //TODO send bestmove move_none immediately, what other search stop infos too?
                        eevent_create_log(&e, self->engine_id, ERR_OK, "TODO. bestmove none");
                        eevent_queue_push(data.outbox, &e);
                        break;
                    }
                } break;
                case EE_TYPE_GAME_SYNC: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_ID: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_OPTION: {
                    if (strcmp(e.option.name, "rng seed") == 0) {
                        data.rng_seed = e.option.value.u64;
                    }
                } break;
                case EE_TYPE_ENGINE_START: {
                    if (data.the_game.methods == NULL) {
                        eevent_create_log(&e, self->engine_id, ERR_INVALID_INPUT, "missing game");
                        eevent_queue_push(data.outbox, &e);
                        break;
                    }
                    uint8_t ptm_c;
                    player_id* ptm = (player_id*)malloc(sizeof(player_id) * data.the_game.sizer.max_players_to_move);
                    data.the_game.methods->players_to_move(&data.the_game, &ptm_c, ptm);
                    if (ptm_c == 0) {
                        //TODO send bestmove move_none immediately, what other search stop infos too?
                        eevent_create_log(&e, self->engine_id, ERR_OK, "TODO. bestmove none");
                        eevent_queue_push(data.outbox, &e);
                        break;
                    }
                    data.searching = true;
                    eevent_create_start_empty(&e, self->engine_id);
                    eevent_queue_push(data.outbox, &e);
                    //TODO respect timeout as set by the gui
                } break;
                case EE_TYPE_ENGINE_SEARCHINFO: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_SCOREINFO: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_LINEINFO: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_STOP: {
                    if (data.searching == false) {
                        eevent_create_log(&e, self->engine_id, ERR_INVALID_INPUT, "no search running");
                        eevent_queue_push(data.outbox, &e);
                        break;
                    }
                    eevent_create_stop_empty(&e, self->engine_id);
                    eevent_queue_push(data.outbox, &e);
                    eevent_destroy(&e);
                } /* fallthrough */;
                case EE_TYPE_ENGINE_BESTMOVE: {
                    if (data.searching == false) {
                        eevent_create_log(&e, self->engine_id, ERR_INVALID_INPUT, "no search running");
                        eevent_queue_push(data.outbox, &e);
                        break;
                    }
                    data.searching = false;
                    // issue final information
                    uint8_t ptm_c;
                    player_id* ptm = (player_id*)malloc(sizeof(player_id) * data.the_game.sizer.max_players_to_move);
                    data.the_game.methods->players_to_move(&data.the_game, &ptm_c, ptm);
                    if (ptm_c == 0) {
                        eevent_create_log(&e, self->engine_id, ERR_INVALID_INPUT, "no bestmove available on finished game");
                        eevent_queue_push(data.outbox, &e);
                        free(ptm);
                        break;
                    }
                    uint32_t moves_c;
                    move_code* moves = (move_code*)malloc(sizeof(move_code) * data.the_game.sizer.max_moves);
                    data.the_game.methods->get_concrete_moves(&data.the_game, ptm[0], &moves_c, moves);

                    eevent_create_searchinfo(&e, self->engine_id);
                    eevent_set_searchinfo_depth(&e, ptm_c);
                    eevent_set_searchinfo_nodes(&e, moves_c);
                    eevent_queue_push(data.outbox, &e);

                    eevent_create_scoreinfo(&e, self->engine_id, 1);
                    e.scoreinfo.score_player[0] = ptm[0];
                    e.scoreinfo.score_eval[0] = 0.5;
                    e.scoreinfo.forced_end[0] = EE_SCOREINFO_FORCED_UNKNOWN;
                    eevent_queue_push(data.outbox, &e);
                    eevent_destroy(&e);
                    
                    eevent_create_bestmove(&e, self->engine_id, 1);
                    e.bestmove.player[0] = ptm[0];
                    e.bestmove.move[0] = moves[squirrelnoise5(data.rng_counter, data.rng_seed) % moves_c];
                    e.bestmove.confidence[0] = 0.95;
                    eevent_queue_push(data.outbox, &e);
                    free(ptm);
                    free(moves);
                } break;
                case EE_TYPE_ENGINE_MOVESCORE: {
                    assert(0);
                } break;
                default: {
                    assert(0);
                } break;
            }
            eevent_destroy(&e);
        }
        eevent_create(&e, self->engine_id, EE_TYPE_EXIT);
        eevent_queue_push(data.outbox, &e);
        eevent_destroy(&e);
    }

}

const engine_methods randomengine_ebe{

    .engine_name = "Random",
    .version = semver{1, 0, 0},
    .features = engine_feature_flags{
        .options = false,
        .options_bin = false,
        .score_all_moves = false,
    },
    .internal_methods = NULL,
    
    .get_last_error = surena::_get_last_error,
    .create_with_opts_str = NULL,
    .create_with_opts_bin = NULL,
    .create_default = surena::_create_default,
    .destroy = surena::_destroy,
    .is_game_compatible = surena::_is_game_compatible,
    
};
