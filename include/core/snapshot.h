#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stdbool.h>
#include <stdlib.h>

#include "ds/bst.h"

typedef struct file_info file_info_t;

typedef bst_node_t snapshot_node_t;

typedef bst_t snapshot_bst_t;

file_info_t *file_info_create_from_index(const char *path, const char *hash, bool is_changed);

snapshot_bst_t *snapshot_bst_create();

snapshot_bst_t *snapshot_bst_create_from_list(file_info_t **list, uint64_t size);

void snapshot_bst_insert(snapshot_bst_t *self, const char *path, snapshot_bst_t *leader_bst);

void snapshot_bst_insert_projectdir(snapshot_bst_t *self, const char *path);

void snapshot_bst_insert_only_path(snapshot_bst_t *self, const char *path);

void snapshot_bst_delete(snapshot_bst_t *self, const char *path);

void snapshot_bst_free(snapshot_bst_t **bst);

void file_info_get_content(file_info_t *file_info, char **path, char **hash, bool *is_changed);

bool is_same_file_info(void *file_info1, void *file_info2);

bool is_snapshot_bst_contains(snapshot_bst_t *self, const char *path);

#endif