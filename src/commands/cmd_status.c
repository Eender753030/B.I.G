#include "commands/cmd_status.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/commit.h"
#include "core/index.h"
#include "core/snapshot.h"
#include "utils/color.h"
#include "utils/error_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

// Two compare condition
#define LEADER_VS_INDEX true
#define INDEX_VS_DIR false

// Helper function that compare two BST and print out the status
static void print_diff(snapshot_bst_t *bst1, snapshot_bst_t *bst2, bool group) {
    // Create two lists of two BSTs
    file_info_t **list1 = (file_info_t **)bst_inorder_to_list(bst1);
    file_info_t **list2 = (file_info_t **)bst_inorder_to_list(bst2);

    uint64_t list1_len = bst_get_amount(bst1);
    uint64_t list2_len = bst_get_amount(bst2);
    uint64_t i = 0, j = 0;

    // Staged files use green, unstaged files use red
    char *color = (group == LEADER_VS_INDEX) ? COLOR_GREEN : COLOR_RED;

    // Compare two lists after both reach to end, this is O(n + m)
    while (i < list1_len || j < list2_len) {
        char *path1, *hash1;
        char *path2, *hash2;
        if (i < list1_len) {
            file_info_get_content(list1[i], &path1, &hash1, NULL);
        }
        if (j < list2_len) {
            file_info_get_content(list2[j], &path2, &hash2, NULL);
        }

        int cmp_result;
        if (i >= list1_len) {
            // bst1 reach to end, just set cmp_result as 1
            cmp_result = 1;
        } else if (j >= list2_len) {
            // bst2 reach to end, just set cmp_result as -1
            cmp_result = -1;
        } else {
            // Compare two path
            cmp_result = strcmp(path1, path2);
        }

        if (cmp_result < 0) {
            // Leader vs Index: Leader has the file but not in index, means deleted in index
            // Index vs Dir: means deleted in project directory
            printf("%s\tdeleted:    %s\n" COLOR_END, color, path1);
            i++;
        } else if (cmp_result > 0) {
            // Leader vs Index: Leader doesn't has the file but in index, means is staged new file
            // Index vs Dir: It is untracked file in index
            printf("%s\t%s:  %s\n" COLOR_END, color,
                   (group == LEADER_VS_INDEX ? "new file" : "untracked"), path2);
            j++;
        } else {
            // Both has the file, check the content is the same
            // If not, means this file is tmodified
            if (strcmp(hash1, hash2) != 0) {
                printf("%s\tmodified:   %s\n" COLOR_END, color, path1);
            }
            i++;
            j++;
        }
    }

    if (list1 != NULL) {
        xfree(list1);
    }
    if (list2 != NULL) {
        xfree(list2);
    }
}

void cmd_status(int argc, char *argv[]) {
    // argv is not need
    UNUSED(argv);

    // Check initalize first
    if (check_init() == NOT_INIT) {
        error_not_init();
    }

    // Make sure is only 'big status'
    if (argc > 1) {
        error_custom_msg("Usage: big status\n");
    }

    // Change working directory to project directory
    cd_to_project_root(NULL);

    // Read global index, project directory, and current bracnh's head commit(if is exist)
    snapshot_bst_t *leader_bst = NULL;
    snapshot_bst_t *index_bst = read_index_file();
    snapshot_bst_t *dir_bst = snapshot_bst_create_from_projectdir();
    char *leader_hash = load_leader();
    if (leader_hash != NULL) {
        char path[1024];
        snprintf(path, sizeof(path), ".big/objects/%s/list", leader_hash);
        leader_bst = read_index_file_from_path(path);
        xfree(leader_hash);
    } else {
        // Not exist just create empty BST
        leader_bst = snapshot_bst_create();
    }

    // Compare Leader and index for staged files
    printf("Ready to commit:\n");
    print_diff(leader_bst, index_bst, LEADER_VS_INDEX);
    puts("");

    // Compare index and project directory for unstaged files
    printf("Changes not staged:\n");
    print_diff(index_bst, dir_bst, INDEX_VS_DIR);
    puts("");

    snapshot_bst_free(&dir_bst);
    snapshot_bst_free(&index_bst);
    snapshot_bst_free(&leader_bst);
}