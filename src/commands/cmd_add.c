#include "commands/cmd_add.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/snapshot.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/utils.h"

static void check_add_files(const char *path) {
    if (access(path, F_OK) == -1) {
        ErrorCustomMsg("Error: '%s' did not match to any file or directory.\n", path);
    }

    if (access(".big", F_OK) == 0 && strncmp(path, "..", 3) == 0) {
        ErrorCustomMsg("Error: '..' is outside project directory.\n");
    }
}

static void process_dir_or_file(const char *path, size_t *total_size, SnapshotBST **bst,
                                struct stat *file_stat) {
    if (strncmp(path, ".", 2) == 0)
        process_path(bst, ".", total_size);
    else {
        if (stat(path, file_stat) == -1)
            ErrnoHandler(__func__, __FILE__, __LINE__);

        if (S_ISDIR(file_stat->st_mode))
            process_path(bst, path, total_size);
        else {
            if (SnapshotBSTInsert(bst, path) == 0)
                (*total_size)++;
        }
    }
}

void cmd_add(int argc, char *argv[]) {
    if (check_init() == -1) {
        NotInitError();
    }
    if (argc < 2) {
        ErrorCustomMsg(
            "Usage: big add <filename or directory> <...>\n"
            "Use 'big add .' in root of project directory to add whole\n");
    }

    size_t input_size = argc - 1;
    char **root_path_list = argv + 1;

    for (size_t i = 0; i < input_size; i++) {
        check_add_files(root_path_list[i]);
    }

    char *org_dir;
    cd_to_project_root(&org_dir);

    size_t total_size = 0;

    SnapshotBST *bst = read_index_file(&total_size);

    struct stat file_stat;
    for (size_t i = 0; i < input_size; i++) {
        char *normalized_path = relative_path_calc(org_dir, root_path_list[i]);

        process_dir_or_file(normalized_path, &total_size, &bst, &file_stat);

        free(normalized_path);
        normalized_path = NULL;
    }

    save_index_file(bst, total_size);

    free(org_dir);
    org_dir = NULL;
    SnapshotBSTDestory(&bst);
}