#include "commands/cmd_log.h"

#include <stdio.h>
#include <stdlib.h>

#include "core/commit_graph.h"
#include "utils/color.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

static inline void print_log(CommitNode *node) {
    printf(COLOR_BROWN "Commit: %s\t" COLOR_END "Date: %s\tLog: \"%s\"\n", node->commit_id,
           node->datetime, node->log);
}

void cmd_log(int argc, char *argv[]) {
    if (check_init() == -1)
        NotInitError();

    long amount;

    if (argc == 1)
        amount = -1;
    else if (argc == 2) {
        char *c = argv[1] + 1;
        if (*argv[1] == '-' && *c >= '0' && *c <= '9') {
            for (; *c != '\0'; c++) {
                if (*c < '0' || *c > '9')
                    ErrorCustomMsg("'%s' is not a positive integer\n", argv[1] + 1);
            }
            amount = strtol(argv[1] + 1, NULL, 10);
        } else
            ErrorCustomMsg("Usage: big log [-<amount>]\n");
    } else
        ErrorCustomMsg("Usage: big log [-<amount>]\n");

    char *leader_id = load_leader();
    if (leader_id == NULL)
        ErrorCustomMsg("No commit\n");

    CommitNode *leader_node = load_parent_info(leader_id);

    CommitNode *current_node = leader_node;
    if (amount == -1) {
        while (current_node != NULL) {
            print_log(current_node);
            if (current_node->parent == NULL)
                break;
            current_node = current_node->parent[0];
        }
    } else {
        for (long i = 0; i < amount; i++) {
            print_log(current_node);
            if (current_node->parent == NULL)
                break;
            current_node = current_node->parent[0];
        }
    }

    CommitNodeFree(&leader_node);
}