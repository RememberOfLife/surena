#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "surena/games/chess.h"
#include "surena/games/havannah.h"
#include "surena/games/oshisumo.h"
#include "surena/games/tictactoe_ultimate.h"
#include "surena/games/tictactoe.h"
#include "surena/games/twixt_pp.h"
#include "surena/game_plugin.h"
#include "surena/game.h"

#include "repl.h"

// string pointer to first char occurence
// returns either ptr to the first occurence of c in str, or ptr to the zero terminator if c was not found in str
const char* strpfc(const char* str, char c)
{
    const char* wstr = str;
    while (*wstr != c && *wstr != '\0') {
        wstr++;
    }
    return wstr;
}

// argc and argv are optional, the string is transformed into zero terminator separated args anyway
// if str is NULL, then this frees the content of argv
void strargsplit(char* str, int* argc, char*** argv)
{
    //TODO want do backslash escapes
    //TODO
}

const game_methods* load_plugin_game_methods(const char* file, uint32_t idx)
{
    void* dll_handle = dlopen(file, RTLD_LAZY);
    plugin_get_game_capi_version_t version = (plugin_get_game_capi_version_t)dlsym(dll_handle, "plugin_get_game_capi_version");
    if (version == NULL || version() != SURENA_GAME_API_VERSION) {
        return NULL;
    }
    void (*init)() = (void (*)())dlsym(dll_handle, "plugin_init_game");
    if (init == NULL) {
        return NULL;
    }
    init();
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
    if (idx >= method_cnt) {
        return NULL;
    }
    const game_methods* ret_method = method_buf[idx];
    free(method_buf);
    return ret_method;
}

//TODO keep this ordered via a sort and compare func, for now use hierarchical
const game_methods* static_game_methods[] = {
    // &chess_gbe,
    // &havannah_gbe,
    &tictactoe_standard_gbe,
    // &tictactoe_ultimate_gbe,
    // &twixt_pp_gbe,
};

const game_methods* load_static_game_methods(const char* composite_id)
{
    const int GNAME_IDX = 0;
    const int VNAME_IDX = 1;
    const int INAME_IDX = 2;
    char id_name[3][64] = {'\0', '\0', '\0'};
    // split composite id into proper g/v/i names
    const char* wstr = composite_id;
    const char* estr = composite_id;
    for (int i = 0; i < 3; i++) {
        if (*estr == '\0') {
            break;
        }
        if (i > 0) {
            estr++;
        }
        wstr = estr;
        estr = strpfc(estr, '.');
        if (estr - wstr >= 64) {
            return NULL;
        }
        strncpy(id_name[i], wstr, estr - wstr);
    }
    if (strlen(id_name[GNAME_IDX]) == 0) {
        return NULL;
    }
    if (strlen(id_name[VNAME_IDX]) == 0) {
        strcpy(id_name[VNAME_IDX], "Standard");
    }
    // find correctly named game method
    size_t statics_cnt = sizeof(static_game_methods) / sizeof(game_methods*);
    for (size_t i = 0; i < statics_cnt; i++) {
        if (strcmp(id_name[GNAME_IDX], static_game_methods[i]->game_name) == 0 &&
            strcmp(id_name[VNAME_IDX], static_game_methods[i]->variant_name) == 0 &&
            (strlen(id_name[INAME_IDX]) == 0 || strcmp(id_name[INAME_IDX], static_game_methods[i]->impl_name) == 0)) {
            return static_game_methods[i];
        }
    }
    return NULL;
}

void print_game_error(game* g, error_code ec)
{
    if (ec == ERR_OK) {
        return;
    }
    if (game_ff(g).error_strings == true) {
        printf("[WARN] game error #%u (%s): %s\n", ec, get_general_error_string(ec, "unknown specific"), game_get_last_error(g));
    } else {
        printf("[WARN] game error #%u (%s)\n", ec, get_general_error_string(ec, "unknown specific"));
    }
}

