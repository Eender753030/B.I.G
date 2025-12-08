#include "commands/cmd_add.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/commit.h"
#include "core/index.h"
#include "core/snapshot.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

// Helper function for check if user input ".." whether in project directory
static void check_add_files(const char *path) {
    if (access(".big", F_OK) == 0 && strcmp(path, "..") == 0) {
        error_custom_msg("Error: '..' is outside project directory.\n");
    }
}

// Helper function to delete target directory's files in index
static void delete_file_in_dir(const char *dir_path, snapshot_bst_t *bst, bool delete_mode) {
    uint64_t bst_amount = bst_get_amount(bst);
    if (bst_amount == 0) {
        return;
    }

    // Get a sorted list from BST
    file_info_t **file_info_list = (file_info_t **)bst_inorder_to_list(bst);

    // Perpare a delete list max amount same as BST
    char **delete_list = xmalloc(sizeof(*delete_list) * bst_amount);
    uint64_t delete_count = 0;
    uint64_t dir_len = strlen(dir_path);

    for (uint64_t i = 0; i < bst_amount; i++) {
        char *file_info_path;
        file_info_get_content(file_info_list[i], &file_info_path, NULL, NULL);

        bool is_in_dir = false;

        // If path is "." means current directory
        if (strcmp(dir_path, ".") == 0) {
            is_in_dir = true;
        } else {
            // Compare file and directory name with length of directory name
            // If is the same, means file in this directory
            if (strncmp(file_info_path, dir_path, dir_len) == 0) {
                // Make sure is end with '/'
                if (file_info_path[dir_len] == '\0' || file_info_path[dir_len] == '/') {
                    is_in_dir = true;
                }
            }
        }

        if (is_in_dir == true) {
            if (delete_mode == false) {
                // if is not delete mode and file not exist
                // Means file is delete in project directory
                // Need to remove it from index
                if (access(file_info_path, F_OK) != 0) {
                    delete_list[delete_count++] = str_dup(file_info_path);
                }
            }
            // This is for force delete file in global index
            else {
                delete_list[delete_count++] = str_dup(file_info_path);
            }
        }
    }
    xfree(file_info_list);

    // Start to delete path node in delete list
    for (uint16_t i = 0; i < delete_count; i++) {
        snapshot_bst_delete(bst, delete_list[i]);
        xfree(delete_list[i]);
    }
    xfree(delete_list);
}

// Helper function for parse user input is directory or file
static void process_dir_or_file(const char *path, snapshot_bst_t *bst, struct stat *file_stat,
                                snapshot_bst_t *leader_bst) {
    if (stat(path, file_stat) == 0) {
        if (S_ISDIR(file_stat->st_mode)) {
            // If is directory go deeper
            process_path(bst, path, leader_bst);
            // And check there is deleted file or not
            delete_file_in_dir(path, bst, false);
        } else {
            // Insert file to BST
            snapshot_bst_insert(bst, path, leader_bst);
        }
    }
    // File not exist
    else {
        // Check is in BST or not
        if (is_snapshot_bst_contains(bst, path) == true) {
            // If true, delete that node
            snapshot_bst_delete(bst, path);
        } else {
            // invalid input file name just warning
            warning_custom_msg("Warning: '%s' matches no file or directory.\n", path);
        }
    }
}

void cmd_add(int argc, char *argv[]) {
    // Check initalize first
    if (check_init() == NOT_INIT) {
        error_not_init();
    }
    // Not enough args for 'big add'
    if (argc < 2) {
        error_custom_msg(
            "Usage: big add <file or directory> ...\n"
            "Use 'big add .' in root of project directory to add whole\n");
    }

    bool delete_mode = false;
    int start_index = 1;

    // If it has option -d | --delete means force delete file in index
    if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--delete") == 0) {
        // delete mode on
        delete_mode = true;
        start_index = 2;  // skip option input
        if (argc < 3) {
            error_custom_msg("Usage: big add -d <file or directory> ...\n");
        }
    }

    uint64_t input_size = (uint64_t)(argc - start_index);
    // Input files or directories
    char **root_path_list = argv + start_index;

    // Check ".."
    for (uint64_t i = 0; i < input_size; i++) {
        check_add_files(root_path_list[i]);
    }

    char *org_dir = NULL;
    // Back to project directory and save working directory first
    cd_to_project_root(&org_dir);

    // Get a builded BST from index file
    // This is a balanced BST (empty BST for first time)
    snapshot_bst_t *snapshot_bst = read_index_file();

    char *leader_hash = NULL;
    snapshot_bst_t *leader_bst = NULL;
    // If Leader(HEAD) is exist, get a builded BST from current branch's head commit index list
    if (delete_mode == false && (leader_hash = load_leader()) != NULL) {
        char leader_list_path[1024];
        snprintf(leader_list_path, sizeof(leader_list_path), ".big/objects/%s/list", leader_hash);

        leader_bst = read_index_file_from_path(leader_list_path);
        xfree(leader_hash);
    }

    // Main process of user's input
    struct stat file_stat;
    for (uint64_t i = 0; i < input_size; i++) {
        // Get the relative path of file to project directory
        char *normalized_path = relative_path_calc(org_dir, root_path_list[i]);
        if (delete_mode == true) {
            // delete mode for 'big add -d'
            delete_file_in_dir(normalized_path, snapshot_bst, true);
        } else {
            // Normal add file
            process_dir_or_file(normalized_path, snapshot_bst, &file_stat, leader_bst);
        }
        xfree(normalized_path);
    }

    // Save BST data to index file
    save_index_file(snapshot_bst);

    xfree(org_dir);
    snapshot_bst_free(&leader_bst);
    snapshot_bst_free(&snapshot_bst);
}