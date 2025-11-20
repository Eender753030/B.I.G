#ifndef COMMIT_H
#define COMMIT_H

#include "snapshot.h"

typedef struct CommitGraph CommitGraph;

void commit(const char *log_message);

#endif