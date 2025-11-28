#include "commands/cmd_add.h"

#include <errno.h>
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
    if (access(path, F_OK) != 0) {
        ErrorCustomMsg("Error: '%s' did not match to any file or directory.\n", path);
    }
    if (access(".big", F_OK) == 0 && strncmp(path, "..", 3) == 0) {
        ErrorCustomMsg("Error: '..' is outside project directory.\n");
    }
}

static void process_dir_or_file(const char *path, snapshot_bst_t *bst, struct stat *file_stat,
                                snapshot_bst_t *leader_bst) {
    if (strncmp(path, ".", 2) == 0) {
        process_path(bst, ".", leader_bst);
    } else {
        if (stat(path, file_stat) == -1)
            ErrnoHandler(__func__, __FILE__, __LINE__);

        if (S_ISDIR(file_stat->st_mode))
            process_path(bst, path, leader_bst);
        else {
            snapshot_bst_insert(bst, path, leader_bst);
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

    snapshot_bst_t *snapshot_bst = read_index_file();

    char *leader_hash = load_leader();
    snapshot_bst_t *leader_bst = NULL;
    if (leader_hash != NULL) {
        char leader_list_path[1024];
        snprintf(leader_list_path, sizeof(leader_list_path), ".big/objects/%s/list", leader_hash);

        leader_bst = read_index_file_from_path(leader_list_path);
        xfree(leader_hash);
    }

    struct stat file_stat;
    for (size_t i = 0; i < input_size; i++) {
        process_dir_or_file(root_path_list[i], snapshot_bst, &file_stat, leader_bst);
    }

    save_index_file(snapshot_bst);
    snapshot_bst_free(&leader_bst);
    snapshot_bst_free(&snapshot_bst);
}