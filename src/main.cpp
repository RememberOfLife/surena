#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>

#include "surena/games/chess.h"
#include "surena/games/havannah.h"
#include "surena/games/oshisumo.h"
#include "surena/games/tictactoe_ultimate.h"
#include "surena/games/tictactoe.h"
#include "surena/games/twixt_pp.h"
#include "surena/util/semver.h"
#include "surena/game_plugin.h"
#include "surena/game.h"
#include "generated/git_commit_hash.h"

namespace surena {
    const semver version = {0, 10, 0};
} // namespace surena

// args https://github.com/p-ranav/argparse

//TODO move this elsewhere
const game_methods* load_plugin_first_method(const char* file)
{
    void* dll_handle = dlopen(file, RTLD_LAZY);
    plugin_get_game_capi_version_t version = (plugin_get_game_capi_version_t)dlsym(dll_handle, "plugin_get_game_capi_version");
    if (version == NULL || version() != SURENA_GAME_API_VERSION) {
        return NULL;
    }
    plugin_get_game_methods_t get_games = (plugin_get_game_methods_t)dlsym(dll_handle, "plugin_get_game_methods");
    if (get_games == NULL) {
        return NULL;
    }
    uint32_t method_cnt;
    get_games(&method_cnt, NULL);
    if (method_cnt == 0) {
        return NULL;
    }
    const game_methods** method_buf = (const game_methods**)malloc(sizeof(game_methods*) * method_cnt);
    get_games(&method_cnt, method_buf);
    if (method_cnt == 0) {
        return NULL;
    }
    const game_methods* ret_method = method_buf[0];
    free(method_buf);
    return ret_method;
}

