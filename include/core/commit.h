#ifndef COMMIT_H
#define COMMIT_H

#include "core/snapshot.h"

// Type of commit_node that does not provide structure detail for client
typedef struct commit_node commit_node_t;

// Get branch's hash that Leader(HEAD) current point to
char *load_leader();

// Get commit_node's parent information
commit_node_t *load_parent_info(char *commit_id, long *limit_amount);

// Create new commit_node with log
commit_node_t *commit_node_create(const char *log);

void commit_node_free(commit_node_t **node);

// Create commit object in .big/objects/ that contain metadata and index list
void save_commit_obj(commit_node_t *node);

// Update Leader(HEAD) point branch
void update_leader(const char *branch_name);

// Update branch's hash using hash
void update_branch_with_hash(const char *hash);

// Update branch's hash by passing commit_node
void update_branch(commit_node_t *node);

// Get branch's name that Leader(HEAD) current point to
char *load_current_branch();

// Get branch's hash by using path
char *load_ref_hash(const char *path);

// Getter of commit_node to get data
void get_commit_node_info(commit_node_t *node, char **log, char **datetime, char **hash);

// Getter of get commit_node's parent
commit_node_t *get_commit_parent(commit_node_t *node);

#endif