#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands/commit.h"
#include "commands/init.h"
#include "commands/log.h"
#include "commands/snapshot.h"
#include "utils/error_handle.h"

int main(int argc, char **argv) {
    if (argc < 2)
        InputError();

    if (strncmp(argv[1], "init", 5) == 0)
        init();

    else if (strncmp(argv[1], "add", 4) == 0) {
        if (check_init() == -1)
            NotInitError();
        if (argc < 3) {
            ErrorCustomMsg(
                "Usage: big add <filename or directory> <...>\n"
                "Use 'big add .' in root of project directory to add whole\n");
        }
        add((size_t)argc - 2, (const char **)argv + 2);

    } else if (strncmp(argv[1], "commit", 7) == 0) {
        if (check_init() == -1)
            NotInitError();
        if (argc == 2)
            commit(NULL);
        else {
            if (strncmp(argv[2], "-m", 3) == 0 && argc == 4)
                commit(argv[3]);
            else
                ErrorCustomMsg("Usage: big commit [-m \"<log message>\"]\n");
        }

    } else if (strncmp(argv[1], "log", 4) == 0) {
        if (argc == 2)
            cmd_log(NULL);
        else {
            for (int i = 2; i < argc; i++) {
                if (*argv[i] == '-') {
                    char *c = argv[i] + 1;
                    if (*c >= '0' && *c <= '9') {
                        for (; *c != '\0'; c++) {
                            if (*c < '0' || *c > '9')
                                ErrorCustomMsg("'%s' is not a positive integer\n", argv[i] + 1);
                        }
                        long amount = strtol(argv[i] + 1, NULL, 10);
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