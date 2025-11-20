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

unsigned long hash_function(char *string) {
    unsigned long hash = 5381;
    for (; *string != '\0'; string++) {
        hash = ((hash << 5) + hash) + *string;
    }
    return hash;
}

char *hash_to_string(unsigned long hash) {
    char *hex_str = (char *)malloc(17);
    if (hex_str == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);
    sprintf(hex_str, "%lx", hash);
    return hex_str;
}

void mk_dir_and_file(const char *path, const char *content) {
    char *temp_path = str_dup(path);

    char *slash_pos = temp_path;
    while ((slash_pos = strchr(slash_pos + 1, '/')) != NULL) {
        *slash_pos = '\0';
        if (mkdir(temp_path, 0775) == -1) {
            if (errno != EEXIST)
                ErrnoHandler(__func__, __FILE__, __LINE__);
        }
        *slash_pos = '/';
    }
    FILE *target_file = fopen(path, "wb");
    if (target_file == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    fwrite(content, 1, strlen(content), target_file);

    fclose(target_file);
    free(temp_path);
}