// read up to buf_size characters from stdin into buf, guaranteeing a zero terminator after them and cutting off any excess characers
// automatically clears stdin after getting the characters, so further calls will not read them
// returns true if input was truncated
bool readbufsafe(char* buf, size_t buf_size)
{
    size_t read_pos = 0;
    bool read_stop = false;
    bool read_clear = false;
    while (read_stop == false) {
        int tc = getc(stdin);
        if (tc == EOF) {
            printf("\nerror/eof\n");
            exit(EXIT_FAILURE);
        }
        if (tc == '\n' || read_pos >= buf_size - 1) {
            tc = '\0';
            read_stop = true;
            if (read_pos >= buf_size - 1) {
                read_clear = true;
            }
        }
        buf[read_pos++] = tc;
    }
    if (read_clear) {
        // got input str up to buffer length, now clear read buffer until newline char, only really works for stdin on cli but fine for now
        int tc;
        do {
            tc = getc(stdin);
        } while (tc != '\n' && tc != EOF);
    }
    return read_clear;
}

typedef struct repl_state_s {
    bool exit;
    //TODO set and get variable array and enum type + string tag list
    const game_methods* g_methods;
    const char* g_c_options;
    const char* g_c_legacy;
    const char* g_c_initial_state;
    const char* g_c_b64_serialized;
    game g;
    player_id pov;
} repl_state;

typedef enum REPL_CMD_E {
    REPL_CMD_NONE = 0,
    //TODO M_HELP
    //TODO M_LIST_STATIC
    REPL_CMD_M_LOAD_STATIC,
    REPL_CMD_M_LOAD_PLUGIN, //TODO support unloading?
    REPL_CMD_M_EXIT,
    REPL_CMD_M_GET,
    REPL_CMD_M_SET,
    //TODO M_POV
    //TODO M_RS
    //TODO M_GINFO
    REPL_CMD_G_CREATE,
    REPL_CMD_G_DESTROY,
    // REPL_CMD_G_EXPORT_OPTIONS,
    // REPL_CMD_G_PLAYER_COUNT,
    // REPL_CMD_G_EXPORT_STATE,
    // REPL_CMD_G_IMPORT_STATE,
    // REPL_CMD_G_SERIALIZE,
    // REPL_CMD_G_PLAYERS_TO_MOVE,
    // REPL_CMD_G_GET_CONCRETE_MOVES,
    // REPL_CMD_G_GET_CONCRETE_MOVE_PROBABILITIES,
    // REPL_CMD_G_GET_CONCRETE_MOVES_ORDERED,
    // REPL_CMD_G_GET_ACTIONS,
    // REPL_CMD_G_IS_LEGAL_MOVE,
    // REPL_CMD_G_MOVE_TO_ACTION,
    // REPL_CMD_G_IS_ACTION,
    REPL_CMD_G_MAKE_MOVE,
    // REPL_CMD_G_GET_RESULTS,
    // REPL_CMD_G_EXPORT_LEGACY,
    // REPL_CMD_G_GET_SCORES,
    // REPL_CMD_G_ID,
    // REPL_CMD_G_EVAL,
    // REPL_CMD_G_DISCRETIZE,
    // REPL_CMD_G_PLAYOUT,
    // REPL_CMD_G_REDACT_KEEP_STATE,
    // REPL_CMD_G_EXPORT_SYNC_DATA,
    // REPL_CMD_G_IMPORT_SYNC_DATA,
    // REPL_CMD_G_GET_MOVE_DATA,
    // REPL_CMD_G_GET_MOVE_STR,
    REPL_CMD_G_PRINT,
    REPL_CMD_COUNT,
} REPL_CMD;

typedef void repl_cmd_func_t(repl_state* rs, const char* args);

repl_cmd_func_t repl_cmd_handle_m_load_static;
repl_cmd_func_t repl_cmd_handle_m_load_plugin;
repl_cmd_func_t repl_cmd_handle_m_exit;
repl_cmd_func_t repl_cmd_handle_m_get;
repl_cmd_func_t repl_cmd_handle_m_set;
repl_cmd_func_t repl_cmd_handle_g_create;
repl_cmd_func_t repl_cmd_handle_g_destroy;
repl_cmd_func_t repl_cmd_handle_g_make_move;
repl_cmd_func_t repl_cmd_handle_g_print;

typedef struct game_command_info_s {
    char* text; //TODO support multiple alias via "abc\0def\0"
    repl_cmd_func_t* handler;
} game_command_info;

