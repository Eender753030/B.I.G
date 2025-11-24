#include "core/snapshot.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/color.h"
#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

typedef struct {
    char *path;
    bool changed;
    union {
        char *content;
        char *commit_id;
    } ref;
} FileInfo;

typedef struct SnapshotNode {
    FileInfo *file;
    struct SnapshotNode *left;
    struct SnapshotNode *right;
} SnapshotNode;

struct SnapshotBST {
    SnapshotNode *root;
    size_t node_amount;
};

// static void check_leader_commit() {
//     // TODO: Check leader files and make unchanged file point to it.
// }

static SnapshotNode *SnapshotNodeCreate(const char *path) {
    SnapshotNode *new_node = xmalloc(sizeof(*new_node));

    new_node->file = xmalloc(sizeof(*(new_node->file)));
    new_node->file->path = str_dup(path);
    new_node->file->ref.content = read_whole_file(path);  // TODO: Point to leader commit file
    new_node->file->changed = false;

    new_node->left = NULL;
    new_node->right = NULL;

    return new_node;
}

static SnapshotNode *path_list_build_BST(char **path_list, long long left, long long right) {
    if (left > right) {
        return NULL;
    }
    long long middle = left + (right - left) / 2;

    SnapshotNode *root = SnapshotNodeCreate(path_list[middle]);

    root->left = path_list_build_BST(path_list, left, middle - 1);
    root->right = path_list_build_BST(path_list, middle + 1, right);

    return root;
}

static SnapshotBST *SnapshotBSTCreate(char **path_list, size_t list_len) {
    SnapshotBST *new_bst = xmalloc(sizeof(*new_bst));
    new_bst->root = path_list_build_BST(path_list, 0, (long long)list_len - 1);

    return new_bst;
}

SnapshotBST *SnapshotBSTCreateEmpty() {
    SnapshotBST *new_bst = xmalloc(sizeof(*new_bst));
    new_bst->root = NULL;

    return new_bst;
}

static void freeNode(SnapshotNode **node) {
    if (node == NULL || *node == NULL) {
        return;
    }
    xfree((*node)->file->ref.content);
    xfree((*node)->file->path);
    xfree((*node)->file);
    xfree((*node));
}

int SnapshotBSTInsert(SnapshotBST *bst, const char *path) {
    SnapshotNode *new_node = SnapshotNodeCreate(path);
    if (bst->root == NULL) {
        bst->root = new_node;
        return 0;
    }
    SnapshotNode *current = bst->root;
    int cmp_result;
    while (current != NULL) {
        cmp_result = strcmp(new_node->file->path, current->file->path);

        if (cmp_result > 0) {
            if (current->right == NULL) {
                current->right = new_node;
                break;
            }
            current = current->right;

        } else if (cmp_result < 0) {
            if (current->left == NULL) {
                current->left = new_node;
                break;
            }
            current = current->left;

        } else {
            freeNode(&new_node);
            return -1;
        }
    }
    return 0;
}

int SnapshotBSTDelete(SnapshotBST *bst, const char *target_path, size_t *total_size) {
    if (bst == NULL || bst->root == NULL) {
        return NOT_FOUND;
    }
    SnapshotNode *parent = NULL;
    SnapshotNode *current = bst->root;
    int cmp_result;
    while (current != NULL && (cmp_result = strcmp(target_path, current->file->path)) != 0) {
        parent = current;
        if (cmp_result > 0) {
            current = current->right;
        } else if (cmp_result < 0) {
            current = current->left;
        }
    }
    if (current == NULL) {
        return NOT_FOUND;
    }
    if (current->left != NULL && current->right != NULL) {
        SnapshotNode *successor_parent = current;
        SnapshotNode *successor = current->right;
        while (successor->left != NULL) {
            successor_parent = successor;
            successor = successor->left;
        }
        xfree(current->file->path);
        xfree(current->file->ref.content);

        current->file->path = str_dup(successor->file->path);
        current->file->ref.content = str_dup(successor->file->ref.content);

        parent = successor_parent;
        current = successor;
    }

    SnapshotNode *child = (current->left != NULL) ? current->left : current->right;

    if (parent == NULL) {
        bst->root = child;
    } else {
        if (current == parent->left) {
            parent->left = child;
        } else {
            parent->right = child;
        }
    }

    (*total_size)--;
    freeNode(&current);
    return FOUND;
}

static void SnapshotNodesFree(SnapshotNode *node) {
    if (node == NULL) {
        return;
    }
    SnapshotNodesFree(node->left);
    SnapshotNodesFree(node->right);
    freeNode(&node);
}

void SnapshotBSTDestory(SnapshotBST **bst) {
    if (bst == NULL || *bst == NULL) {
        return;
    }
    SnapshotNodesFree((*bst)->root);
    xfree(*bst);
}

