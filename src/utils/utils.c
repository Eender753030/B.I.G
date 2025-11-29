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
    // Allocate memory: length of string + 1 for null terminator
    uint64_t str_len = strlen(string);
    char *new_string = xmalloc(str_len + 1);
    memcpy(new_string, string, str_len);
    new_string[str_len] = '\0';
    return new_string;
}

bool check_init() {
    // Save current directory to return later
    char *org_dir = xgetcwd();

    char cwd[4096];
    // Traverse up the directory tree until ".big" is found or root is reached
    do {
        getcwd(cwd, 4096);  // Get current directory

        // Check if ".big" exists in the current level
        if (access(".big", F_OK) != -1) {
            chdir(org_dir);  // Return to original directory before exiting
            xfree(org_dir);
            return INITED;
        }
        chdir("..");  // Move to parent directory
    } while (strcmp(cwd, "/"));  // Stop if we hit the root directory "/"

    xfree(org_dir);
    return NOT_INIT;
}

void cd_to_project_root(char **org_dir) {
    // If the caller requested the original path, save it first
    if (org_dir != NULL) {
        *org_dir = xgetcwd();
    }

    char current_dir[4096];

    // Keep moving up ("..") until we find the ".big" folder
    while (access(".big", F_OK) == -1) {
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            errno_handle(__func__, __FILE__, __LINE__);
        }

        // Safety check: If we hit root "/" and still haven't found .big, it's an error
        if (strcmp(current_dir, "/") == 0) {
            error_custom_msg("Error: can not cd to outside the root directory\n");
        }
        if (chdir("..") == -1) {
            errno_handle(__func__, __FILE__, __LINE__);
        }
        // Now the CWD (Current Working Directory) is the project root
    }
}

/* * DJB2 Hash Algorithm
 * A simple and effective string hash function.
 * Magic number 5381 and multiplier 33 (<<5 + 1) provide good distribution.
 */
uint64_t hash_function(const char *string) {
    uint64_t hash = 5381;
    for (; *string != '\0'; string++) {
        hash = ((hash << 5) + hash) + (uint64_t)(*string);
    }
    return hash;
}

char *hash_to_string(uint64_t hash) {
    // Size needs to be enough for hex representation + null terminator
    // sizeof(hash) * 2 covers 2 hex chars per byte.
    uint64_t size = sizeof(hash) * 2 + 1;
    char *hex_str = xmalloc(size);

    snprintf(hex_str, size, "%llx", (unsigned long long)hash);
    return hex_str;
}

char *datetime_now_to_str() {
    char date_buffer[100];
    struct tm *datetime_now;
    time_t time_now = time(NULL);

    datetime_now = localtime(&time_now);
    strftime(date_buffer, sizeof(date_buffer), "%Y/%m/%d %H:%M:%S", datetime_now);

    return str_dup(date_buffer);
}