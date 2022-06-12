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
        std::thread runner;
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
        data.runner = std::thread(_engine_loop, self);
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
        data.runner.join();
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
        eevent_create_option_spin(&e, self->engine_id, "rng seed", 42, 0, UINT64_MAX);
        eevent_queue_push(data.outbox, &e);
        eevent_destroy(&e);

        bool quit = false;
        while (!quit) {
            eevent_queue_pop(&data.inbox, &e, 1000);
            e.engine_id = self->engine_id; //TODO probably shouldnt do this on every event
            switch (e.type) {
                case EE_TYPE_NULL: {
                    if (data.searching) {
                        //TODO should send this periodically and not just if nothing happened for 1s
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
                    eevent_create(&e, self->engine_id, EE_TYPE_HEARTBEAT);
                    eevent_queue_push(data.outbox, &e);
                } break;
                case EE_TYPE_GAME_LOAD: {
                    e.load.the_game->methods->clone(e.load.the_game, &data.the_game);
                    data.rng_counter = 0;
                    data.searching = false;
                } break;
                case EE_TYPE_GAME_UNLOAD: {
                    data.the_game.methods->destroy(&data.the_game);
                    data.the_game.methods = NULL;
                    data.searching = false;
                } break;
                case EE_TYPE_GAME_STATE: {
                    data.the_game.methods->import_state(&data.the_game, e.state.str);
                    data.rng_counter = 0;
                    data.searching = false;
                } break;
                case EE_TYPE_GAME_MOVE: {
                    data.the_game.methods->make_move(&data.the_game, e.move.player, e.move.code);
                    data.rng_counter++;
                } break;
                case EE_TYPE_GAME_SYNC: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_ID: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_OPTION: {
                    if (strcmp(e.option.name, "rng seed") == 0) {
                        data.rng_seed = e.option.value.spin;
                    }
                } break;
                case EE_TYPE_ENGINE_START: {
                    data.searching = true;
                    eevent_create_searchinfo(&e, self->engine_id);
                    eevent_set_searchinfo_string(&e, "search started");
                    eevent_queue_push(data.outbox, &e);
                } break;
                case EE_TYPE_ENGINE_STOP: {
                    data.searching = false;
                    // issue final searchinfo and bestmove
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
                    
                    eevent_create_bestmove(&e, self->engine_id, ptm[0], moves[squirrelnoise5(data.rng_counter, data.rng_seed) % moves_c]);
                    eevent_queue_push(data.outbox, &e);
                    free(ptm);
                    free(moves);
                } break;
                case EE_TYPE_ENGINE_SEARCHINFO: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_BESTMOVE: {
                    assert(0);
                } break;
                default: {
                    assert(0);
                } break;
            }
            eevent_destroy(&e);
        }
    }

}

const engine_methods randomengine_ebe{

    .engine_name = "Random",
    .version = semver{1, 0, 0},
    .features = engine_feature_flags{
        .options = false,
        .options_bin = false,
    },
    .internal_methods = NULL,
    
    .get_last_error = surena::_get_last_error,
    .create_with_opts_str = NULL,
    .create_with_opts_bin = NULL,
    .create_default = surena::_create_default,
    .destroy = surena::_destroy,
    .is_game_compatible = surena::_is_game_compatible,
    
};
