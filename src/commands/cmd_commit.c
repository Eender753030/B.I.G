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
        if (strcmp(argv[1], "-m") == 0 && argc == 3) {
            log_message = argv[2];
        } else {
            ErrorCustomMsg("Usage: big commit [-m \"<log message>\"]\n");
        }
    }

    cd_to_project_root(NULL);

    if (access(".big/index", F_OK) == -1) {
        ErrorCustomMsg("Error: Nothing to commit\n");
    }

    char *leader_hash = load_leader();
    if (leader_hash != NULL) {
        char leader_list_path[1024];
        snprintf(leader_list_path, sizeof(leader_list_path), ".big/objects/%s/list", leader_hash);

        snapshot_bst_t *leader_bst = read_index_file_from_path(leader_list_path);

        if (is_same_bst(index_bst, leader_bst, is_same_file_info) == true) {
            xfree(leader_hash);
            snapshot_bst_free(&index_bst);
            snapshot_bst_free(&leader_bst);
            ErrorCustomMsg("Error: Nothing to commit\n");
        }

        snapshot_bst_free(&leader_bst);
        xfree(leader_hash);
    }

    snapshot_bst_free(&index_bst);

    commit_node_t *new_commit = commit_node_create(log_message);

    save_commit_obj(new_commit);

    update_leader(new_commit);

    commit_node_free(&new_commit);
}