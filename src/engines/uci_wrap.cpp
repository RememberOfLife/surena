#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "surena/util/noise.h"
#include "surena/engine.h"
#include "surena/game.h"

#include "surena/engines/uci_wrap.h"

namespace {

    //TODO launch uci engine process with
    // https://stackoverflow.com/questions/9405985/linux-executing-child-process-with-piped-stdin-stdout
    // https://github.com/arun11299/cpp-subprocess
    // https://gitlab.com/eidheim/tiny-process-library

    struct data_repr {
        eevent_queue* outbox;
        eevent_queue inbox;
        std::thread* loop_runner; // engine event loop, writes to the uci process
        std::thread* exec_runner; // uci process listener, reads from the uci process and issues output to outbox
        game the_game;
        bool searching;
    };

    data_repr& get_repr(engine* self)
    {
        return *((data_repr*)(self->data1));
    }

    // forward declares

    // engine wrapper
    error_code create_with_opts_str(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox, const char* str);
    error_code create_with_opts_bin(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox, void* options_struct);
    error_code create_default(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox);
    error_code destroy(engine* self);
    error_code is_game_compatible(engine* self, game* compat_game);
    // engine loop
    void engine_loop(data_repr* data_p, uint32_t engine_id);
    void exec_loop(data_repr* data_p, uint32_t engine_id);

    //TODO process forward b/c there is no peek on the stdout of the uci engine

    // implementation

    error_code create_with_opts_str(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox, const char* str)
    {
        //TODO
        return ERR_OK;
    }

    error_code create_with_opts_bin(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox, void* options_struct)
    {
        //TODO
        return ERR_OK;
    }

    error_code create_default(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox)
    {
        self->data1 = malloc(sizeof(data_repr));
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        self->engine_id = engine_id;
        data_repr& data = get_repr(self);
        data.outbox = outbox;
        eevent_queue_create(&data.inbox);
        *inbox = &data.inbox;
        data.loop_runner = new std::thread(engine_loop, (data_repr*)self->data1, engine_id);
        data.the_game.methods = NULL;
        data.searching = false;
        return ERR_OK;
    }

    error_code destroy(engine* self)
    {
        data_repr& data = get_repr(self);
        engine_event e_quit = (engine_event){
            .type = EE_TYPE_EXIT};
        eevent_queue_push(&data.inbox, &e_quit);
        data.loop_runner->join();
        eevent_queue_destroy(&data.inbox);
        delete data.loop_runner;
        free(self->data1);
        return ERR_OK;
    }

    error_code is_game_compatible(engine* self, game* compat_game)
    {
        if (strcmp(compat_game->methods->game_name, "Chess") == 0 && strcmp(compat_game->methods->variant_name, "Standard") == 0) {
            return ERR_OK;
        }
        return ERR_INVALID_INPUT;
    }

    // engine loop

    void engine_loop(data_repr* data_p, uint32_t engine_id)
    {
        data_repr& data = *data_p;
        engine_event e;
        engine_event e2;

        // send id and options
        //TODO from uci engine

        bool quit = false;
        while (!quit) {
            eevent_queue_pop(&data.inbox, &e, UINT32_MAX);
            if (e.engine_id != engine_id) {
                eevent_create_log(&e, engine_id, ERR_INVALID_INPUT, "engine id mismatch");
                eevent_queue_push(data.outbox, &e);
                eevent_destroy(&e);
                continue;
            }
            bool fallthrough = false;
            switch (e.type) {
                case EE_TYPE_NULL: {
                    // pass
                } break;
                case EE_TYPE_EXIT: {
                    quit = true;
                } break;
                case EE_TYPE_LOG: {
                    assert(0);
                } break;
                case EE_TYPE_HEARTBEAT: {
                    eevent_create_heartbeat(&e, engine_id, e.heartbeat.id);
                    eevent_queue_push(data.outbox, &e);
                } break;
                case EE_TYPE_GAME_LOAD: {
                    //TODO
                } break;
                case EE_TYPE_GAME_UNLOAD: {
                    //TODO
                } break;
                case EE_TYPE_GAME_STATE: {
                    //TODO
                } break;
                case EE_TYPE_GAME_MOVE: {
                    //TODO
                } break;
                case EE_TYPE_GAME_SYNC: {
                    assert(0);
                } break;
                case EE_TYPE_GAME_DRAW: {
                    assert(0);
                } break;
                case EE_TYPE_GAME_RESIGN: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_ID: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_OPTION: {
                    //TODO
                } break;
                case EE_TYPE_ENGINE_START: {
                    //TODO
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
                    //TODO
                } break;
                case EE_TYPE_ENGINE_BESTMOVE: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_MOVESCORE: {
                    assert(0);
                } break;
            }
            eevent_destroy(&e);
        }
        eevent_create(&e, engine_id, EE_TYPE_EXIT);
        eevent_queue_push(data.outbox, &e);
        eevent_destroy(&e);
    }

    void exec_loop(data_repr* data_p, uint32_t engine_id)
    {
    }

} // namespace

const engine_methods uci_wrap_ebe{

    .engine_name = "uci_wrap",
    .version = semver{0, 2, 0},
    .features = engine_feature_flags{
        .error_strings = false,
        .options = true,
        .options_bin = true,
        .score_all_moves = false,
        .running_bestmove = false,
        .draw_and_resign = false,
    },
    .internal_methods = NULL,

    .get_last_error = NULL,
    .create_with_opts_str = create_with_opts_str,
    .create_with_opts_bin = create_with_opts_bin,
    .create_default = create_default,
    .destroy = destroy,
    .is_game_compatible = is_game_compatible,

};
