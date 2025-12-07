#include "commands/cmd_commit.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core/commit.h"
#include "core/index.h"
#include "core/snapshot.h"
#include "utils/error_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

void cmd_commit(int argc, char *argv[]) {
    // Check initalize first
    if (check_init() == NOT_INIT) {
        error_not_init();
    }

    // Only 'big commit' for input, use editor to write log
    char *log_message = NULL;

    // Use 'big commit -m <"msg"> for input, "msg" is the log
    if (argc > 1) {
        if (strcmp(argv[1], "-m") == 0 && argc == 3) {
            log_message = argv[2];
        } else {
            error_custom_msg("Usage: big commit [-m \"<log message>\"]\n");
        }
    }

    // Change working directory to project directory
    cd_to_project_root(NULL);

    // Read out BST of global index
    snapshot_bst_t *index_bst = read_index_file();
    if (bst_get_amount(index_bst) == 0) {
        // 0 means no staged file
        snapshot_bst_free(&index_bst);
        error_custom_msg("Error: Nothing to commit\n");
    }

    char *leader_hash = load_leader();
    // If has previous commit
    if (leader_hash != NULL) {
        char leader_list_path[1024];
        snprintf(leader_list_path, sizeof(leader_list_path), ".big/objects/%s/list", leader_hash);

        snapshot_bst_t *leader_bst = read_index_file_from_path(leader_list_path);

        // Check previous commit is same as index or not
        // Sames means there is no change of status
        if (is_same_bst(index_bst, leader_bst, is_same_file_info) == true) {
            xfree(leader_hash);
            snapshot_bst_free(&index_bst);
            snapshot_bst_free(&leader_bst);
            error_custom_msg("Error: Nothing to commit\n");
        }

        snapshot_bst_free(&leader_bst);
        xfree(leader_hash);
    }

    snapshot_bst_free(&index_bst);

    // Create a commit node from log
    commit_node_t *new_commit = commit_node_create(log_message);

    // Save it to objects/
    save_commit_obj(new_commit);

    // Update current branch's hash
    update_branch(new_commit);

    commit_node_free(&new_commit);
}