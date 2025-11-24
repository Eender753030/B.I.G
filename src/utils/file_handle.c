#include "utils/file_handle.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/error_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

FILE *_xfopen(const char *target_file_name, const char *modes, const char *func_name,
              const char *file_name, const int line) {
    FILE *file = fopen(target_file_name, modes);
    if (file == NULL) {
        ErrnoHandler(func_name, file_name, line);
    }
    return file;
}

char *read_whole_file(const char *file_name) {
    FILE *file = xfopen(file_name, "rb");

    /* * [Pattern] Get File Size
     * 1. Seek to the end (SEEK_END).
     * 2. Tell the position (ftell), which equals the size in bytes.
     * 3. Rewind back to start for reading.
     */
    fseek(file, 0, SEEK_END);
    size_t file_len = (size_t)ftell(file);
    rewind(file);

    // Allocate buffer (size + 1 for null terminator)
    char *content = xmalloc(file_len + 1);
    size_t bytes_read = fread(content, 1, file_len, file);
    if (bytes_read != file_len) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    content[file_len] = '\0';  // Ensure valid C-string
    fclose(file);
    return content;
}

void mk_dir_and_file(const char *path, const char *content) {
    char *temp_path = str_dup(path);

    /* * [Algorithm] Recursive Directory Creation
     * Iterate through the string finding each '/'.
     * Temporarily replace '/' with '\0' to create the parent directory.
     * Then restore '/' and continue.
     */
    char *slash_pos = temp_path;
    while ((slash_pos = strchr(slash_pos + 1, '/')) != NULL) {
        *slash_pos = '\0';  // Truncate string at current level

        // 0775: rwxrwxr-x (Standard shared directory permissions)
        if (mkdir(temp_path, 0775) == -1) {
            // It's okay if the directory already exists (EEXIST)
            if (errno != EEXIST) {
                ErrnoHandler(__func__, __FILE__, __LINE__);
            }
        }
        *slash_pos = '/';  // Restore and move to next level
    }

    // Write the actual file content
    FILE *target_file = xfopen(path, "wb");
    fwrite(content, 1, strlen(content), target_file);
    fclose(target_file);

    xfree(temp_path);
}

char *relative_path_calc(const char *org_dir, const char *root_path) {
    char *normalized_path;
    char temp_path[4096];
    char root_dir[4096];

    if (getcwd(root_dir, 4096) == NULL) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    // Construct the potential absolute path
    snprintf(temp_path, sizeof(temp_path), "%s/%s", org_dir, root_path);

    char absolute_path[4096];
    // realpath resolves ".." and "." and symlinks to a canonical path
    if (realpath(temp_path, absolute_path) == NULL) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    // Check if the file is inside our project root
    char *relative_path = strstr(absolute_path, root_dir);

    if (relative_path != NULL && strcmp(relative_path, root_dir) != 0) {
        // Pointer arithmetic: Skip the root_dir part + 1 for the '/'
        relative_path += strlen(root_dir) + 1;
        normalized_path = str_dup(relative_path);
    } else {
        // If paths match exactly, we are at the root
        normalized_path = str_dup(".");
    }
    return normalized_path;
}