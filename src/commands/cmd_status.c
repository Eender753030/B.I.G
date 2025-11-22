#include "commands/cmd_status.h"

void cmd_status(int argc, char *argv[]) {
    if (argc > 2)
        ErrorCustomMsg("Usage: big status\n");
}