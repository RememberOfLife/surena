#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rosalia/semver.h"

#include "generated/git_commit_hash.h"
#include "repl.h"

namespace surena {
    const semver version = {0, 14, 0};
} // namespace surena

int main(int argc, char** argv)
{
    //TODO fix this up
    int w_argc = argc - 1; // remaining arg cnt
    char* w_arg = argv[1];
    if (w_argc < 1) {
        printf("no args mode not supported right now\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(w_arg, "version") == 0) {
        printf("surena version %d.%d.%d\n", surena::version.major, surena::version.minor, surena::version.patch);
        //TODO api versions?
        printf("git commit hash: %s%s\n", GIT_COMMIT_HASH == NULL ? "<no commit info available>" : GIT_COMMIT_HASH, GIT_COMMIT_HASH != NULL && GIT_COMMIT_DIRTY ? " (dirty)" : "");
        exit(EXIT_SUCCESS);
    } else if (strcmp(w_arg, "repl") == 0) {
        //TODO allow for initial input via cli
        return repl();
    }
    printf("unrecognized verb \"%s\"\n", w_arg);
    exit(EXIT_FAILURE);
}
