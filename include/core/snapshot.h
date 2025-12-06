#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stdbool.h>
#include <stdlib.h>

#include "ds/bst.h"

// Type of file_info that does not provide structure detail for client
typedef struct file_info file_info_t;

// Using snapshot_node_t to repersent bst_node_t from bst.h for specialized store snapshot data
typedef bst_node_t snapshot_node_t;

// Using snapshot_bst_t to repersent bst_t from bst.h for specialized is snapshot BST
typedef bst_t snapshot_bst_t;

// Direct create file_info by data
file_info_t *file_info_create_from_index(const char *path, const char *hash, bool is_changed);

// Create a empty snapshot BST
snapshot_bst_t *snapshot_bst_create();

// Create a balance snapshot BST from sorted list
snapshot_bst_t *snapshot_bst_create_from_list(file_info_t **list, uint64_t size);

// Insertion for snapshot BST that may have leader(HEAD) information
void snapshot_bst_insert(snapshot_bst_t *self, const char *path, snapshot_bst_t *leader_bst);

// Insertion for snapshot BST that store project directory datas
void snapshot_bst_insert_projectdir(snapshot_bst_t *self, const char *path);

// Insertion for snapshot BST that store directorys' path
void snapshot_bst_insert_only_path(snapshot_bst_t *self, const char *path);

// Deletion for snapshot BST
void snapshot_bst_delete(snapshot_bst_t *self, const char *path);

// Free all snapshot BST
void snapshot_bst_free(snapshot_bst_t **bst);

// Getter for file_info's data
void file_info_get_content(file_info_t *file_info, char **path, char **hash, bool *is_changed);

/* Check two file_info is the same
 * Function for is_same_bst to comparsion
 */
bool is_same_file_info(void *file_info1, void *file_info2);

// Check path whether in the snapshot BST
bool is_snapshot_bst_contains(snapshot_bst_t *self, const char *path);

#endif