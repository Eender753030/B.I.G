#include "utils/utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/error_handle.h"

char *str_dup(const char *string) {
    char *new_string = (char *)malloc(strlen(string) + 1);
    if (new_string == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    strcpy(new_string, string);

    return new_string;
}

int check_init() {
    char org_dir[4096];
    if (getcwd(org_dir, 4096) == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    char cwd[4096];

    do {
        getcwd(cwd, 4096);
        if (access(".big", F_OK) != -1) {
            chdir(org_dir);
            return 0;
        }
        chdir("..");
    } while (strncmp(cwd, "/", 2));

    return -1;
}

void cd_to_project_root(char **org_dir) {
    if (org_dir != NULL) {
        char buffer[4096];
        if (getcwd(buffer, 4096) == NULL)
            ErrnoHandler(__func__, __FILE__, __LINE__);

        *org_dir = str_dup(buffer);
        if (*org_dir == NULL)
            ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    char current_dir[4096];
    while (access(".big", F_OK) == -1) {
        if (getcwd(current_dir, 4096) == NULL)
            ErrnoHandler(__func__, __FILE__, __LINE__);

        if (strncmp(current_dir, "/", 2) == 0)
            ErrorCustomMsg("Error: can not cd to outside the root directory\n");

        if (chdir("..") == -1)
            ErrnoHandler(__func__, __FILE__, __LINE__);
    }
}

unsigned long hash_function(const char *string) {
    unsigned long hash = 5381;
    char *temp_string = str_dup(string);
    for (char *c = temp_string; *c != '\0'; c++) {
        hash = ((hash << 5) + hash) + (unsigned long)*c;
    }
    free(temp_string);
    return hash;
}

char *hash_to_string(unsigned long hash) {
    char *hex_str = (char *)malloc(17);
    if (hex_str == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);
    sprintf(hex_str, "%lx", hash);
    return hex_str;
}