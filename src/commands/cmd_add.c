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

static void check_add_files(const char *path) {
    if (access(".big", F_OK) == 0 && strcmp(path, "..") == 0) {
        ErrorCustomMsg("Error: '..' is outside project directory.\n");
    }
}

static void delete_file_in_dir(const char *dir_path, snapshot_bst_t *bst, bool delete_mode) {
    uint64_t bst_amount = bst_get_amount(bst);
    if (bst_amount == 0) {
        return;
    }

    file_info_t **file_info_list = (file_info_t **)bst_inorder_to_list(bst);

    char **delete_list = xmalloc(sizeof(*delete_list) * bst_amount);
    uint16_t delete_count = 0;

    for (uint64_t i = 0; i < bst_amount; i++) {
        char *file_info_path;
        file_info_get_content(file_info_list[i], &file_info_path, NULL, NULL);

        bool is_in_dir = false;

        if (strcmp(dir_path, ".") == 0) {
            is_in_dir = true;
        } else {
            if (strncmp(file_info_path, dir_path, strlen(dir_path)) == 0) {
                is_in_dir = true;
            }
        }

        if (is_in_dir == true) {
            if (delete_mode == false) {
                if (access(file_info_path, F_OK) != 0) {
                    delete_list[delete_count++] = str_dup(file_info_path);
                }
            } else {
                delete_list[delete_count++] = str_dup(file_info_path);
            }
        }
    }
    xfree(file_info_list);

    for (uint16_t i = 0; i < delete_count; i++) {
        snapshot_bst_delete(bst, delete_list[i]);
        xfree(delete_list[i]);
    }
    xfree(delete_list);
}

static void process_dir_or_file(const char *path, snapshot_bst_t *bst, struct stat *file_stat,
                                snapshot_bst_t *leader_bst) {
    if (stat(path, file_stat) == 0) {
        if (S_ISDIR(file_stat->st_mode)) {
            process_path(bst, path, leader_bst);
            delete_file_in_dir(path, bst, false);
        } else {
            snapshot_bst_insert(bst, path, leader_bst);
        }
    } else {
        if (is_snapshot_bst_contains(bst, path) == true) {
            snapshot_bst_delete(bst, path);
        } else {
            WarningCustomMsg("Warning: '%s' matches no file or directory.\n", path);
        }
    }
}

void cmd_add(int argc, char *argv[]) {
    if (check_init() == NOT_INIT) {
        NotInitError();
    }
    if (argc < 2) {
        ErrorCustomMsg(
            "Usage: big add <file or directory> ...\n"
            "Use 'big add .' in root of project directory to add whole\n");
    }

    bool delete_mode = false;
    int start_index = 1;

    if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--delete") == 0) {
        delete_mode = true;
        start_index = 2;
        if (argc < 3) {
            ErrorCustomMsg("Usage: big add -d <file or directory> ...\n");
        }
    }

    uint64_t input_size = (uint64_t)(argc - start_index);
    char **root_path_list = argv + start_index;

    for (uint64_t i = 0; i < input_size; i++) {
        check_add_files(root_path_list[i]);
    }

    char *org_dir = NULL;
    cd_to_project_root(&org_dir);

    snapshot_bst_t *snapshot_bst = read_index_file();

    char *leader_hash = NULL;
    snapshot_bst_t *leader_bst = NULL;
    if (delete_mode == false && (leader_hash = load_leader()) != NULL) {
        char leader_list_path[1024];
        snprintf(leader_list_path, sizeof(leader_list_path), ".big/objects/%s/list", leader_hash);

        leader_bst = read_index_file_from_path(leader_list_path);
        xfree(leader_hash);
    }

    struct stat file_stat;
    for (uint64_t i = 0; i < input_size; i++) {
        char *normalized_path = relative_path_calc(org_dir, root_path_list[i]);
        if (delete_mode == true) {
            delete_file_in_dir(normalized_path, snapshot_bst, true);
        } else {
            process_dir_or_file(normalized_path, snapshot_bst, &file_stat, leader_bst);
        }
        xfree(normalized_path);
    }

    save_index_file(snapshot_bst);

    xfree(org_dir);
    snapshot_bst_free(&leader_bst);
    snapshot_bst_free(&snapshot_bst);
}