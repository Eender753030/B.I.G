#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands/cmd.h"
#include "commands/cmd_add.h"
#include "commands/cmd_commit.h"
#include "commands/cmd_init.h"
#include "commands/cmd_log.h"
#include "commands/cmd_status.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

/**
 * @brief Command Table.
 * Maps command names to their handler functions.
 * Ends with {NULL, NULL} as a sentinel.
 */
static const Command commands[] = {{"init", cmd_init},     {"add", cmd_add},
                                   {"commit", cmd_commit}, {"log", cmd_log},
                                   {"status", cmd_status}, {NULL, NULL}};

/**
 * @brief Main entry point. Dispatches subcommands.
 */
int main(int argc, char **argv) {
    if (argc < 2) {
        InputError();  // Show usage if no command provided
    }

    const char *input_cmd = argv[1];

    // Iterate through commands until sentinel (NULL) is reached
    for (int num_of_cmd = 0; commands[num_of_cmd].cmd_name != NULL; num_of_cmd++) {
        if (strcmp(input_cmd, commands[num_of_cmd].cmd_name) == 0) {
            // Execute command.
            // Pass (argc - 1) and (argv + 1) to skip the program name.
            commands[num_of_cmd].cmd(argc - 1, argv + 1);
            return EXIT_SUCCESS;
        }
    }

    InputError();  // Command not found
    return EXIT_FAILURE;
}
