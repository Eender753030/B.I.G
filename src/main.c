#include <stdlib.h>
#include <string.h>

#include "commands/cmd_add.h"
#include "commands/cmd_commit.h"
#include "commands/cmd_init.h"
#include "commands/cmd_log.h"
#include "utils/error_handle.h"
#include "utils/utils.h"

int main(int argc, char **argv) {
    if (argc < 2)
        InputError();

    if (strncmp(argv[1], "init", 5) == 0) {
        if (argc == 2)
        cmd_init();
        else
            ErrorCustomMsg("Usage: big init\n");

    } else if (check_init() == -1)
        NotInitError();

    else if (strncmp(argv[1], "add", 4) == 0) {
        if (argc < 3) {
            ErrorCustomMsg(
                "Usage: big add <filename or directory> <...>\n"
                "Use 'big add .' in root of project directory to add whole\n");
        }
        cmd_add((size_t)argc - 2, (const char **)argv + 2);

    } else if (strncmp(argv[1], "commit", 7) == 0) {
        if (argc == 2)
            cmd_commit(NULL);
        else {
            if (strncmp(argv[2], "-m", 3) == 0 && argc == 4)
                cmd_commit(argv[3]);
            else
                ErrorCustomMsg("Usage: big commit [-m \"<log message>\"]\n");
        }

    } else if (strncmp(argv[1], "log", 4) == 0) {
        if (argc == 2)
            cmd_log(NULL);
        else if (argc == 3) {
            char *c = argv[2] + 1;
            if (*argv[2] == '-' && *c >= '0' && *c <= '9') {
                        for (; *c != '\0'; c++) {
                            if (*c < '0' || *c > '9')
                        ErrorCustomMsg("'%s' is not a positive integer\n", argv[2] + 1);
                        }
                long amount = strtol(argv[2] + 1, NULL, 10);
                        cmd_log(&amount);

                    } else
                        ErrorCustomMsg("Usage: big log [-<amount>]\n");
                } else
                    ErrorCustomMsg("Usage: big log [-<amount>]\n");
            }
        }

    } else
        InputError();

    return EXIT_SUCCESS;
}