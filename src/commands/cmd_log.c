#include "commands/cmd_log.h"

#include <stdio.h>
#include <stdlib.h>

#include "core/commit.h"
#include "utils/color.h"
#include "utils/error_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

#define ALL -1  // Use -1 represent all logs

// Helper function to print logs
static inline void print_log(commit_node_t *node) {
    char *log, *datetime, *commit_hash;
    get_commit_node_info(node, &log, &datetime, &commit_hash);
    // Use brown to represent commit hash, 16 character ensure the format
    printf(COLOR_BROWN "Commit: %16s\t" COLOR_END "Date: %19s\tLog: \"%s\"\n", commit_hash,
           datetime, log);
}

void cmd_log(int argc, char *argv[]) {
    // Check initalize first
    if (check_init() == NOT_INIT) {
        error_not_init();
    }
    long amount;

    // If just input 'big log', means print all logs to the first commit
    if (argc == 1) {
        amount = ALL;
    } else if (argc == 2) {
        char *c = argv[1] + 1;

        // Check the input is valid '-<amount>' for finite amount of logs
        if (*argv[1] == '-' && *c >= '0' && *c <= '9') {
            for (; *c != '\0'; c++) {
                if (*c < '0' || *c > '9')
                    error_custom_msg("'%s' is not a positive integer\n", argv[1] + 1);
            }
            amount = strtol(argv[1] + 1, NULL, 10);  // Use strtol instead of atoi
        } else
            error_custom_msg("Usage: big log [-<amount>]\n");
    } else
        error_custom_msg("Usage: big log [-<amount>]\n");

    // Back to project directory and save working directory first
    cd_to_project_root(NULL);

    char *leader_id = load_leader();
    if (leader_id == NULL) {
        error_custom_msg("No commit\n");
    }

    commit_node_t *leader_node;
    // Load all parent to the end
    if (amount == ALL) {
        leader_node = load_parent_info(leader_id, NULL);
    }
    // Load finite amount of parent
    else {
        leader_node = load_parent_info(leader_id, &amount);
    }

    // Load current branch name and print it by using cyan
    char *current_branch = load_current_branch();
    commit_node_t *current_node = leader_node;
    printf(COLOR_CYAN "Branch: %s\n" COLOR_END, current_branch);

    // Print out log and move to it's parent repeat to hit NULL
    while (current_node != NULL) {
        print_log(current_node);
        current_node = get_commit_parent(current_node);
    }

    xfree(current_branch);
    commit_node_free(&leader_node);
}