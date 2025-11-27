#ifndef COMMIT_H
#define COMMIT_H

#include <stdlib.h>

#include "core/snapshot.h"

typedef struct commit_node commit_node_t;

char *load_leader();

commit_node_t *load_parent_info(char *commit_id, long *limit_amount);

commit_node_t *commit_node_create(const char *log);

void commit_node_free(commit_node_t **node);

void save_commit_obj(commit_node_t *node);

void update_leader(commit_node_t *node);

#endif