//TODO separate commands for game and meta like repl state and info and load mehtods
game_command_info game_command_infos[REPL_CMD_COUNT] = {
    [REPL_CMD_NONE] = {"noop", NULL},
    [REPL_CMD_M_LOAD_STATIC] = {"load_static", repl_cmd_handle_m_load_static},
    [REPL_CMD_M_LOAD_PLUGIN] = {"load_plugin", repl_cmd_handle_m_load_plugin},
    [REPL_CMD_M_EXIT] = {"exit", repl_cmd_handle_m_exit},
    [REPL_CMD_M_GET] = {"get", repl_cmd_handle_m_get},
    [REPL_CMD_M_SET] = {"set", repl_cmd_handle_m_set},
    [REPL_CMD_G_CREATE] = {"create", repl_cmd_handle_g_create},
    [REPL_CMD_G_DESTROY] = {"destroy", repl_cmd_handle_g_destroy},
    // [REPL_CMD_G_EXPORT_OPTIONS] = {"export_options", NULL},
    // [REPL_CMD_G_PLAYER_COUNT] = {"player_count", NULL},
    // [REPL_CMD_G_EXPORT_STATE] = {"export_state", NULL},
    // [REPL_CMD_G_IMPORT_STATE] = {"import_state", NULL},
    // [REPL_CMD_G_SERIALIZE] = {"serialize", NULL},
    // [REPL_CMD_G_PLAYERS_TO_MOVE] = {"players_to_move", NULL},
    // [REPL_CMD_G_GET_CONCRETE_MOVES] = {"get_concrete_moves", NULL},
    // [REPL_CMD_G_GET_CONCRETE_MOVE_PROBABILITIES] = {"get_concrete_move_probabilities", NULL},
    // [REPL_CMD_G_GET_CONCRETE_MOVES_ORDERED] = {"get_concrete_moves_ordered", NULL},
    // [REPL_CMD_G_GET_ACTIONS] = {"get_actions", NULL},
    // [REPL_CMD_G_IS_LEGAL_MOVE] = {"is_legal_move", NULL},
    // [REPL_CMD_G_MOVE_TO_ACTION] = {"move_to_action", NULL},
    // [REPL_CMD_G_IS_ACTION] = {"is_action", NULL},
    [REPL_CMD_G_MAKE_MOVE] = {"make_move", repl_cmd_handle_g_make_move},
    // [REPL_CMD_G_GET_RESULTS] = {"get_results", NULL},
    // [REPL_CMD_G_EXPORT_LEGACY] = {"export_legacy", NULL},
    // [REPL_CMD_G_GET_SCORES] = {"get_scores", NULL},
    // [REPL_CMD_G_ID] = {"id", NULL},
    // [REPL_CMD_G_EVAL] = {"eval", NULL},
    // [REPL_CMD_G_DISCRETIZE] = {"discretize", NULL},
    // [REPL_CMD_G_PLAYOUT] = {"playout", NULL},
    // [REPL_CMD_G_REDACT_KEEP_STATE] = {"redact_keep_state", NULL},
    // [REPL_CMD_G_EXPORT_SYNC_DATA] = {"export_sync_data", NULL},
    // [REPL_CMD_G_IMPORT_SYNC_DATA] = {"import_sync_data", NULL},
    // [REPL_CMD_G_GET_MOVE_DATA] = {"get_move_data", NULL},
    // [REPL_CMD_G_GET_MOVE_STR] = {"get_move_str", NULL},
    [REPL_CMD_G_PRINT] = {"print", repl_cmd_handle_g_print},
};

void handle_command(repl_state* rs, const char* str)
{
    /*TODO ideally cmd list wanted:
    :set option=value // use for things like print_after_move=off and similarly :set options="8+" and :set pov=1
    :get option
    :create [options] [legacy] [initial_state] // want default params here?
    :destroy
    :move
    :ptm
    :get_moves{_prob,_ordered}
    :info // prints lots of things about the game
    */
    const char* wstr = str;
    const char* estr = strpfc(str, ' ');
    size_t cmp_len = estr - wstr;
    REPL_CMD cmd_type = REPL_CMD_NONE;
    for (REPL_CMD ct = 0; ct < REPL_CMD_COUNT; ct++) {
        if (strncmp(wstr, game_command_infos[ct].text, cmp_len) == 0) {
            cmd_type = ct;
            break;
        }
    }
    if (*estr != '\0') {
        estr++;
    }
    wstr = estr;
    if (cmd_type != REPL_CMD_NONE && cmd_type < REPL_CMD_COUNT) {
        game_command_infos[cmd_type].handler(rs, wstr);
    } else {
        printf("[INFO] unknown command\n");
    }
}

