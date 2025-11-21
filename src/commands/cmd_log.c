#include "commands/cmd_log.h"

#include <stdio.h>
#include <stdlib.h>

#include "core/commit_graph.h"
#include "utils/error_handle.h"

void cmd_log(const long *amount) {
    char *leader_id = load_leader();
    if (leader_id == NULL)
        ErrorCustomMsg("No commit\n");

    CommitNode *leader_node = load_parent_info(leader_id);

    CommitNode *current_node = leader_node;
    if (amount == NULL) {
        while (current_node != NULL) {
            printf("Commit: %s  Date: %s  Log: %s\n", current_node->commit_id,
                   current_node->datetime, current_node->log);
            if (current_node->parent == NULL)
                break;
            current_node = current_node->parent[0];
        }
    } else {
        for (long i = 0; i < *amount; i++) {
            printf("Commit: %s  Date: %s  Log: %s\n", current_node->commit_id,
                   current_node->datetime, current_node->log);
            if (current_node->parent == NULL)
                break;
            current_node = current_node->parent[0];
        }
    }

    CommitNodeFree(&leader_node);
}