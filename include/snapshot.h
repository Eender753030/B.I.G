#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <dirent.h>

typedef struct SnapshotBST SnapshotBST;

SnapshotBST *SnapshotBSTCreate(char **path_list, size_t list_len);

void SnapshotBSTDestory(SnapshotBST **bst);

#endif