void process_path(SnapshotBST *bst, const char *root_path, size_t *list_length) {
    DIR *dir = opendir(root_path);
    if (dir == NULL) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    struct dirent *file_dirent;
    struct stat file_stat;

    while ((file_dirent = readdir(dir)) != NULL) {
        if (strncmp(file_dirent->d_name, ".", 2) == 0 ||
            strncmp(file_dirent->d_name, "..", 3) == 0 ||
            strncmp(file_dirent->d_name, ".big", 5) == 0 ||
            strncmp(file_dirent->d_name, "big", 4) == 0) {
            continue;
        }
        char pathbuffer[1024];

        if (strcmp(root_path, ".") == 0) {
            snprintf(pathbuffer, sizeof(pathbuffer), "%s", file_dirent->d_name);
        } else {
            snprintf(pathbuffer, sizeof(pathbuffer), "%s/%s", root_path, file_dirent->d_name);
        }

        if (stat(pathbuffer, &file_stat) == -1) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }

        if (S_ISDIR(file_stat.st_mode)) {
            process_path(bst, pathbuffer, list_length);
        } else {
            if (SnapshotBSTInsert(bst, pathbuffer) == 0) {
                (*list_length)++;
            }
        }
    }
    xfree(dir);
}

static void inorder_traversal_to_path_list(char ***list, SnapshotNode *node, size_t *idx) {
    if (node == NULL) {
        return;
    }
    inorder_traversal_to_path_list(list, node->left, idx);
    (*list)[*idx] = node->file->path;
    (*idx)++;
    inorder_traversal_to_path_list(list, node->right, idx);
}

static void _inorder_traversal_func_recu(SnapshotNode *node, void (*action)(SnapshotNode *)) {
    if (node == NULL) {
        return;
    }
    _inorder_traversal_func_recu(node->left, action);
    action(node);
    _inorder_traversal_func_recu(node->right, action);
}

void inorder_traversal_func(SnapshotBST *bst, void (*action)(SnapshotNode *)) {
    _inorder_traversal_func_recu(bst->root, action);
}

static void scan_and_create_files(SnapshotNode *node) {
    mk_dir_and_file(node->file->path, node->file->ref.content);
}

static void _inorder_traversal_print(SnapshotNode *node, const char *msg, const char *color) {
    if (node == NULL) {
        return;
    }
    _inorder_traversal_print(node->left, msg, color);
    printf("%s%s%s\n" COLOR_END, color, msg, node->file->path);
    _inorder_traversal_print(node->right, msg, color);
}

void inorder_traversal_print(SnapshotBST *bst, const char *msg, const char *color) {
    _inorder_traversal_print(bst->root, msg, color);
}

static void _inorder_traversal_delete(SnapshotBST *bst, SnapshotNode *node, size_t *total_size) {
    if (bst == NULL || node == NULL) {
        return;
    }
    _inorder_traversal_delete(bst, node->left, total_size);
    SnapshotBSTDelete(bst, node->file->path, total_size);
    _inorder_traversal_delete(bst, node->right, total_size);
}

void inorder_traversal_delete(SnapshotBST *target_bst, SnapshotBST *ref_bst,
                              size_t *target_total_size) {
    _inorder_traversal_delete(target_bst, ref_bst->root, target_total_size);
}

static void save_index_file_list(SnapshotBST *bst, size_t total_size) {
    if (bst == NULL || total_size == 0) {
        return;
    }
    FILE *index_file = xfopen("index_list", "w");

    char **path_list = xmalloc(sizeof(*path_list) * total_size);

    size_t idx = 0;
    inorder_traversal_to_path_list(&path_list, bst->root, &idx);

    if (idx != total_size) {
        ErrorCustomMsg("Error: save index file failed: path list size not match\n");
    }
    fprintf(index_file, "%ld\n", idx);

    for (size_t i = 0; i < idx; i++) {
        fprintf(index_file, "%s\n", path_list[i]);
    }
    fclose(index_file);
    xfree(path_list);
}

void save_index_dic(SnapshotBST *bst, size_t total_size) {
    if (bst == NULL || total_size == 0) {
        return;
    }

    cd_to_project_root(NULL);

    if (mkdir(".big/index", 0775) == -1) {
        if (errno != EEXIST) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }
    }

    if (mkdir(".big/index/root", 0775) == -1) {
        if (errno != EEXIST) {
            ErrnoHandler(__func__, __FILE__, __LINE__);
        }
    }

    if (chdir(".big/index/root") == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    inorder_traversal_func(bst, scan_and_create_files);

    if (chdir("..") == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    save_index_file_list(bst, total_size);
}

SnapshotBST *read_index_dic(size_t *total_size) {
    char org_dir[1024];
    if (getcwd(org_dir, sizeof(org_dir)) == NULL) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }
    cd_to_project_root(NULL);

    FILE *index_file = fopen(".big/index/index_list", "r");
    if (index_file == NULL) {
        SnapshotBST *bst = SnapshotBSTCreateEmpty();
        return bst;
    }

    fscanf(index_file, "%lu\n", total_size);

    if (total_size == 0) {
        SnapshotBST *bst = SnapshotBSTCreateEmpty();
        fclose(index_file);
        return bst;
    }

    char **path_list = xmalloc(sizeof(*path_list) * (*total_size));

    size_t count = 0;
    char buffer[256];
    while (fscanf(index_file, "%s\n", buffer) == 1) {
        if (access(buffer, F_OK) == -1) {
            (*total_size)--;
        } else {
            path_list[count++] = str_dup(buffer);
        }
    }

    SnapshotBST *bst = SnapshotBSTCreate(path_list, *total_size);

    fclose(index_file);

    for (size_t i = 0; i < count; i++) {
        xfree(path_list[i]);
    }
    xfree(path_list);

    chdir(org_dir);

    return bst;
}