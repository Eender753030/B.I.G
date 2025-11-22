#include "commands/cmd_commit.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core/commit_graph.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

void cmd_commit(int argc, char *argv[]) {
    if (check_init() == -1)
        NotInitError();

    char *log_message;

    if (argc == 1)
        log_message = NULL;
    else {
        if (strncmp(argv[1], "-m", 3) == 0 && argc == 3)
            log_message = argv[2];
        else
            ErrorCustomMsg("Usage: big commit [-m \"<log message>\"]\n");
    }

    cd_to_project_root(NULL);

    if (access(".big/index", F_OK) == -1)
        ErrorCustomMsg("Error: Nothing to commit\n");

    CommitNode *new_commit = CommitNodeCreate(commit_log_insert(log_message));

    save_object_file(new_commit);

    leader_update(new_commit);

    remove(".big/index");
    CommitNodeFree(&new_commit);
}