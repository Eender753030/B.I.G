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
    if (check_init() == NOT_INIT) {
        error_not_init();
    }

    char *log_message;

    if (argc == 1)
        log_message = NULL;
    else {
        if (strcmp(argv[1], "-m") == 0 && argc == 3) {
            log_message = argv[2];
        } else {
            error_custom_msg("Usage: big commit [-m \"<log message>\"]\n");
        }
    }

    cd_to_project_root(NULL);

    snapshot_bst_t *index_bst = read_index_file();
    if (bst_get_amount(index_bst) == 0) {
        snapshot_bst_free(&index_bst);
        error_custom_msg("Error: Nothing to commit\n");
    }

    char *leader_hash = load_leader();
    if (leader_hash != NULL) {
        if (access(".big/refs/temp_checkout_ref", F_OK) == 0) {
            warning_custom_msg(
                "Warning: Are sure you really want to commit? This will replace all commits after "
                "this\nYou can use 'big checkout Leader' Back to top commit\nIf you do want to. "
                "Please enter current branch name: ");

            char user_input[4096];
            char *curr_branch_name = load_current_branch();
            fgets(user_input, sizeof(user_input), stdin);
            user_input[strcspn(user_input, "\n")] = '\0';

            if (strcmp(user_input, curr_branch_name) != 0) {
                snapshot_bst_free(&index_bst);
                xfree(curr_branch_name);
                error_custom_msg("Commit operation cancelled");
            }

            xfree(curr_branch_name);
            xfree(leader_hash);
            leader_hash = load_ref_hash(".big/refs/temp_checkout_ref");
            remove(".big/refs/temp_checkout_ref");
        }

        char leader_list_path[1024];
        snprintf(leader_list_path, sizeof(leader_list_path), ".big/objects/%s/list", leader_hash);

        snapshot_bst_t *leader_bst = read_index_file_from_path(leader_list_path);

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

    commit_node_t *new_commit = commit_node_create(log_message);

    save_commit_obj(new_commit);

    update_branch(new_commit);

    commit_node_free(&new_commit);
}