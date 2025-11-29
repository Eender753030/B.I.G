#include "utils/error_handle.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Usage hint string
static const char usage_hint[] =
    "\nUsage: big <command> [<args>]\n\n"
    "Commands:\n"
    "\tbig init                                           Initialize current directory to project "
    "root\n"
    "\tbig add [-d | --delete] <file or directory> ...    Add files into index to wait for commit\n"
    "\tbig commit [-m <message>]                          Commit and enter log\n"
    "\tbig log [-<amount>]                                Show previous commit logs\n"
    "\tbig status                                         Show the status of files from project "
    "root\n\n";

void errno_handle(const char *func_name, const char *file_name, const int line) {
    // strerror(errno) converts the error number to a human-readable string
    fprintf(stderr, "Error in function '%s' at line %d of %s: %s\n", func_name, line, file_name,
            strerror(errno));
    exit(EXIT_FAILURE);
}

void error_custom_msg(const char *msg, ...) {
    va_list args;
    va_start(args, msg);  // Initialize args to store all values after 'msg'
    vprintf(msg, args);
    va_end(args);

    // Custom errors are fatal, so we exit
    exit(EXIT_FAILURE);
}

void warning_custom_msg(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}

void error_input() {
    fprintf(stderr, "%s\n", usage_hint);
    exit(EXIT_FAILURE);
}

void error_not_init() {
    fprintf(stderr,
            "Error: Not a initalized directory. Use 'big init' to initalize current directory\n");
    exit(EXIT_FAILURE);
}