int main(int argc, char** argv)
{
    const game_methods* game_method = NULL;
    const char* game_options = NULL;
    const char* initial_state = NULL;

    //TODO important, support options as both "--opt=VAL" and "--opt VAL", i.e. if current arg contains '=' then split into next arg
    //TODO use proper command enum style options parsing
    int w_argc = argc - 1; // remaining arg cnt
    while (w_argc > 0) {
        char* w_arg = argv[argc - (w_argc--)]; // working arg
        char* n_arg = (w_argc > 0) ? argv[argc - w_argc] : NULL; // next arg
        if (strcmp(w_arg, "--version") == 0) {
            printf("surena version %d.%d.%d\n", surena::version.major, surena::version.minor, surena::version.patch);
            //TODO api versions?
            printf("git commit hash: %s%s\n", GIT_COMMIT_HASH == NULL ? "<no commit info available>" : GIT_COMMIT_HASH, GIT_COMMIT_DIRTY ? " (dirty)" : "");
            exit(0);
        } else if (strcmp(w_arg, "--game") == 0) {
            w_argc--;
            if (strcmp(n_arg, "chess") == 0) {
                game_method = &chess_gbe;
            } else if (strcmp(n_arg, "havannah") == 0) {
                game_method = &havannah_gbe;
            } else if (strcmp(n_arg, "tictactoe") == 0) {
                game_method = &tictactoe_gbe;
            } else if (strcmp(n_arg, "tictactoe.ultimate") == 0) {
                game_method = &tictactoe_ultimate_gbe;
            } else if (strcmp(n_arg, "twixt.pp") == 0) {
                game_method = &twixt_pp_gbe;
            } else {
                printf("unknown game\n");
            }
        } else if (strcmp(w_arg, "--game-plugin") == 0) {
            w_argc--;
            if (n_arg) {
                game_method = load_plugin_first_method(n_arg);
                if (game_method == NULL) {
                    printf("plugin did not provide at least one game, ignoring\n");
                    printf("HINT: relative plugin paths must be prefixed with \"./\"\n");
                }
            } else {
                printf("ignoring missing game plugin file name\n");
            }
        } else if (strcmp(w_arg, "--game-options") == 0) {
            w_argc--;
            if (n_arg) {
                game_options = n_arg;
            } else {
                printf("ignoring missing game options\n");
            }
        } else if (strcmp(w_arg, "--game-state") == 0) {
            w_argc--;
            if (n_arg) {
                initial_state = n_arg;
            } else {
                printf("ignoring missing initial state\n");
            }
        } else {
            printf("ignoring unknown argument: \"%s\"\n", w_arg);
        }
    }

    if (game_method == NULL) {
        printf("no game method specified\n");
        exit(1);
    }
    if (game_method->debug_print == NULL) {
        printf("game method does not support debug print\n");
        exit(1);
    }

    error_code ec;
    game thegame{
        .methods = game_method,
        .data1 = NULL,
        .data2 = NULL,
    };
    printf("game method: %s.%s.%s %d.%d.%d\n", thegame.methods->game_name, thegame.methods->variant_name, thegame.methods->impl_name, thegame.methods->version.major, thegame.methods->version.minor, thegame.methods->version.patch);
    if (thegame.methods->features.options) {
        ec = thegame.methods->create_with_opts_str(&thegame, game_options);
        if (ec != ERR_OK) {
            printf("failed to create with options \"%s\": #%d %s\n", game_options, ec, thegame.methods->get_last_error(&thegame));
            exit(1);
        }
        size_t options_str_size = thegame.sizer.options_str;
        char* options_str = (char*)malloc(options_str_size);
        thegame.methods->export_options_str(&thegame, &options_str_size, options_str);
        printf("options: \"%s\"\n", options_str);
    } else if (game_options != NULL) {
        printf("game does not support options, ignoring\n");
        ec = thegame.methods->create_default(&thegame);
    } else {
        ec = thegame.methods->create_default(&thegame);
    }
    if (ec != ERR_OK) {
        printf("failed to create: #%d %s\n", ec, thegame.methods->get_last_error(&thegame));
        exit(1);
    }
    ec = thegame.methods->import_state(&thegame, initial_state);
    if (ec != ERR_OK) {
        printf("failed to import state \"%s\": #%d %s\n", initial_state, ec, thegame.methods->get_last_error(&thegame));
        exit(1);
    }
    size_t state_str_size = thegame.sizer.state_str;
    char* state_str = (char*)malloc(state_str_size);
    size_t print_buf_size = thegame.sizer.print_str;
    char* print_buf = thegame.methods->features.print ? (char*)malloc(print_buf_size) : NULL;
    size_t move_str_size = thegame.sizer.move_str;
    move_str_size++; // account for reading '\n' later on
    if (move_str_size < 1024) {
        // increase input buffer to prevent typical input overflows
        // this does not serve security purposes but rather usability, overflow security is handled later on
        move_str_size = 1024;
    }
    char* move_str = (char*)malloc(move_str_size);
    player_id ptm;
    uint8_t ptm_count;
    thegame.methods->players_to_move(&thegame, &ptm_count, &ptm);
    sync_counter sync = SYNC_COUNTER_DEFAULT;
    //TODO adapt loop for simul player games, and what way to print the whole knowing board AND a privacy view hidden board?
    //TODO also print sync data when it becomes available
    while (true) {
        printf("================================\n");
        thegame.methods->export_state(&thegame, &state_str_size, state_str);
        printf("state: \"%s\"\n", state_str);
        bool extra_state = false;
        if (thegame.methods->features.id) {
            extra_state = true;
            uint64_t theid;
            thegame.methods->id(&thegame, &theid);
            printf("ID#%016lx", theid);
        }
        if (thegame.methods->features.eval) {
            if (extra_state) {
                printf(" ");
            }
            extra_state = true;
            float theeval;
            thegame.methods->eval(&thegame, ptm, &theeval);
            printf("EVAL:%.5f", theeval);
        }
        if (extra_state) {
            printf("\n");
        }
        if (thegame.methods->features.print) {
            thegame.methods->debug_print(&thegame, &print_buf_size, print_buf);
            printf("%s", print_buf);
        }
        if (ptm_count == 0) {
            break;
        }
        printf("player to move %d: ", ptm);

        size_t read_pos = 0;
        bool read_stop = false;
        bool read_clear = false;
        while (read_stop == false) {
            int tc = getc(stdin);
            if (tc == EOF) {
                printf("\nerror/aborted\n");
                exit(1);
            }
            if (tc == '\n' || read_pos >= move_str_size - 1) {
                tc = '\0';
                read_stop = true;
                if (read_pos >= move_str_size - 1) {
                    read_clear = true;
                }
            }
            move_str[read_pos++] = tc;
        }
        if (read_clear) {
            // got move str up to buffer length, now clear read buffer until newline char, only really works for stdin on cli but fine for now
            int tc;
            do {
                tc = getc(stdin);
            } while (tc != '\n' && tc != EOF);
        }

        move_code themove;
        ec = thegame.methods->get_move_code(&thegame, PLAYER_NONE, move_str, &themove);
        if (ec == ERR_OK) {
            if (thegame.methods->features.sync_counter) {
                thegame.methods->get_sync_counter(&thegame, &sync);
            } else {
                sync++;
            }
            ec = thegame.methods->is_legal_move(&thegame, ptm, themove, sync);
        }
        if (ec == ERR_OK) {
            thegame.methods->make_move(&thegame, ptm, themove);
        } else {
            printf("invalid move: %s\n", thegame.methods->get_last_error(&thegame));
        }
        thegame.methods->players_to_move(&thegame, &ptm_count, &ptm);
    }
    player_id res;
    uint8_t res_count;
    thegame.methods->get_results(&thegame, &res_count, &res);
    if (res_count == 0) {
        res = PLAYER_NONE;
    }
    printf("result player %d\n", res);
    thegame.methods->destroy(&thegame);
    printf("================================\n");

    printf("done\n");
    return 0;
}
