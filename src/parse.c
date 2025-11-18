#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_handle.h"
#include "init.h"
#include "snapshot.h"

void parse(int argc, char **argv) {
    if (argc < 2)
        InputError();

    if (strncmp(argv[1], "init", 5) == 0)
        init();
    else if (strncmp(argv[1], "add", 4) == 0) {
        if (cheak_init() == -1)
            NotInitError();
        if (argc < 3) {
            ErrorCustomMsg(
                "Usage: big add <filename or directory> <...>\n"
                "Use 'big add .' in root of project directory to add whole\n");
        }
        add((size_t)argc - 2, (const char **)argv + 2);
    } else
        InputError();
}