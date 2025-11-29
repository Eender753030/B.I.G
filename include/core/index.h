#ifndef INDEX_H
#define INDEX_H

#include "core/snapshot.h"

void process_path(snapshot_bst_t *bst, const char *root_path, snapshot_bst_t *leader_bst);

void save_index_file(snapshot_bst_t *bst);

snapshot_bst_t *read_index_file_from_path(const char *path);

snapshot_bst_t *read_index_file();

snapshot_bst_t *snapshot_bst_create_from_projectdir();

snapshot_bst_t *snapshot_bst_create_dir_path();

#endif