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

// lazy error handle for me
FILE *_xfopen(const char *target_file_name, const char *modes, const char *func_name,
              const char *file_name, const int line) {
    FILE *file = fopen(target_file_name, modes);
    if (file == NULL) {
        errno_handle(func_name, file_name, line);
    }
    return file;
}

// lazy error handle for me
long _xftell(FILE *file, const char *func_name, const char *file_name, const int line) {
    long ftell_val = ftell(file);
    if (ftell_val == -1L) {  // ftell return -1L if error
        errno_handle(func_name, file_name, line);
    }
    return ftell_val;
}

// lazy error handle for me
char *_xgetcwd(const char *func_name, const char *file_name, const int line) {
    // getcwd() means get current working directory.
    char *cwd = getcwd(NULL, 0);
    // Input NULL it will allocate memory for return string. I must free it

    if (cwd == NULL) {
        errno_handle(func_name, file_name, line);
    }
    return cwd;
}

uint64_t get_file_len(FILE *file) {
    // Store current position in file. Because maybe is not in the start of file
    long current_pos = xftell(file);

    // If already in end. Just return 0
    if (current_pos == EOF) {
        return 0;
    }

    // Go to last position of file and check if success
    if (fseek(file, 0, SEEK_END) != 0) {
        return 0;
    }

    // Get length of whole file
    long file_len = xftell(file);

    // Back to the original position of file input
    if (fseek(file, current_pos, SEEK_SET) != 0) {
        return 0;
    }

    // Return length between end and original position of file input
    return (uint64_t)(file_len - current_pos);
}

char *read_whole_file(const char *file_name, uint64_t *len) {
    FILE *file = xfopen(file_name, "rb");  // Use read binary mode for maybe binary file

    uint64_t file_len = get_file_len(file);
    char *content = xmalloc(file_len + 1);  // + 1 for '\0'

    // fread with file_len make sure binary file will not terminate by zero value ('\0' is also 0)
    uint64_t bytes_read = fread(content, 1, file_len, file);
    if (bytes_read != file_len) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    content[file_len] = '\0';  // Ensure '\0' in the end
    fclose(file);

    if (len != NULL) {
        *len = file_len;  // Pass length if caller need it
    }
    return content;
}

void mk_dir_and_file(const char *path, const char *content, uint64_t content_len) {
    char *temp_path = str_dup(path);

    // Start from left to right first / and turn it to '\0'
    // temp_path string will terminate by '\0' and make the directory
    // And then ture '\0' back to / and keep repeat until reach file name
    char *slash_pos = temp_path;
    while ((slash_pos = strchr(slash_pos + 1, '/')) != NULL) {  // slash_pos + 1 for skip '\0'
        *slash_pos = '\0';
        // mkdir for 0775 means rwxrwxr-x
        if (mkdir(temp_path, 0775) == -1) {
            // Directory already exist is OK so skip
            if (errno != EEXIST) {
                errno_handle(__func__, __FILE__, __LINE__);
            }
        }
        *slash_pos = '/';
    }

    // Write all content into file
    FILE *target_file = xfopen(path, "wb");
    fwrite(content, 1, content_len, target_file);
    fclose(target_file);

    xfree(temp_path);
}

// This is a helper function for path does not exist
static char *path_normalize(const char *path) {
    char *paths[1024];   // A buffer that store every part of string
    uint16_t depth = 0;  // count the depth this path

    char *temp_path = str_dup(path);

    // Use strtok for split string. temp_path + 1 for skip if first is /
    char *token = strtok(temp_path + 1, "/");
    while (token != NULL) {
        // if is ".." means go to previous directory
        if (strcmp(token, "..") == 0) {
            if (depth > 0) {
                depth--;
            }
        }
        // if is not "." store string and then go deeper
        else if (strcmp(token, ".") != 0) {
            paths[depth++] = token;
        }

        // keep take next splited string
        token = strtok(NULL, "/");
    }

    char path_buffer[4096] = {0};
    // Combine each part of path
    // depth is 0 means it is in current directory
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

    // Use realpath() to take real absolute path without "." and ".."
    char *absolute_path = realpath(temp_path, NULL);

    if (absolute_path == NULL) {
        // If path does not exist, use path_normalize
        if (errno == ENOENT) {
            absolute_path = path_normalize(temp_path);
        } else {
            xfree(root_dir);
            errno_handle(__func__, __FILE__, __LINE__);
        }
    }

    // use strstr() to take the diff
    char *relative_path = strstr(absolute_path, root_dir);
    if (relative_path != NULL && strcmp(relative_path, root_dir) != 0) {
        // + 1 for skip '/'
        relative_path += strlen(root_dir) + 1;
        normalized_path = str_dup(relative_path);
    } else {
        // If path is the same, means is in the project directory
        normalized_path = str_dup(".");
    }
    xfree(root_dir);
    xfree(absolute_path);
    return normalized_path;
}