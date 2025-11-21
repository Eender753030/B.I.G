#include "utils/error_handle.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char usage_hint[] =
    "\nUsage: big <command> [<args>]\n\n"
    "Commands:\n"
    "\tbig init                             Initalize current directory to project root\n"
    "\tbig add <file or direcotry> <...>    Add files into index to wait for commit\n"
    "\tbig commit [-m <\"message\">]        Commit and enter log\n"
    "\tbig log [-<number>]                  Show previous commit logs\n\n";

void ErrnoHandler(const char *func_name, const char *file_name, const int line) {
    fprintf(stderr, "Error in function '%s' at line %d of %s: %s\n", func_name, line, file_name,
            strerror(errno));
    exit(EXIT_FAILURE);
}
void ErrorCustomMsg(const char *msg, ...) {
    va_list args;
    va_start(args, msg);

    for (; *msg != '\0'; msg++) {
        if (*msg == '%') {
            msg++;
            switch (*msg) {
                case 's': {
                    const char *str = va_arg(args, const char *);
                    printf("%s", str ? str : "(null)");
                    break;
                }
                case 'd': {
                    int val = va_arg(args, int);
                    printf("%d", val);
                    break;
                }
                case '%':
                    putchar('%');
                    break;
                default:
                    putchar('%');
                    putchar(*msg);
            }
        } else
            putchar(*msg);
    }

    va_end(args);
    exit(EXIT_FAILURE);
}

void InputError() {
    fprintf(stderr, "%s\n", usage_hint);
    exit(EXIT_FAILURE);
}

void NotInitError() {
    fprintf(stderr,
            "Error: Not a initalized directory. Use 'big init' to initalize current directory\n");
    exit(EXIT_FAILURE);
}
