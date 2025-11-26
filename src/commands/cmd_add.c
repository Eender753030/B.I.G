#include "commands/cmd_add.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/snapshot.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

static void check_add_files(const char *path) {
    if (access(".big", F_OK) == 0 && strncmp(path, "..", 3) == 0) {
        ErrorCustomMsg("Error: '..' is outside project directory.\n");
    }
}

static void process_dir_or_file(const char *path, SnapshotBST *bst, struct stat *file_stat,
                                SnapshotBST *leader_bst, const char *leader_id) {
    if (strncmp(path, ".", 2) == 0) {
        process_path(bst, ".", leader_bst, leader_id);
    } else {
        if (stat(path, file_stat) == -1)
            ErrnoHandler(__func__, __FILE__, __LINE__);

        if (S_ISDIR(file_stat->st_mode))
            process_path(bst, path, leader_bst, leader_id);
        else {
            SnapshotBSTInsert(bst, path, leader_bst, leader_id);
        }
    }
}

void cmd_add(int argc, char *argv[]) {
    if (check_init() == NOT_INIT) {
        NotInitError();
    }
    if (argc < 2) {
        ErrorCustomMsg(
            "Usage: big add <filename or directory> <...>\n"
            "Use 'big add .' in root of project directory to add whole\n");
    }

    size_t input_size = (size_t)(argc - 1);
    char **root_path_list = argv + 1;

    for (size_t i = 0; i < input_size; i++) {
        check_add_files(root_path_list[i]);
    }

    char *org_dir;
    cd_to_project_root(&org_dir);

    SnapshotBST *bst = read_index_dic(NULL, NULL);

    struct stat file_stat;
    for (size_t i = 0; i < input_size; i++) {
        char index_dir[4096];
        snprintf(index_dir, sizeof(index_dir), ".big/index/root/%s", root_path_list[i]);
        if (access(index_dir, F_OK) == 0 && access(root_path_list[i], F_OK) != 0) {
            remove(index_dir);
            SnapshotBSTDelete(bst, root_path_list[i]);
            continue;
        } else {
            ErrorCustomMsg("Error: '%s' did not match to any file or directory.\n",
                           root_path_list[i]);
        }
        char *normalized_path = relative_path_calc(org_dir, root_path_list[i]);

        if (access(normalized_path, F_OK) == 0) {
            process_dir_or_file(normalized_path, bst, &file_stat, NULL, NULL);
        } else {
            ErrorCustomMsg("Error: '%s' did not match to any file or directory.\n",
                           root_path_list[i]);
        }
        free(normalized_path);
        normalized_path = NULL;
    }

    save_index_dic(bst);
    xfree(org_dir);
    SnapshotBSTDestory(&bst);
}