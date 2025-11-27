#include "commands/cmd_commit.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core/commit.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

void cmd_commit(int argc, char *argv[]) {
    if (check_init() == NOT_INIT)
        NotInitError();

    char *log_message;

    if (argc == 1)
        log_message = NULL;
    else {
        if (strncmp(argv[1], "-m", 3) == 0 && argc == 3) {
            log_message = argv[2];
        } else {
            ErrorCustomMsg("Usage: big commit [-m \"<log message>\"]\n");
        }
    }

    cd_to_project_root(NULL);

    if (access(".big/index", F_OK) == -1) {
        ErrorCustomMsg("Error: Nothing to commit\n");
    }
    commit_node_t *new_commit = commit_node_create(log_message);

    save_commit_obj(new_commit);

    update_leader(new_commit);

    commit_node_free(&new_commit);
}