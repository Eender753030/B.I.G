#include "commands/cmd_commit.h"

#include <unistd.h>

#include "core/commit_graph.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

void cmd_commit(const char *log_message) {
    cd_to_project_root(NULL);

    if (access(".big/index", F_OK) == -1)
        ErrorCustomMsg("Error: Nothing to commit\n");

    CommitNode *new_commit = CommitNodeCreate(commit_log_insert(log_message));

    save_object_file(new_commit);

    leader_update(new_commit);

    remove(".big/index");
    CommitNodeFree(&new_commit);
}