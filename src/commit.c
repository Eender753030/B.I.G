#include "commit.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "error_handle.h"
#include "snapshot.h"

typedef struct CommitNode {
    SnapshotBST *snapshot;
    char *log;
    char *datetime;
    struct CommitNode **parent;
} CommitNode;

struct CommitGraph {
    CommitNode *nodes;
};
