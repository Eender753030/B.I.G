#include "commands/cmd_checkout.h"

#include <dirent.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core/blob.h"
#include "core/commit.h"
#include "core/index.h"
#include "core/snapshot.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

// Check the input target is branch or not
// Traversal the refs/ to find it
static bool check_is_branch(const char *input) {
    DIR *ref_dir = opendir(".big/refs");
    if (ref_dir == NULL) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    struct dirent *ref;
    while ((ref = readdir(ref_dir)) != NULL) {
        if (strcmp(ref->d_name, ".") == 0 || strcmp(ref->d_name, "..") == 0) {
            continue;
        }

        if (strcmp(ref->d_name, input) == 0) {
            closedir(ref_dir);
            return true;
        }
    }
    closedir(ref_dir);
    return false;
}

// Helper for make sure two BSTs have all same data but maybe different order
static bool search_and_cmp_is_same(snapshot_bst_t *bst1, snapshot_bst_t *bst2) {
    uint64_t bst1_amount = bst_get_amount(bst1);
    uint64_t bst2_amount = bst_get_amount(bst2);

    if (bst1_amount != bst2_amount) {
        return false;
    }

    // Change one to list, O(n)
    file_info_t **file_info_list = (file_info_t **)bst_inorder_to_list(bst2);

    // Search all bst2's data is in bst1, This is worst O(mlog n)
    for (uint64_t i = 0; i < bst2_amount; i++) {
        snapshot_node_t *target = bst_search(bst1, file_info_list[i]);

        // Not found means not the same
        if (target == NULL) {
            xfree(file_info_list);
            return false;
        }
        char *hash1, *hash2;

        file_info_t *temp_file_info = bst_node_get_data(target);
        file_info_get_content(temp_file_info, NULL, &hash1, NULL);
        file_info_get_content(file_info_list[i], NULL, &hash2, NULL);

        // Check file content hash is the same
        if (strcmp(hash1, hash2) != 0) {
            xfree(file_info_list);
            return false;
        }
    }

    xfree(file_info_list);

    return true;
}

// Helper callback function to delete the file
// Use in inorder_func
static void delete_files(void *data, void *args) {
    UNUSED(args);

    file_info_t *file_info = (file_info_t *)data;
    char *path;
    file_info_get_content(file_info, &path, NULL, NULL);

    // Delete target file
    remove(path);
}

// Helper callback function to restore all files and directory of target commit
// Use in inorder_func
static void restore_files(void *data, void *args) {
    UNUSED(args);

    file_info_t *file_info = (file_info_t *)data;
    char *path, *hash;
    file_info_get_content(file_info, &path, &hash, NULL);

    uint64_t content_len = 0;
    // Get the file's content
    char *content = blob_read_from_hash(hash, &content_len);
    // Write the file with it's content
    mk_dir_and_file(path, content, content_len);

    xfree(content);
}

// Delete the directory from directory path only BST
static void delete_dirs(snapshot_bst_t *dir_bst) {
    uint64_t bst_amount = bst_get_amount(dir_bst);
    if (bst_amount == 0) {
        return;
    }
    // Use sorted list of directory path
    file_info_t **file_info_list = (file_info_t **)bst_inorder_to_list(dir_bst);

    // Start from end beacuse the path is longer
    // e.g src/core and src will delete src/core first because it is bigger
    for (uint64_t i = bst_amount; i > 0; i--) {
        char *dir_path;
        file_info_get_content(file_info_list[i - 1], &dir_path, NULL, NULL);
        remove(dir_path);
    }

    xfree(file_info_list);
}

// Create a temp branch for checkout to history commit
static void create_temp_ref(const char *target_hash) {
    char temp_ref_path[] = ".big/refs/temp_checkout_ref";

    FILE *temp_ref = xfopen(temp_ref_path, "w");

    if (fprintf(temp_ref, "%s\n", target_hash) < 0) {
        fclose(temp_ref);
        errno_handle(__func__, __FILE__, __LINE__);
    }
    fclose(temp_ref);
}

