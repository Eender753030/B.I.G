#include "error_handle.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char usage_hint[] = "Usage: big <command>";

void ErrnoHandler(const char *func_name, const char *file_name, const int line) {
    fprintf(stderr, "Error in function '%s' at line %d of %s: %s\n", func_name, line, file_name,
            strerror(errno));
    exit(EXIT_FAILURE);
}

void InputError() {
    fprintf(stderr, "%s\n", usage_hint);
    exit(EXIT_FAILURE);
}