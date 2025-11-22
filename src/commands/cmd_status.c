#include "commands/cmd_status.h"

#include "utils/error_handle.h"
#include "utils/utils.h"

void cmd_status(int argc, char *argv[]) {
    if (check_init() == -1)
        NotInitError();

    if (argc > 1)
        ErrorCustomMsg("Usage: big status\n");
}