void cmd_checkout(int argc, char *argv[]) {
    // Check initalize first
    if (check_init() == NOT_INIT) {
        error_not_init();
    }
    // Require arg
    if (argc < 2) {
        error_custom_msg("Usage: big checkout <commit hash or branch name>\n");
    }
    // Only one arg
    if (argc > 2) {
        error_custom_msg("Error: Enter only one target commit hash or branch name\n");
    }

    // Back to project directory and save working directory first
    cd_to_project_root(NULL);

    // Check input targe is branch or not
    bool branch_mode = check_is_branch(argv[1]);

    // If change to current branch, stop the operation
    if (branch_mode == true) {
        char *cuurent_branch = load_current_branch();
        if (cuurent_branch == NULL) {
            xfree(cuurent_branch);
            error_custom_msg("No commit\n");
        }
        if (strcmp(cuurent_branch, argv[1]) == 0) {
            xfree(cuurent_branch);
            error_custom_msg("Error: Already in branch '%s'\n", argv[1]);
        }
        xfree(cuurent_branch);
    }

    char *target_hash;
    // If is branch, load the hash in branch file
    if (branch_mode == true) {
        char branch_path[4096];
        snprintf(branch_path, sizeof(branch_path), ".big/refs/%s", argv[1]);
        target_hash = load_ref_hash(branch_path);
    }
    // Not branch means input is hash or else
    else {
        target_hash = argv[1];
    }

    char target_list_path[1024];
    // Check the hash is exist or not
    snprintf(target_list_path, sizeof(target_list_path), ".big/objects/%s/list", target_hash);
    if (access(target_list_path, F_OK) != 0) {
        if (branch_mode == true) {
            xfree(target_hash);
        }
        error_custom_msg("Error: Target commit or branch '%s' does not exist\n", target_hash);
    }

    char *leader_hash = load_leader();
    // No commit can not checkout
    if (leader_hash == NULL) {
        if (branch_mode == true) {
            xfree(target_hash);
        }
        error_custom_msg("No commit\n");
    }

    char leader_list_path[1024];
    snprintf(leader_list_path, sizeof(leader_list_path), ".big/objects/%s/list", leader_hash);

    snapshot_bst_t *dir_bst = snapshot_bst_create_from_projectdir();
    snapshot_bst_t *index_bst = read_index_file();
    snapshot_bst_t *leader_bst = read_index_file_from_path(leader_list_path);
    // Compare three BST is the same otherwise for safety, checkout operation is not allowed
    if (is_same_bst(leader_bst, index_bst, is_same_file_info) == false ||
        search_and_cmp_is_same(index_bst, dir_bst) == false) {
        xfree(leader_hash);
        if (branch_mode == true) {
            xfree(target_hash);
        }
        snapshot_bst_free(&leader_bst);
        snapshot_bst_free(&index_bst);
        snapshot_bst_free(&dir_bst);
        error_custom_msg(
            "Error: There are changes not commit. Please commit first or discard changes\n");
    }

    // Check is current hash
    if (strcmp(leader_hash, target_hash) == 0) {
        xfree(leader_hash);
        xfree(target_hash);
        snapshot_bst_free(&leader_bst);
        snapshot_bst_free(&index_bst);
        snapshot_bst_free(&dir_bst);

        // If is branch mode, just change the Leader point
        if (branch_mode == true) {
            update_leader(argv[1]);
            printf("Change to branch: %s\n", argv[1]);
            return;
        } else {
            error_custom_msg("Error: Already in '%s'\n", target_hash);
        }
    }
    xfree(leader_hash);

    snapshot_bst_t *dir_path_bst = snapshot_bst_create_dir_path();
    // Delete whole files and directories in project directory
    bst_inorder_func(index_bst, delete_files, NULL);
    delete_dirs(dir_path_bst);

    snapshot_bst_t *target_commit_bst = read_index_file_from_path(target_list_path);
    // Restore all files and directories record in target hash
    bst_inorder_func(target_commit_bst, restore_files, NULL);

    // If target is branch, update Leader point to target branch
    if (branch_mode == true) {
        update_leader(argv[1]);
        xfree(target_hash);
        printf("Change to branch: %s\n", argv[1]);
    }
    // If target is hash, create a temp branch and make Leader point to it
    else {
        create_temp_ref(target_hash);
        update_leader("temp_checkout_ref");
    }
    // Update the global index file for target hash or branch
    save_index_file(target_commit_bst);

    snapshot_bst_free(&target_commit_bst);
    snapshot_bst_free(&dir_path_bst);
    snapshot_bst_free(&leader_bst);
    snapshot_bst_free(&index_bst);
    snapshot_bst_free(&dir_bst);
}