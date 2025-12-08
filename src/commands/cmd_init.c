#include "commands/cmd_init.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/error_handle.h"
#include "utils/utils.h"

void cmd_init(int argc, char *argv[]) {
    // argv is not need
    UNUSED(argv);

    // Make sure is only 'big init'
    if (argc > 1) {
        error_custom_msg("Usage: big init\n");
    }

    // Exit if .big/ is already exist
    if (access(".big", F_OK) != -1) {
        error_custom_msg("Error: Directory already initalized. Operation cancelled\n");
    }
    printf("Start to initialize B.I.G structure...\n");

    // Initialize current directory to project directory
    // Create essential directories
    if (mkdir(".big", 0775) == -1) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    if (mkdir(".big/objects", 0775) == -1) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    if (mkdir(".big/refs", 0775) == -1) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    printf("Directory initialize complete\n");
}