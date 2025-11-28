#include "core/index.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "core/snapshot.h"
#include "ds/bst.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"

void process_path(snapshot_bst_t *bst, const char *root_path, snapshot_bst_t *leader_bst) {
    DIR *dir = opendir(root_path);
    if (dir == NULL) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    struct dirent *file_dirent;
    struct stat file_stat;

    while ((file_dirent = readdir(dir)) != NULL) {
        if (strcmp(file_dirent->d_name, ".") == 0 || strcmp(file_dirent->d_name, "..") == 0 ||
            strcmp(file_dirent->d_name, ".big") == 0 || strcmp(file_dirent->d_name, "big") == 0) {
            continue;
        }
        char pathbuffer[1024];
        char *c;

        if (strcmp(root_path, ".") == 0) {
            snprintf(pathbuffer, sizeof(pathbuffer), "%s", file_dirent->d_name);
        } else if ((c = strrchr(root_path, '/')) != NULL && *(c + 1) == '\0') {
            snprintf(pathbuffer, sizeof(pathbuffer), "%s%s", root_path, file_dirent->d_name);

        } else {
            snprintf(pathbuffer, sizeof(pathbuffer), "%s/%s", root_path, file_dirent->d_name);
        }

        if (stat(pathbuffer, &file_stat) == -1) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }

        if (S_ISDIR(file_stat.st_mode)) {
            process_path(bst, pathbuffer, leader_bst);
        } else {
            snapshot_bst_insert(bst, pathbuffer, leader_bst);
        }
    }

    xfree(dir);
}

static void write_index(void *file_info, void *file_t) {
    char *path, *hash;
    bool is_changed;
    file_info_get_content(file_info, &path, &hash, &is_changed);
    if (fprintf(file_t, "%s\t%s\t%d\n", path, hash, is_changed) == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
}

void save_index_file(snapshot_bst_t *bst) {
    if (bst == NULL) {
        return;
    }

    uint64_t bst_amount = bst_get_amount(bst);
    if (bst_amount == 0) {
        return;
    }

    FILE *index_file = xfopen(".big/index", "w");

    if (fprintf(index_file, "%lu\n", bst_amount) == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    bst_inorder_func(bst, write_index, index_file);

    fclose(index_file);
}

snapshot_bst_t *read_index_file_from_path(const char *path) {
    FILE *index_file = fopen(path, "r");
    if (index_file == NULL) {
        return snapshot_bst_create();
    }

    uint64_t total_size;
    if (fscanf(index_file, "%lu\n", &total_size) == -1) {
        fclose(index_file);
        return snapshot_bst_create();
    }

    file_info_t **file_info_list = xmalloc(sizeof(*file_info_list) * total_size);

    uint64_t idx = 0;
    char path_buffer[960];
    char hash_buffer[64];
    int is_changed_temp;
    while (idx < total_size && fscanf(index_file, "%959[^\t]\t%s\t%d\n", path_buffer, hash_buffer,
                                      &is_changed_temp) == 3) {
        file_info_list[idx++] =
            file_info_create_from_index(path_buffer, hash_buffer, is_changed_temp != 0);
    }

    if (idx != total_size) {
        fprintf(stderr, "Warning: index file mismatch\n");
    }

    fclose(index_file);

    snapshot_bst_t *new_bst = snapshot_bst_create_from_list(file_info_list, total_size);

    xfree(file_info_list);

    return new_bst;
}

snapshot_bst_t *read_index_file() {
    return read_index_file_from_path(".big/index");
}