#include "commands/cmd_status.h"

#include <stdio.h>
#include <stdlib.h>

#include "core/snapshot.h"
#include "utils/color.h"
#include "utils/error_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

void leader_cmp_index(const char *path, int result) {
    if (result == MATCH) {
        return;
    }
    printf(COLOR_GREEN "\t%s:    %s\n" COLOR_END, (result == NOT_FOUND) ? "New File" : "Modified",
           path);
}

void cmd_status(int argc, char *argv[]) {
    UNUSED(argv);

    if (check_init() == NOT_INIT) {
        NotInitError();
    }
    if (argc > 1) {
        ErrorCustomMsg("Usage: big status\n");
    }
    cd_to_project_root(NULL);

    char *leader_commit_id;
    SnapshotBST *bst_leader = read_leader_commit_BST(&leader_commit_id);

    SnapshotBST *bst_index = read_index_dic(NULL, NULL);

    SnapshotBST *bst_dir = SnapshotBSTCreateEmpty();
    process_path(bst_dir, ".", NULL, NULL);

    if (amount_of_BST(bst_index) != 0 && bst_leader != NULL) {
        printf("\nReady to commit:\n");
        compare_two_trees(bst_leader, bst_index, leader_cmp_index);
        puts("");
    }

    xfree(leader_commit_id);
    SnapshotBSTDestory(&bst_dir);
    SnapshotBSTDestory(&bst_index);
    SnapshotBSTDestory(&bst_leader);
}