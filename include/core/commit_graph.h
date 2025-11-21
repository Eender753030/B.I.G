#ifndef COMMIT_GRAPH_H
#define COMMIT_GRAPH_H

typedef struct CommitNode CommitNode;

CommitNode *CommitNodeCreate(char *log);

void CommitNodeFree(CommitNode **node);

char *commit_log_insert(char *log_message);

void save_object_file(CommitNode *node);

void leader_update(CommitNode *node);

#endif