char cmd_prefix = '/';

int repl()
{
    repl_state rs = (repl_state){
        .exit = false,
        .g_methods = NULL,
        .g_c_options = NULL,
        .g_c_legacy = NULL,
        .g_c_initial_state = NULL,
        .g_c_b64_serialized = NULL,
        .g = (game){
            .methods = NULL,
            .data1 = NULL,
            .data2 = NULL,
            .sync_ctr = SYNC_CTR_DEFAULT,
        },
        .pov = PLAYER_NONE,
    };
    const size_t read_buf_size = 4096;
    char* read_buf = malloc(read_buf_size);
    while (rs.exit == false) {
        // read input
        printf("(pov%03hhu)> ", rs.pov);
        bool input_truncated = readbufsafe(read_buf, read_buf_size);
        if (input_truncated) {
            printf("[WARN] input was truncated to %zu characters\n", read_buf_size - 1);
        }
        // eval
        if (*read_buf == cmd_prefix) {
            // is a command
            handle_command(&rs, read_buf + 1);
        } else {
            // unknown
            printf("[INFO] not a command, direct moves unsupported for now, use \":help\" for help\n");
            //TODO this should be the default "move"
        }
        // print
        //TODO
    }
    free(read_buf);

    return 0;
}

/*

    if (game_method->features.print == false) {
        printf("[WARN] game method does not support debug print\n");
    }

    error_code ec;
    game thegame{
        .methods = game_method,
        .data1 = NULL,
        .data2 = NULL,
    };
    printf("[INFO] game method: %s.%s.%s %d.%d.%d\n", thegame.methods->game_name, thegame.methods->variant_name, thegame.methods->impl_name, thegame.methods->version.major, thegame.methods->version.minor, thegame.methods->version.patch);
    if (game_options != NULL && thegame.methods->features.options == false) {
        printf("[WARN] game does not support options, ignoring\n");
    }

    // game gets created

    if (ec != ERR_OK) {
        printf("[ERROR] failed to create: #%d %s\n", ec, thegame.methods->features.error_strings ? thegame.methods->get_last_error(&thegame) : NULL);
        printf("%*soptions: %s\n", 8, "", game_options);
        printf("%*slegacy: %s\n", 8, "", game_legacy);
        printf("%*sinitial state: %s\n", 8, "", initial_state);
        exit(1);
    }
    if (thegame.methods->features.options) {
        size_t options_str_size = thegame.sizer.options_str;
        char* options_str = (char*)malloc(options_str_size);
        thegame.methods->export_options(&thegame, &options_str_size, options_str);
        printf("[INFO] options: \"%s\"\n", options_str);
        free(options_str);
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
            thegame.methods->print(&thegame, &print_buf_size, print_buf);
            printf("%s", print_buf);
        }
        if (ptm_count == 0) {
            break;
        }
        printf("player to move %d: ", ptm);

        // input reading

        move_code themove;
        ec = thegame.methods->get_move_code(&thegame, PLAYER_NONE, move_str, &themove);
        if (ec == ERR_OK) {
            ec = thegame.methods->is_legal_move(&thegame, ptm, themove);
        }
        if (ec == ERR_OK) {
            thegame.methods->make_move(&thegame, ptm, themove);
        } else {
            printf("invalid move: %s\n", thegame.methods->features.error_strings ? thegame.methods->get_last_error(&thegame) : NULL);
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

    free(state_str);
    free(print_buf);
    free(move_str);

*/

