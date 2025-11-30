#include "commands/cmd_branch.h"

#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core/commit.h"
#include "utils/color.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

static void print_branches() {
    char *curr_branch = load_current_branch();
    if (curr_branch == NULL) {
        error_custom_msg("No commit\n");
    }

    DIR *ref_dir = opendir(".big/refs");
    if (ref_dir == NULL) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    struct dirent *ref;
    while ((ref = readdir(ref_dir)) != NULL) {
        if (strcmp(ref->d_name, ".") == 0 || strcmp(ref->d_name, "..") == 0) {
            continue;
        }

        if (strcmp(ref->d_name, curr_branch) == 0) {
            printf("* " COLOR_CYAN "%s\n" COLOR_END, curr_branch);
        } else {
            printf("  %s\n", ref->d_name);
        }
    }
    closedir(ref_dir);
    xfree(curr_branch);
}

static void create_branch(const char *branch_name) {
    char new_ref_path[4096];
    snprintf(new_ref_path, sizeof(new_ref_path), ".big/refs/%s", branch_name);

    if (access(new_ref_path, F_OK) == 0) {
        error_custom_msg("Error: Branch '%s' already exist\n", branch_name);
    }

    char *leader_commit_hash = load_leader();

    FILE *ref_file = xfopen(new_ref_path, "w");

    if (fprintf(ref_file, "%s\n", leader_commit_hash) < 0) {
        xfree(leader_commit_hash);
        fclose(ref_file);
        errno_handle(__func__, __FILE__, __LINE__);
    }

    printf("Branch '%s' created. Point to commit: " COLOR_BROWN "%s" COLOR_END "\n", branch_name,
           leader_commit_hash);

    xfree(leader_commit_hash);
    fclose(ref_file);
}

static void delete_branch(const char *branch_name) {
    char ref_path[4096];
    snprintf(ref_path, sizeof(ref_path), ".big/refs/%s", branch_name);

    if (access(ref_path, F_OK) != 0) {
        error_custom_msg("Error: Branch '%s' does not exist\n", branch_name);
    }

    if (remove(ref_path) == -1) {
        error_custom_msg("Error: Delete branch '%s' failed\n", branch_name);
    }
    printf("Branch '%s' deleted\n", branch_name);
}

void cmd_branch(int argc, char *argv[]) {
    if (check_init() == false) {
        error_not_init();
    }

    cd_to_project_root(NULL);

    if (argc == 1) {
        print_branches();
        return;
    }

    if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--delete") == 0) {
        if (argc < 3 || argc > 3) {
            error_custom_msg("Usage: big branch -d <branch name>\n");
        }
        if (strcmp(argv[2], "Leader") == 0 || strcmp(argv[2], "temp_checkout_ref") == 0) {
            error_custom_msg("'%s' is a unvalid name of branch\n", argv[2]);
        }
        delete_branch(argv[2]);
        return;
    }

    if (argc > 2) {
        error_custom_msg("Usage: big branch <branch name>\n");
    }
    if (strcmp(argv[1], "Leader") == 0 || strcmp(argv[1], "temp_checkout_ref") == 0) {
        error_custom_msg("'%s' as name is not allowed\n", argv[1]);
    }
    create_branch(argv[1]);
}
