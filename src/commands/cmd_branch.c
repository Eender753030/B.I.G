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

// Helper for print out all branches
static void print_branches(const char *curr_branch) {
    // refs/ store all branch, so scan all files in it
    DIR *ref_dir = opendir(".big/refs");
    if (ref_dir == NULL) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    struct dirent *ref;

    // scan all files in refs/
    while ((ref = readdir(ref_dir)) != NULL) {
        // Skip this two
        if (strcmp(ref->d_name, ".") == 0 || strcmp(ref->d_name, "..") == 0) {
            continue;
        }

        // Check which one is current branch and mark it with * and cyan
        if (strcmp(ref->d_name, curr_branch) == 0) {
            printf("* " COLOR_CYAN "%s\n" COLOR_END, curr_branch);
        }
        // Just print out if not current branch
        else {
            printf("  %s\n", ref->d_name);
        }
    }
    closedir(ref_dir);
}

// Helper function to create a new branch
static void create_branch(const char *branch_name) {
    char new_ref_path[4096];
    // Combine the path to branch file
    snprintf(new_ref_path, sizeof(new_ref_path), ".big/refs/%s", branch_name);

    // Check the bracnh name is exist or not
    if (access(new_ref_path, F_OK) == 0) {
        error_custom_msg("Error: Branch '%s' already exist\n", branch_name);
    }

    // To get current commit for branch point to
    char *leader_commit_hash = load_leader();

    FILE *ref_file = xfopen(new_ref_path, "w");

    // Write the commit hash into branch file
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

// Helper to delete branch
static void delete_branch(const char *branch_name) {
    char ref_path[4096];
    // Combine the path to branch file
    snprintf(ref_path, sizeof(ref_path), ".big/refs/%s", branch_name);

    // Check branch is exist or not
    if (access(ref_path, F_OK) != 0) {
        error_custom_msg("Error: Branch '%s' does not exist\n", branch_name);
    }

    // Remove branch from ref/
    if (remove(ref_path) == -1) {
        error_custom_msg("Error: Delete branch '%s' failed\n", branch_name);
    }
    printf("Branch '%s' deleted\n", branch_name);
}

void cmd_branch(int argc, char *argv[]) {
    // Check initalize first
    if (check_init() == false) {
        error_not_init();
    }

    // Back to project directory and save working directory first
    cd_to_project_root(NULL);

    char *curr_branch = load_current_branch();
    // Not commit can not do any branch operation
    if (curr_branch == NULL) {
        error_custom_msg("No commit\n");
    }

    // If just 'big branch' for input, print out all branches
    if (argc == 1) {
        print_branches(curr_branch);
        xfree(curr_branch);
        return;
    }
    xfree(curr_branch);

    // Check input has option or not
    // Delete the branch and end this function
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
    // This two name can not be branch name
    if (strcmp(argv[1], "Leader") == 0 || strcmp(argv[1], "temp_checkout_ref") == 0) {
        error_custom_msg("'%s' as name is not allowed\n", argv[1]);
    }

    // Create new branch
    create_branch(argv[1]);
}