void repl_cmd_handle_m_load_static(repl_state* rs, const char* args)
{
    if (strlen(args) == 0) {
        printf("[WARN] game name composite missing\n");
        return;
    }
    const game_methods* gm = load_static_game_methods(args);
    if (gm == NULL) {
        printf("[WARN] game methods \"%s\" not found\n", args);
    } else {
        printf("[INFO] found: %s.%s.%s v%u.%u.%u\n", gm->game_name, gm->variant_name, gm->impl_name, gm->version.major, gm->version.minor, gm->version.patch);
        rs->g_methods = gm;
    }
}

void repl_cmd_handle_m_load_plugin(repl_state* rs, const char* args)
{
    //TODO split args into path and idx properly //TODO use all purpose arg splitter
    if (strlen(args) == 0) {
        printf("[WARN] plugin file path missing\n");
        return;
    }
    const game_methods* gm = load_plugin_game_methods(args, 0);
    if (gm == NULL) {
        printf("[WARN] plugin did not provide at least one game, ignoring\n");
        printf("[INFO] relative plugin paths MUST be prefixed with \"./\"\n");
    } else {
        printf("[INFO] loaded method 0: %s.%s.%s v%u.%u.%u\n", gm->game_name, gm->variant_name, gm->impl_name, gm->version.major, gm->version.minor, gm->version.patch);
        rs->g_methods = gm;
    }
}

void repl_cmd_handle_m_exit(repl_state* rs, const char* args)
{
    rs->exit = true;
}

void repl_cmd_handle_m_get(repl_state* rs, const char* args)
{
    //TODO
    printf("[WARN] feature unsupported");
}

void repl_cmd_handle_m_set(repl_state* rs, const char* args)
{
    //TODO
    printf("[WARN] feature unsupported");
}

void repl_cmd_handle_g_create(repl_state* rs, const char* args)
{
    //TODO switch on args to see if should create using std/b64 and then on if to use provided or from cache
    if (rs->g_methods == NULL) {
        printf("[WARN] can not create game: no methods selected\n");
        return;
    }
    error_code ec;
    if (rs->g.methods != NULL) { // destroy old game
        ec = game_destroy(&rs->g);
        rs->g = (game){
            .methods = NULL,
            .data1 = NULL,
            .data2 = NULL,
            .sync_ctr = SYNC_CTR_DEFAULT,
        };
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game destruction error\n");
        }
    }
    rs->g.methods = rs->g_methods;
    //TODO use selected init type
    game_init game_init_info = (game_init){
        .source_type = GAME_INIT_SOURCE_TYPE_STANDARD,
        .source = {
            .standard = {
                .opts = rs->g_c_options,
                .legacy = rs->g_c_legacy,
                .state = rs->g_c_initial_state,
            },
        },
    };
    ec = game_create(&rs->g, &game_init_info);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        //TODO destroy on failed create could be optional
        ec = game_destroy(&rs->g);
        rs->g = (game){
            .methods = NULL,
            .data1 = NULL,
            .data2 = NULL,
            .sync_ctr = SYNC_CTR_DEFAULT,
        };
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game destruction error\n");
        }
    }
}

void repl_cmd_handle_g_destroy(repl_state* rs, const char* args)
{
    if (rs->g.methods == NULL) {
        printf("[INFO] no game running\n");
        return;
    }
    error_code ec = game_destroy(&rs->g);
    rs->g = (game){
        .methods = NULL,
        .data1 = NULL,
        .data2 = NULL,
        .sync_ctr = SYNC_CTR_DEFAULT,
    };
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("[ERROR] unexpected game destruction error\n");
    }
}

void repl_cmd_handle_g_make_move(repl_state* rs, const char* args)
{
    if (rs->g.methods == NULL) {
        printf("[INFO] no game running\n");
        return;
    }
    //TODO
}

void repl_cmd_handle_g_print(repl_state* rs, const char* args)
{
    //TODO print optionally takes a pov to use
    if (rs->g.methods == NULL) {
        printf("[INFO] no game running\n");
        return;
    }
    if (game_ff(&rs->g).print == false) {
        printf("[INFO] game does not support feature: print\n");
        return;
    }
    size_t print_size;
    const char* print_str;
    error_code ec = game_print(&rs->g, rs->pov, &print_size, &print_str);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        return;
    }
    printf("%s", print_str);
}
