#ifndef INDEX_H
#define INDEX_H

#include "core/snapshot.h"

// Recurrsive process directory (root_path)'s files and directories
void process_path(snapshot_bst_t *bst, const char *root_path, snapshot_bst_t *leader_bst);

/* Save index file that store blob's hashes for 'add' command
 * index is snapshot of current project status
 */
void save_index_file(snapshot_bst_t *bst);

/* Read index using path and create a BST
 * This function is for read index list in commit objects
 */
snapshot_bst_t *read_index_file_from_path(const char *path);

// Read global index that update by 'add' command and create a BST
snapshot_bst_t *read_index_file();

// Process project directory that '.big' locate and create a BST include all project directory files
snapshot_bst_t *snapshot_bst_create_from_projectdir();

// Process project directory and create a BST include only directory's path
snapshot_bst_t *snapshot_bst_create_dir_path();

#endif