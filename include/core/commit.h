#ifndef COMMIT_H
#define COMMIT_H

#include "core/snapshot.h"

typedef struct commit_node commit_node_t;

char *load_leader();

commit_node_t *load_parent_info(char *commit_id, long *limit_amount);

commit_node_t *commit_node_create(const char *log);

void commit_node_free(commit_node_t **node);

void save_commit_obj(commit_node_t *node);

void update_leader(const char *branch_name);

void update_branch_with_hash(const char *hash);

void update_branch(commit_node_t *node);

char *load_current_branch();

char *load_ref_hash(const char *path);

void get_commit_node_info(commit_node_t *node, char **log, char **datetime, char **hash);

commit_node_t *get_commit_parent(commit_node_t *node);

#endif