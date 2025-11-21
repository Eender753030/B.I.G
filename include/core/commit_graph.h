#ifndef COMMIT_GRAPH_H
#define COMMIT_GRAPH_H

#include <stdlib.h>

#include "core/snapshot.h"

typedef struct CommitNode {
    SnapshotBST *snapshot;
    char *log;
    char *datetime;
    struct CommitNode **parent;
    size_t parent_num;
    char *commit_id;
} CommitNode;

char *load_leader();

CommitNode *load_parent_info(char *commit_id);

CommitNode *CommitNodeCreate(char *log);

void CommitNodeFree(CommitNode **node);

char *commit_log_insert(char *log_message);

void save_object_file(CommitNode *node);

void leader_update(CommitNode *node);

#endif