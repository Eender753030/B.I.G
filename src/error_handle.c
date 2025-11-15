#include "error_handle.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char usage_hint[] = "Usage: big <command>";

void MemoryError(char *func_name) {
    fprintf(stderr, "Error: %s from %s\n", strerror(errno), func_name);
    exit(EXIT_FAILURE);
}

void InputError() {
    fprintf(stderr, "%s\n", usage_hint);
    exit(EXIT_FAILURE);
}

void MakeDirectoryError(char *func_name) {
    fprintf(stderr, "Error: %s from %s\n", strerror(errno), func_name);
    exit(EXIT_FAILURE);
}
