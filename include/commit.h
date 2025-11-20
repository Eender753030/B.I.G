#ifndef COMMIT_H
#define COMMIT_H

#include "snapshot.h"

typedef struct CommitGraph CommitGraph;

void scan_and_create_snapshot(SnapshotNode *node);

void commit(const char *log_message);

#endif