#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stdbool.h>
#include <stdlib.h>

#define MATCH false
#define NOT_MATCH true

typedef struct SnapshotNode SnapshotNode;

typedef struct SnapshotBST SnapshotBST;

SnapshotBST *SnapshotBSTCreateEmpty();

void SnapshotBSTInsert(SnapshotBST *bst, const char *path, SnapshotBST *leader_bst,
                       const char *leader_id);

SnapshotNode *SnapshotBSTSearch(SnapshotBST *bst, const char *path);

bool SnapshotBST_Search_and_Compare(SnapshotBST *bst, const char *path, const char *content);

void SnapshotBSTDelete(SnapshotBST *bst, const char *target_path);

void SnapshotBSTDestory(SnapshotBST **bst);

void process_path(SnapshotBST *bst, const char *root_path, SnapshotBST *leader_bst,
                  const char *leader_id);

SnapshotBST *read_index_dic(SnapshotBST *leader_bst, const char *leader_id);

void save_index_dic(SnapshotBST *bst);

void inorder_traversal_func(SnapshotBST *bst, void (*action)(SnapshotNode *));

void inorder_traversal_print(SnapshotBST *bst, const char *msg, const char *color);

void inorder_traversal_delete(SnapshotBST *target_bst, SnapshotBST *ref_bst);

SnapshotBST *read_leader_commit_BST(char **leader_id);

size_t amount_of_BST(SnapshotBST *bst);

#endif