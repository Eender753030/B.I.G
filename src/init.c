#include "init.h"

#include <stdio.h>
#include <sys/stat.h>

#include "error_handle.h"

static const char dir_name[] = ".big";

void init() {
    struct stat dir_stat;

    if (!stat(dir_name, &dir_stat)) {
        fprintf(stderr, "Directory already initalize\nOperation cancelled\n");
        return;
    }

    printf("Start to initalize B.I.G structure...\n");
    if (mkdir(dir_name, 0775) == -1)
        MakeDirectoryError("init");

    printf("Directory initalize complete\n");
}