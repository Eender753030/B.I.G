#include "utils.h"

#include <stdlib.h>
#include <string.h>

#include "error_handle.h"

char *str_dup(const char *string) {
    char *new_string = (char *)malloc(strlen(string) + 1);
    if (new_string == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    strcpy(new_string, string);

    return new_string;
}

void cd_to_project_root(char **org_dir) {
    if (org_dir != NULL) {
        *org_dir = (char *)malloc(1024);
        if (*org_dir == NULL || getcwd(*org_dir, 1024) == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    char current_dir[1024];
    while (access(".big", F_OK) == -1) {
        if (getcwd(current_dir, 1024) == NULL)
            ErrnoHandler(__func__, __FILE__, __LINE__);

        if (strncmp(current_dir, "/", 2) == 0)
            ErrorCustomMsg("Error: can not cd to outside the root directory\n");

        if (chdir("..") == -1)
            ErrnoHandler(__func__, __FILE__, __LINE__);
    }
}
