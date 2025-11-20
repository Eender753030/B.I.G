#ifndef COMMIT_H
#define COMMIT_H

#include "snapshot.h"

typedef struct CommitNode CommitNode;

void commit(const char *log_message);

#endif