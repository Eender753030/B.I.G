#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands/cmd.h"
#include "commands/cmd_add.h"
#include "commands/cmd_branch.h"
#include "commands/cmd_checkout.h"
#include "commands/cmd_commit.h"
#include "commands/cmd_init.h"
#include "commands/cmd_log.h"
#include "commands/cmd_status.h"
#include "utils/error_handle.h"

/* Command Table
 * Each command string pair with it's function
 * End with NULL that is the end point of check
 */
static const command_t commands[] = {
    {"init", cmd_init},     {"add", cmd_add},           {"commit", cmd_commit}, {"log", cmd_log},
    {"status", cmd_status}, {"checkout", cmd_checkout}, {"branch", cmd_branch}, {NULL, NULL}};

// Parse the user input commands from CLI
int main(int argc, char **argv) {
    // No command provided from user
    if (argc < 2) {
        error_input();  // Show usage
    }

    const char *input_cmd = argv[1];  // command

    // Iterate through command table until reach NULL
    for (int i = 0; commands[i].cmd_name != NULL; i++) {
        if (strcmp(input_cmd, commands[i].cmd_name) == 0) {
            // Find command that implemented and execute command.
            // Start from (argc - 1) and (argv + 1) to skip the program name.
            commands[i].cmd(argc - 1, argv + 1);
            return EXIT_SUCCESS;
        }
    }

    error_input();  // Command not found also show usage
    return EXIT_FAILURE;
}
