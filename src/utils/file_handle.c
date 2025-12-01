#include "utils/file_handle.h"

#include <errno.h>
#include <stdint.h>
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
        errno_handle(func_name, file_name, line);
    }
    return file;
}

long _xftell(FILE *file, const char *func_name, const char *file_name, const int line) {
    long ftell_val = ftell(file);
    if (ftell_val == -1L) {
        errno_handle(func_name, file_name, line);
    }
    return ftell_val;
}

char *_xgetcwd(const char *func_name, const char *file_name, const int line) {
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        errno_handle(func_name, file_name, line);
    }
    return cwd;
}

uint64_t get_file_len(FILE *file) {
    long current_pos = xftell(file);
    if (current_pos == -1) {
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        return 0;
    }
    long file_len = xftell(file);

    if (fseek(file, current_pos, SEEK_SET) != 0) {
        return 0;
    }

    return (uint64_t)(file_len - current_pos);
}

char *read_whole_file(const char *file_name, uint64_t *len) {
    FILE *file = xfopen(file_name, "rb");

    uint64_t file_len = get_file_len(file);
    // Allocate buffer (size + 1 for null terminator)
    char *content = xmalloc(file_len + 1);
    uint64_t bytes_read = fread(content, 1, file_len, file);
    if (bytes_read != file_len) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    content[file_len] = '\0';  // Ensure valid C-string
    fclose(file);

    if (len != NULL) {
        *len = file_len;
    }
    return content;
}

void mk_dir_and_file(const char *path, const char *content, uint64_t content_len) {
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
                errno_handle(__func__, __FILE__, __LINE__);
            }
        }
        *slash_pos = '/';  // Restore and move to next level
    }

    // Write the actual file content
    FILE *target_file = xfopen(path, "wb");
    fwrite(content, 1, content_len, target_file);
    fclose(target_file);

    xfree(temp_path);
}

static char *path_normalize(const char *path) {
    char *paths[1024];
    uint16_t depth = 0;

    char *temp_path = str_dup(path);

    char *token = strtok(temp_path + 1, "/");
    while (token != NULL) {
        if (strcmp(token, "..") == 0) {
            if (depth > 0) {
                depth--;
            }
        } else if (strcmp(token, ".") != 0) {
            paths[depth++] = token;
        }
        token = strtok(NULL, "/");
    }

    char path_buffer[4096] = {0};
    if (depth == 0) {
        snprintf(path_buffer, sizeof(path_buffer), "%s/.", path);
    } else {
        for (uint16_t i = 0; i < depth; i++) {
            strcat(path_buffer, "/");
            strcat(path_buffer, paths[i]);
        }
    }

    xfree(temp_path);
    return str_dup(path_buffer);
}

char *relative_path_calc(const char *org_dir, const char *root_path) {
    char *normalized_path;

    char *root_dir = xgetcwd();

    // Construct the potential absolute path
    char temp_path[4096];
    snprintf(temp_path, sizeof(temp_path), "%s/%s", org_dir, root_path);

    // realpath resolves ".." and "." and symlinks to a canonical path
    char *absolute_path = realpath(temp_path, NULL);

    if (absolute_path == NULL) {
        if (errno == ENOENT) {
            absolute_path = path_normalize(temp_path);
        } else {
            xfree(root_dir);
            errno_handle(__func__, __FILE__, __LINE__);
        }
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
    xfree(root_dir);
    xfree(absolute_path);
    return normalized_path;
}