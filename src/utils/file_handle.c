#include "utils/file_handle.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/error_handle.h"
#include "utils/utils.h"

char *read_whole_file(const char *full_path) {
    FILE *file = fopen(full_path, "rb");
    if (file == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    fseek(file, 0, SEEK_END);
    size_t file_len = ftell(file);

    char *buffer = (char *)malloc(file_len + 1);
    if (buffer == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    rewind(file);

    size_t bytes_read = fread(buffer, 1, file_len, file);
    if (bytes_read != file_len)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    buffer[file_len] = '\0';

    fclose(file);

    return buffer;
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

char *relative_path_calc(const char *org_dir, const char *root_path) {
    char *normalized_path;
    char temp_path[4096];
    char root_dir[4096];
    if (getcwd(root_dir, 4096) == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    snprintf(temp_path, sizeof(temp_path), "%s/%s", org_dir, root_path);

    char absolute_path[4096];
    if (realpath(temp_path, absolute_path) == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);
    char *relative_path = strstr(absolute_path, root_dir);

    if (relative_path != NULL && strcmp(relative_path, root_dir) != 0) {
        relative_path += strlen(root_dir) + 1;
        normalized_path = str_dup(relative_path);
    } else {
        normalized_path = str_dup(".");
    }

    return normalized_path;
}