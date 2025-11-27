#include "utils/utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/error_handle.h"
#include "utils/memory.h"

char *str_dup(const char *string) {
    // Allocate memory: length of string + 1 for null terminator
    char *new_string = xmalloc(strlen(string) + 1);
    strcpy(new_string, string);
    return new_string;
}

int check_init() {
    char org_dir[4096];

    // Save current directory to return later
    if (getcwd(org_dir, 4096) == NULL) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    char cwd[4096];

    // Traverse up the directory tree until ".big" is found or root is reached
    do {
        getcwd(cwd, 4096);  // Get current directory

        // Check if ".big" exists in the current level
        if (access(".big", F_OK) != -1) {
            chdir(org_dir);  // Return to original directory before exiting
            return INITED;
        }
        chdir("..");  // Move to parent directory
    } while (strncmp(cwd, "/", 2));  // Stop if we hit the root directory "/"

    return NOT_INIT;
}

void cd_to_project_root(char **org_dir) {
    // If the caller requested the original path, save it first
    if (org_dir != NULL) {
        char buffer[4096];
        if (getcwd(buffer, 4096) == NULL) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }
        *org_dir = str_dup(buffer);
        if (*org_dir == NULL) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }
    }

    char current_dir[4096];

    // Keep moving up ("..") until we find the ".big" folder
    while (access(".big", F_OK) == -1) {
        if (getcwd(current_dir, 4096) == NULL) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }

        // Safety check: If we hit root "/" and still haven't found .big, it's an error
        if (strncmp(current_dir, "/", 2) == 0) {
            ErrorCustomMsg("Error: can not cd to outside the root directory\n");
        }
        if (chdir("..") == -1) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }
        // Now the CWD (Current Working Directory) is the project root
    }
}

/* * DJB2 Hash Algorithm
 * A simple and effective string hash function.
 * Magic number 5381 and multiplier 33 (<<5 + 1) provide good distribution.
 */
unsigned long hash_function(const char *string) {
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