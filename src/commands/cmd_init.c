#include "commands/cmd_init.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/error_handle.h"

static const char dir_name[] = ".big";

void cmd_init(int argc, char *argv[]) {
    if (argc > 2) {
        ErrorCustomMsg("Usage: big init\n");
    }

    if (access(".big", F_OK) != -1) {
        fprintf(stderr, "Error: Directory already initalize. Operation cancelled\n");
        return;
    }

    printf("Start to initalize B.I.G structure...\n");
    if (mkdir(dir_name, 0775) == -1)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    printf("Directory initalize complete\n");
}