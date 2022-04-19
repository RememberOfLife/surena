#include <cstdio>
#include <cstdlib>
#include <cstring>

// #include "surena/engines/singlethreadedmcts.hpp"
// #include "surena/games/alhambra.hpp"
// #include "surena/games/caesar.hpp"
// #include "surena/games/chess.hpp"
#include "surena/games/tictactoe_ultimate.h"
#include "surena/games/tictactoe.h"
// #include "surena/games/havannah.hpp"
#include "surena/util/semver.h"
// #include "surena/engine.hpp"
#include "surena/game.h"

namespace surena {
    const semver version = {0, 7, 0};
}

// args https://github.com/p-ranav/argparse

int main(int argc, char* argv[])
{
    const char* initial_position = NULL;
    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0) {
            printf("surena version %d.%d.%d\n", surena::version.major, surena::version.minor, surena::version.patch);
            return 0;
        } else if (strcmp(argv[1], "--initial-position") == 0) {
            if (argc > 2) {
                initial_position = argv[2];
            } else {
                printf("ignoring missing initial position\n");
            }
        } else {
            printf("ignoring unknown argument\n");
        }
        //TODO make this a loop and switch on games to play and engines to use
    }

    error_code ec;
    game thegame{
        .sync_ctr = 0,
        .padding = 0,
        .data = NULL,
        .options = NULL,
        .methods = &tictactoe_ultimate_gbe,
    };
    thegame.methods->create(&thegame);
    printf("created game: %s.%s.%s %d.%d.%d\n",
        thegame.methods->game_name, thegame.methods->variant_name, thegame.methods->impl_name,
        thegame.methods->version.major, thegame.methods->version.minor, thegame.methods->version.patch);
    ec = thegame.methods->import_state(&thegame, initial_position);
    if (ec != ERR_OK) {
        printf("failed to import state \"%s\": %d\n", initial_position, ec);
        return 1;
    }
    size_t print_buf_size;
    thegame.methods->debug_print(&thegame, &print_buf_size, NULL);
    char* print_buf = (char*)malloc(print_buf_size);
    size_t move_str_size;
    thegame.methods->get_move_str(&thegame, &move_str_size, NULL, MOVE_NONE);
    char* move_str = (char*)malloc(move_str_size);
    player_id ptm;
    uint8_t ptm_count;
    thegame.methods->players_to_move(&thegame, &ptm_count, &ptm);
    while (ptm_count > 0) {
        printf("================================\n");
        thegame.methods->debug_print(&thegame, &print_buf_size, print_buf);
        printf("%s", print_buf);
        printf("player to move %d: ", ptm);
        scanf("%2s", move_str);
        move_code themove;
        ec = thegame.methods->get_move_code(&thegame, &themove, move_str);
        if (ec == ERR_OK) {
            ec = thegame.methods->is_legal_move(&thegame, ptm, themove, thegame.sync_ctr);
        }
        if (ec == ERR_OK) {
            thegame.methods->make_move(&thegame, ptm, themove);
        } else {
            printf("invalid move\n");
        }
        thegame.methods->players_to_move(&thegame, &ptm_count, &ptm);
    }
    printf("================================\n");
    thegame.methods->debug_print(&thegame, &print_buf_size, print_buf);
    printf("%s", print_buf);
    player_id res;
    uint8_t res_count;
    thegame.methods->get_results(&thegame, &res_count, &res);
    if (res_count == 0) {
        res = PLAYER_NONE;
    }
    printf("result player %d\n", res);
    thegame.methods->destroy(&thegame);

    printf("done\n");
    return 0;
}
