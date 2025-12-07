#include "utils/utils.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"

char *str_dup(const char *string) {
    uint64_t str_len = strlen(string);        // strlen() is length without '\0'
    char *new_string = xmalloc(str_len + 1);  // + 1 for '\0'
    memcpy(new_string, string, str_len);  // use memcpy() is faster because already know the length
    new_string[str_len] = '\0';
    return new_string;
}

// A stupid way to check initialization
bool check_init() {
    // Save current directory to return later
    char *org_dir = xgetcwd();

    char *cwd = NULL;
    // Keep changing working directory to parent when meet .big\ or root directory /
    do {
        xfree(cwd);
        cwd = xgetcwd();  // Get current directory

        // Check if .big/ exists in the current directory
        if (access(".big", F_OK) != -1) {
            xfree(cwd);
            // Return to original directory
            if (chdir(org_dir) == -1) {
                xfree(org_dir);
                errno_handle(__func__, __FILE__, __LINE__);
            }
            xfree(org_dir);
            return INITED;
        }
        // Move to parent directory
        if (chdir("..") == -1) {
            xfree(cwd);
            xfree(org_dir);
            errno_handle(__func__, __FILE__, __LINE__);
        }
    } while (strcmp(cwd, "/"));  // Stop if already in root directory /

    xfree(org_dir);
    return NOT_INIT;
}

void cd_to_project_root(char **org_dir) {
    // Save the original directory if caller need
    if (org_dir != NULL) {
        *org_dir = xgetcwd();
    }

    char current_dir[4096];

    // Keep moving to parent directory until meet .big/
    while (access(".big", F_OK) == -1) {
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            errno_handle(__func__, __FILE__, __LINE__);
        }

        // If we meet root / , it is an error
        if (strcmp(current_dir, "/") == 0) {
            error_custom_msg("Error: can not cd to outside the root directory\n");
        }
        if (chdir("..") == -1) {
            errno_handle(__func__, __FILE__, __LINE__);
        }
    }
}

/* DJB2 Hash Algorithm
 * A simple string hash function.
 * Use 5381 as magic number
 */
uint64_t hash_function(const char *string) {
    uint64_t hash = 5381;
    for (; *string != '\0'; string++) {
        hash = ((hash << 5) + hash) + (uint64_t)(*string);  // << 5 is faster then * 2^5
    }
    return hash;
}

char *hash_to_string(uint64_t hash) {
    // long is 8 bytes, * 2 for 16 char plus 1 '\0'
    uint64_t size = sizeof(hash) * 2 + 1;
    char *hex_str = xmalloc(size);

    snprintf(hex_str, size, "%llx", (unsigned long long)hash);
    return hex_str;
}

char *datetime_now_to_str() {
    char date_buffer[100];
    struct tm *datetime_now;
    time_t time_now = time(NULL);  // Get current time

    datetime_now = localtime(&time_now);
    // Format to string
    strftime(date_buffer, sizeof(date_buffer), "%Y/%m/%d %H:%M:%S", datetime_now);

    return str_dup(date_buffer);
}