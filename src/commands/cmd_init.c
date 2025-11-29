#include "commands/cmd_init.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/error_handle.h"
#include "utils/utils.h"

void cmd_init(int argc, char *argv[]) {
    UNUSED(argv);

    if (argc > 1) {
        ErrorCustomMsg("Usage: big init\n");
    }

    if (access(".big", F_OK) != -1) {
        ErrorCustomMsg("Error: Directory already initalize. Operation cancelled\n");
    }
    printf("Start to initalize B.I.G structure...\n");
    if (mkdir(".big", 0775) == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    if (mkdir(".big/objects", 0775) == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    printf("Directory initalize complete\n");
}