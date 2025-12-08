#include "utils/error_handle.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Usage hint string
static const char usage_hint[] =
    "\nUsage: big <command> [-option] [<args>]\n\n"
    "Commands:\n"
    "\tbig init                                           Initialize current directory to project "
    "root\n"
    "\tbig add [-d | --delete] <file or directory> ...    Add files into index to wait for commit\n"
    "\tbig commit [-m <message>]                          Commit and enter log\n"
    "\tbig log [-<amount>]                                Show previous commit logs\n"
    "\tbig status                                         Show the status of files from project "
    "root\n"
    "\tbig checkout <commit hash or branch name>          Change project root to past commit "
    "status\n"
    "\tbig branch [-d | -delete] [<branch name>]          List branches or create branch with "
    "name\n\n";

void errno_handle(const char *func_name, const char *file_name, const int line) {
    // strerror(errno) converts the error number to a string that can read
    fprintf(stderr, "Error in function '%s' at line %d of %s: %s\n", func_name, line, file_name,
            strerror(errno));
    exit(EXIT_FAILURE);
}

void error_custom_msg(const char *msg, ...) {
    va_list args;
    va_start(args, msg);  // Take out args in ...
    vprintf(msg, args);   // Print out format string with args
    va_end(args);

    exit(EXIT_FAILURE);
}

void warning_custom_msg(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}

void error_input() {
    fprintf(stderr, "%s\n", usage_hint);  // Print out usage hint string
    exit(EXIT_FAILURE);
}

void error_not_init() {
    fprintf(
        stderr,
        "Error: Not an initialized directory. Use 'big init' to initialize current directory\n");
    exit(EXIT_FAILURE);
}
