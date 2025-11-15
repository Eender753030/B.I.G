#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_handle.h"
#include "init.h"

void parse(int argc, char **argv) {
    if (argc < 2)
        InputError();

    if (!strncmp(argv[1], "init", 5))
        init();

    else
        InputError();
}