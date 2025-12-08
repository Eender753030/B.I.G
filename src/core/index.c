#include "core/index.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "core/snapshot.h"
#include "ds/bst.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"

// A helper function to decide which action from _process_handler to do
static void _process_parser(snapshot_bst_t *bst, const char *path, bool on_file, bool on_dir,
                            bool have_leader, void *args) {
    // If has args (leader_bst) and on file, means it is from process_path()
    // Insert for global index
    if (on_file == true && have_leader == true) {
        snapshot_bst_insert(bst, path, (snapshot_bst_t *)args);
    }
    // If it only on file, means it is from snapshot_bst_create_from_projectdir()
    // Insert for project directory BST
    else if (on_file == true) {
        snapshot_bst_insert_projectdir(bst, path);
    }
    // If it only on dir, means it is from snapshot_bst_create_dir_path()
    // Insert for only directories path BST
    else if (on_dir == true) {
        snapshot_bst_insert_only_path(bst, path);
    }
}

/* Recursive Directory Traversal
 * We need to scan directory in commands 'add', 'status', and 'checkout'
 * There are three different BST use in those commands
 * I use <dirent.h> with recursive to traversal directory
 * And use <sys/stat.h> to judge is file or directory
 */
static void _process_handler(snapshot_bst_t *bst, const char *root_path, bool on_file, bool on_dir,
                             bool have_leader, void *args) {
    // Open root directory
    DIR *dir = opendir(root_path);
    if (dir == NULL) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
    struct dirent *file_dirent;
    struct stat file_stat;

    // Start to checking all files and directories
    while ((file_dirent = readdir(dir)) != NULL) {
        // Skip if meet these
        if (strcmp(file_dirent->d_name, ".") == 0 || strcmp(file_dirent->d_name, "..") == 0 ||
            strcmp(file_dirent->d_name, ".big") == 0 || strcmp(file_dirent->d_name, "big") == 0) {
            continue;
        }
        char pathbuffer[1024];
        char *c;

        // Combime the root directory path and next file or directory path
        if (strcmp(root_path, ".") == 0) {
            snprintf(pathbuffer, sizeof(pathbuffer), "%s", file_dirent->d_name);
        } else if ((c = strrchr(root_path, '/')) != NULL && *(c + 1) == '\0') {
            snprintf(pathbuffer, sizeof(pathbuffer), "%s%s", root_path, file_dirent->d_name);

        } else {
            snprintf(pathbuffer, sizeof(pathbuffer), "%s/%s", root_path, file_dirent->d_name);
        }

        // Get file's or directory's status
        if (stat(pathbuffer, &file_stat) == -1) {
            errno_handle(__func__, __FILE__, __LINE__);
        }
        // Check is directory
        if (S_ISDIR(file_stat.st_mode)) {
            // If is, keep go into the directory
            _process_handler(bst, pathbuffer, on_file, on_dir, have_leader, args);
            // Parse the action on directory
            if (on_dir == true) {
                _process_parser(bst, pathbuffer, NULL, on_dir, have_leader, args);
            }
        } else {
            // This means is a file, parse the action on file
            if (on_file == true) {
                _process_parser(bst, pathbuffer, on_file, NULL, have_leader, args);
            }
        }
    }
    // Use closedir() instead of free()
    closedir(dir);
}

// Helper function that write index file content
// Use in inorder travesal make index file is sorted
static void write_index(void *file_info, void *file_t) {
    char *path, *hash;
    bool is_changed;
    // Get file info that contains path, hash, and is_changed data
    file_info_get_content(file_info, &path, &hash, &is_changed);
    // write all into file_t file
    if (fprintf(file_t, "%s\t%s\t%d\n", path, hash, is_changed) == -1) {
        errno_handle(__func__, __FILE__, __LINE__);
    }
}

void save_index_file(snapshot_bst_t *bst) {
    if (bst == NULL) {
        return;
    }

    uint64_t bst_amount = bst_get_amount(bst);

    FILE *index_file = xfopen(".big/index", "w");

    // First line in index file is the amount of path
    if (fprintf(index_file, "%lu\n", bst_amount) == -1) {
        errno_handle(__func__, __FILE__, __LINE__);
    }

    // Pass write_index() function pointer make index file's content is sorted
    if (bst_amount != 0) {
        bst_inorder_func(bst, write_index, index_file);
    }
    fclose(index_file);
}

//
snapshot_bst_t *read_index_file_from_path(const char *path) {
    FILE *index_file = fopen(path, "r");
    // If index not exist, just return a empty BST
    if (index_file == NULL) {
        return snapshot_bst_create();
    }

    uint64_t total_size;
    // Read first line of index to get the amount of path
    if (fscanf(index_file, "%lu\n", &total_size) == -1) {
        fclose(index_file);
        return snapshot_bst_create();
    }

    if (total_size == 0) {
        fclose(index_file);
        return snapshot_bst_create();
    }

    // Malloc() a file_info list to store all path, hash, and is_changed data for create a BST
    file_info_t **file_info_list = xmalloc(sizeof(*file_info_list) * total_size);

    uint64_t idx = 0;
    char path_buffer[960];
    char hash_buffer[64];
    int is_changed_temp;
    // Loop read lines of content in index
    while (idx < total_size && fscanf(index_file, "%959[^\t]\t%s\t%d\n", path_buffer, hash_buffer,
                                      &is_changed_temp) == 3) {
        // Add all data to list
        file_info_list[idx++] =
            file_info_create_from_index(path_buffer, hash_buffer, is_changed_temp != 0);
    }

    // Check the amount is correct
    if (idx != total_size) {
        warning_custom_msg("Warning: index file mismatch\n");
    }

    fclose(index_file);

    // Create a balaned BST from sorted index
    snapshot_bst_t *new_bst = snapshot_bst_create_from_list(file_info_list, total_size);

    xfree(file_info_list);

    return new_bst;
}

// Call read_index_file_from_path() use path of global index
snapshot_bst_t *read_index_file() {
    return read_index_file_from_path(".big/index");
}

// For 'add' command use to add files into index
void process_path(snapshot_bst_t *bst, const char *path, snapshot_bst_t *leader_bst) {
    if (bst == NULL || path == NULL) {
        return;
    }

    _process_handler(bst, path, true, false, true, (void *)leader_bst);
}

// Create a BST of project directory for 'status' command to compare
snapshot_bst_t *snapshot_bst_create_from_projectdir() {
    snapshot_bst_t *new_dir_bst = snapshot_bst_create();
    _process_handler(new_dir_bst, ".", true, false, false, NULL);
    return new_dir_bst;
}

// Create a BST of only directory path for 'checkout' command to delete directory
snapshot_bst_t *snapshot_bst_create_dir_path() {
    snapshot_bst_t *new_dir_path_bst = snapshot_bst_create();
    _process_handler(new_dir_path_bst, ".", false, true, false, NULL);
    return new_dir_path_bst;
}