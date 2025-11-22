#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands/cmd.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        InputError();
    }

    const char *input_cmd = argv[1];

    for (int num_of_cmd = 0; commands[num_of_cmd].cmd_name != NULL; num_of_cmd++) {
        if (strcmp(input_cmd, commands[num_of_cmd].cmd_name) == 0) {
            commands[num_of_cmd].cmd(argc, argv);
            return EXIT_SUCCESS;
        }
    }

    InputError();

    return EXIT_FAILURE;
}
