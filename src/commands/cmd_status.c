#include "commands/cmd_status.h"

#include <stdio.h>
#include <stdlib.h>

#include "core/snapshot.h"
#include "utils/color.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

void cmd_status(int argc, char *argv[]) {
    UNUSED(argv);

    if (check_init() == NOT_ININ) {
        NotInitError();
    }
    if (argc > 1) {
        ErrorCustomMsg("Usage: big status\n");
    }
    cd_to_project_root(NULL);

    size_t total_size_index = 0;
    SnapshotBST *bst_index = read_index_file(&total_size_index);

    size_t total_size_dir = 0;
    SnapshotBST *bst_dir = SnapshotBSTCreateEmpty();
    process_path(bst_dir, ".", &total_size_dir);

    if (total_size_index != 0) {
        printf("\nReady to commit:\n");
        inorder_traversal_print(bst_index, "\ttracked:    ", COLOR_GREEN);
        puts("");

        inorder_traversal_delete(bst_dir, bst_index, &total_size_dir);
    }

    if (total_size_dir != 0) {
        printf("\nNot in index:\n");
        inorder_traversal_print(bst_dir, "\tuntracked:    ", COLOR_RED);
        puts("");
    }

    SnapshotBSTDestory(&bst_dir);
    SnapshotBSTDestory(&bst_index);
}