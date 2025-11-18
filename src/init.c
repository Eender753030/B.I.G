#include "init.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error_handle.h"

static const char dir_name[] = ".big";

int cheak_init() {
    char org_dir[1024];
    if (getcwd(org_dir, 1024) == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    char cwd[1024];

    do {
        getcwd(cwd, 1024);
        if (access(".big", F_OK) != -1) {
            chdir(org_dir);
            return 0;
        }
        chdir("..");
    } while (strncmp(cwd, "/", 2));

    return -1;
}

void init() {
    if (access(".big", F_OK) != -1) {
        fprintf(stderr, "Error: Directory already initalize. Operation cancelled\n");
        return;
    }

    printf("Start to initalize B.I.G structure...\n");
    if (mkdir(dir_name, 0775) == -1)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    printf("Directory initalize complete\n");
}