#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stdlib.h>

#define FOUND 0
#define NOT_FOUND -1

typedef struct SnapshotNode SnapshotNode;

typedef struct SnapshotBST SnapshotBST;

SnapshotBST *SnapshotBSTCreateEmpty();

int SnapshotBSTInsert(SnapshotBST *bst, const char *path);

int SnapshotBSTDelete(SnapshotBST *bst, const char *target_path, size_t *total_size);

void SnapshotBSTDestory(SnapshotBST **bst);

void process_path(SnapshotBST *bst, const char *root_path, size_t *list_length);

SnapshotBST *read_index_dic(size_t *total_size);

void save_index_dic(SnapshotBST *bst, size_t total_size);

void inorder_traversal_func(SnapshotBST *bst, void (*action)(SnapshotNode *));

void inorder_traversal_print(SnapshotBST *bst, const char *msg, const char *color);

void inorder_traversal_delete(SnapshotBST *target_bst, SnapshotBST *ref_bst,
                              size_t *target_total_size);

#endif