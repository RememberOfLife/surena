#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rosalia/semver.h"

#include "surena/engine.h"
#include "surena/game.h"
#include "surena/move_history.h"

#include "generated/git_commit_hash.h"
#include "repl.h"

namespace surena {
    const semver version = {0, 17, 2};
} // namespace surena

int main(int argc, char** argv)
{
    //TODO fix this up
    int w_argc = argc - 1; // remaining arg cnt
    char* w_arg = argv[1];
    if (w_argc < 1) {
        int str_pad = -15;
        printf("Usage: surena [options]\n");
        printf("Options:\n");
        printf("  %*s%s\n", str_pad, "repl", "Run in repl mode.");
        printf("  %*s%s\n", str_pad, "version", "Display surena version information.");
        exit(EXIT_SUCCESS);
    }
    if (strcmp(w_arg, "version") == 0) {
        printf("surena version %d.%d.%d\n", surena::version.major, surena::version.minor, surena::version.patch);
        printf("apis: game(%lu) engine(%lu) move_history(%lu)\n", SURENA_GAME_API_VERSION, SURENA_ENGINE_API_VERSION, SURENA_MOVE_HISTORY_API_VERSION);
        printf("git commit hash: %s%s\n", GIT_COMMIT_HASH == NULL ? "<no commit info available>" : GIT_COMMIT_HASH, GIT_COMMIT_HASH != NULL && GIT_COMMIT_DIRTY ? " (dirty)" : "");
        exit(EXIT_SUCCESS);
    } else if (strcmp(w_arg, "repl") == 0) {
        //TODO allow for initial input via cli
        return repl();
    }
    printf("unrecognized verb \"%s\"; run \"surena\" for usage\n", w_arg);
    exit(EXIT_FAILURE);
}
