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

void cmd_add(size_t input_size, const char **root_path_list) {
    for (size_t i = 0; i < input_size; i++) {
        if (access(root_path_list[i], F_OK) == -1) {
            ErrorCustomMsg("Error: '%s' did not match to any file or directory.\n",
                           root_path_list[i]);
        }
        if (access(".big", F_OK) == 0 && strncmp(root_path_list[i], "..", 3) == 0)
            ErrorCustomMsg("Error: '..' is outside project directory.\n");
    }

    char *org_dir;
    cd_to_project_root(&org_dir);

    size_t total_size = 0;

    SnapshotBST *bst = read_index_file(&total_size);

    struct stat *file_stat = (struct stat *)malloc(sizeof(struct stat));
    if (file_stat == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    for (size_t i = 0; i < input_size; i++) {
        char *normalized_path = relative_path_calc(org_dir, root_path_list[i]);

        if (strncmp(normalized_path, ".", 2) == 0)
            process_path(&bst, ".", &total_size);
        else {
            if (stat(normalized_path, file_stat) == -1)
                ErrnoHandler(__func__, __FILE__, __LINE__);

            if (S_ISDIR(file_stat->st_mode))
                process_path(&bst, normalized_path, &total_size);
            else {
                if (SnapshotBSTInsert(&bst, normalized_path) == 0)
                    total_size++;
            }
        }

        free(normalized_path);
    }

    save_index_file(bst, total_size);

    free(org_dir);

    free(file_stat);
    file_stat = NULL;
    SnapshotBSTDestory(&bst);
}