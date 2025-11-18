#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <dirent.h>

typedef struct SnapshotBST SnapshotBST;

void SnapshotBSTDestory(SnapshotBST **bst);

SnapshotBST *read_index_file(size_t *total_size);

void add(const size_t input_size, const char **path_list);

#endif