#include "commit.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "error_handle.h"
#include "snapshot.h"
#include "utils.h"

typedef struct CommitNode {
    SnapshotBST *snapshot;
    char *log;
    char *datetime;
    struct CommitNode **parent;
} CommitNode;

struct CommitGraph {
    CommitNode *nodes;
};

static CommitNode *CommitNodeCreate(const char *log) {
    CommitNode *new_node = (CommitNode *)malloc(sizeof(CommitNode));
    if (new_node == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    size_t size = 0;
    char buffer[100];
    struct tm *datetime_now;
    time_t time_now = time(NULL);

    datetime_now = localtime(&time_now);
    strftime(buffer, 100, "%Y/%m/%d %H:%M:%S", datetime_now);

    new_node->log = str_dup(log);
    new_node->datetime = str_dup(buffer);
    new_node->parent = NULL;
    new_node->snapshot = read_index_file(&size);

    return new_node;
}

static void CommitNodeFree(CommitNode *node) {
    free(node->log);
    node->log = NULL;
    free(node->datetime);
    node->datetime = NULL;
    SnapshotBSTDestory(&(node->snapshot));
}

static void CommitGraphDestory(CommitGraph **graph) {
    // TODO: Free all Graph. Use with CommitNodeFree
}
