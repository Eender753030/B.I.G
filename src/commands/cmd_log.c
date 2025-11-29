#include "commands/cmd_log.h"

#include <stdio.h>
#include <stdlib.h>

#include "core/commit.h"
#include "utils/color.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

#define ALL -1

static inline void print_log(commit_node_t *node) {
    char *log, *datetime, *commit_hash;
    get_commit_node_info(node, &log, &datetime, &commit_hash);
    printf(COLOR_BROWN "Commit: %s\t" COLOR_END "Date: %s\tLog: \"%s\"\n", commit_hash, datetime,
           log);
}

void cmd_log(int argc, char *argv[]) {
    if (check_init() == NOT_INIT) {
        error_not_init();
    }
    long amount;

    if (argc == 1) {
        amount = ALL;
    } else if (argc == 2) {
        char *c = argv[1] + 1;
        if (*argv[1] == '-' && *c >= '0' && *c <= '9') {
            for (; *c != '\0'; c++) {
                if (*c < '0' || *c > '9')
                    error_custom_msg("'%s' is not a positive integer\n", argv[1] + 1);
            }
            amount = strtol(argv[1] + 1, NULL, 10);
        } else
            error_custom_msg("Usage: big log [-<amount>]\n");
    } else
        error_custom_msg("Usage: big log [-<amount>]\n");

    char *leader_id = load_leader();
    if (leader_id == NULL) {
        error_custom_msg("No commit\n");
    }

    commit_node_t *leader_node;
    if (amount == ALL) {
        leader_node = load_parent_info(leader_id, NULL);
    } else {
        leader_node = load_parent_info(leader_id, &amount);
    }

    commit_node_t *current_node = leader_node;
    while (current_node != NULL) {
        print_log(current_node);
        current_node = get_commit_parent(current_node);
    }

    commit_node_free(&leader_node);
}