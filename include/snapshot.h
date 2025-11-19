#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <dirent.h>
#include <stdlib.h>

typedef struct FileInfo {
    char *content;
    char *path;  // Use path as search key
} FileInfo;

typedef struct SnapshotNode SnapshotNode;

typedef struct SnapshotBST SnapshotBST;

void SnapshotBSTDestory(SnapshotBST **bst);

SnapshotBST *read_index_file(size_t *total_size);

void add(const size_t input_size, const char **path_list);

FileInfo *get_fileinfo(SnapshotNode *node);

#endif