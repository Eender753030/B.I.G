#include "core/snapshot.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/commit_graph.h"
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

static SnapshotNode *SnapshotNodeCreate(const char *path, SnapshotBST *leader_bst,
                                        const char *leader_id) {
    SnapshotNode *new_node = xmalloc(sizeof(*new_node));

    new_node->file = xmalloc(sizeof(*(new_node->file)));
    new_node->file->path = str_dup(path);
    char *content = read_whole_file(path);
    if (leader_bst != NULL && leader_id != NULL &&
        SnapshotBST_Search_and_Compare(leader_bst, path, content) == MATCH) {
        xfree(content);
        SnapshotNode *point_node = SnapshotBSTSearch(leader_bst, path);
        if (strncmp(point_node->file->ref.content, "Point_to_commit:", 16) == 0) {
            new_node->file->ref.commit_id = str_dup(point_node->file->ref.commit_id);
        } else {
            char buffer[4096];
            snprintf(buffer, sizeof(buffer), "Point_to_commit: .big/objects/%s/root/%s", leader_id,
                     path);
            new_node->file->ref.commit_id = str_dup(buffer);
        }
        new_node->file->changed = false;
    } else {
        new_node->file->ref.content = content;
        new_node->file->changed = true;
    }

    new_node->left = NULL;
    new_node->right = NULL;

    return new_node;
}

static SnapshotNode *path_list_build_BST(char **path_list, long long left, long long right,
                                         SnapshotBST *leader_bst, const char *leader_id) {
    if (left > right) {
        return NULL;
    }
    long long middle = left + (right - left) / 2;

    SnapshotNode *root = SnapshotNodeCreate(path_list[middle], leader_bst, leader_id);

    root->left = path_list_build_BST(path_list, left, middle - 1, leader_bst, leader_id);
    root->right = path_list_build_BST(path_list, middle + 1, right, leader_bst, leader_id);

    return root;
}

static SnapshotBST *SnapshotBSTCreate(char **path_list, size_t list_len, SnapshotBST *leader_bst,
                                      const char *leader_id) {
    SnapshotBST *new_bst = xmalloc(sizeof(*new_bst));
    new_bst->node_amount = list_len;
    new_bst->root =
        path_list_build_BST(path_list, 0, (long long)list_len - 1, leader_bst, leader_id);

    return new_bst;
}

SnapshotBST *SnapshotBSTCreateEmpty() {
    SnapshotBST *new_bst = xmalloc(sizeof(*new_bst));
    new_bst->root = NULL;
    new_bst->node_amount = 0;
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

void SnapshotBSTInsert(SnapshotBST *bst, const char *path, SnapshotBST *leader_bst,
                       const char *leader_id) {
    if (bst->root == NULL) {
        bst->root = SnapshotNodeCreate(path, leader_bst, leader_id);
        bst->node_amount++;
        return;
    }
    SnapshotNode *current = bst->root;
    int cmp_result;
    while (current != NULL) {
        cmp_result = strcmp(path, current->file->path);
        if (cmp_result > 0) {
            if (current->right == NULL) {
                current->right = SnapshotNodeCreate(path, leader_bst, leader_id);
                bst->node_amount++;
                return;
            }
            current = current->right;

        } else if (cmp_result < 0) {
            if (current->left == NULL) {
                current->left = SnapshotNodeCreate(path, leader_bst, leader_id);
                bst->node_amount++;

                return;
            }
            current = current->left;

        } else {
            char *added_content = read_whole_file(path);
            if (strcmp(current->file->ref.content, added_content) != 0) {
                xfree(current->file->ref.content);
                current->file->ref.content = added_content;
                return;
            }
            xfree(added_content);
            return;
        }
    }
}

static char *parse_commit_pointer(const char *content) {
    if (strncmp(content, "Point_to_commit:", 16) == 0) {
        char *org_dir;
        cd_to_project_root(&org_dir);
        char *commit_path = strchr(content, ' ') + 1;
        char *org_content = read_whole_file(commit_path);
        chdir(org_dir);
        xfree(org_dir);
        return org_content;
    }
    return str_dup(content);
}

SnapshotNode *SnapshotBSTSearch(SnapshotBST *bst, const char *path) {
    if (bst->root == NULL) {
        return NULL;
    }
    SnapshotNode *current = bst->root;
    int cmp_result;
    while (current != NULL) {
        cmp_result = strcmp(path, current->file->path);
        if (cmp_result > 0) {
            current = current->right;
        } else if (cmp_result < 0) {
            current = current->left;
        } else {
            return current;
        }
    }
    return NULL;
}

int SnapshotBST_Search_and_Compare(SnapshotBST *bst, const char *path, const char *content) {
    if (bst->root == NULL) {
        return NOT_FOUND;
    }
    SnapshotNode *current = bst->root;
    int cmp_result;
    while (current != NULL) {
        cmp_result = strcmp(path, current->file->path);
        if (cmp_result > 0) {
            current = current->right;
        } else if (cmp_result < 0) {
            current = current->left;
        } else {
            char *commit_pointer_content = parse_commit_pointer(current->file->ref.content);
            int content_cmp = strcmp(commit_pointer_content, content);
            xfree(commit_pointer_content);
            if (content_cmp == 0) {
                return MATCH;
            }
            return NOT_MATCH;
        }
    }
    return NOT_FOUND;
}

void SnapshotBSTDelete(SnapshotBST *bst, const char *target_path) {
    if (bst == NULL || bst->root == NULL) {
        return;
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
        return;
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

    bst->node_amount--;
    freeNode(&current);
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

void process_path(SnapshotBST *bst, const char *root_path, SnapshotBST *leader_bst,
                  const char *leader_id) {
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
            process_path(bst, pathbuffer, leader_bst, leader_id);
        } else {
            SnapshotBSTInsert(bst, pathbuffer, leader_bst, leader_id);
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

static void _inorder_traversal_delete(SnapshotBST *bst, SnapshotNode *node) {
    if (bst == NULL || node == NULL) {
        return;
    }
    _inorder_traversal_delete(bst, node->left);
    SnapshotBSTDelete(bst, node->file->path);
    _inorder_traversal_delete(bst, node->right);
}

void inorder_traversal_delete(SnapshotBST *target_bst, SnapshotBST *ref_bst) {
    _inorder_traversal_delete(target_bst, ref_bst->root);
}

static void _inorder_traversal_search_and_compare(SnapshotBST *bst, SnapshotNode *node,
                                                  void (*action)(const char *, int)) {
    if (bst == NULL || node == NULL) {
        return;
    }
    _inorder_traversal_search_and_compare(bst, node->left, action);
    int result = SnapshotBST_Search_and_Compare(bst, node->file->path, node->file->ref.content);
    action(node->file->path, result);
    _inorder_traversal_search_and_compare(bst, node->right, action);
}

void compare_two_trees(SnapshotBST *main_bst, SnapshotBST *ref_bst,
                       void (*action)(const char *, int)) {
    _inorder_traversal_search_and_compare(main_bst, ref_bst->root, action);
}

static int _is_same_tree(SnapshotNode *node1, SnapshotNode *node2) {
    if (node1 == NULL && node2 == NULL) {
        return MATCH;
    }
    if (node1 == NULL || node2 == NULL) {
        return NOT_MATCH;
    }
    if (strcmp(node1->file->path, node2->file->path) != 0) {
        return NOT_MATCH;
    }
    char *node1_content = parse_commit_pointer(node1->file->ref.content);
    char *node2_content = parse_commit_pointer(node2->file->ref.content);
    int content_cmp_result = strcmp(node1_content, node2_content);
    xfree(node1_content);
    xfree(node2_content);
    if (content_cmp_result != 0) {
        return NOT_MATCH;
    }
    if (_is_same_tree(node1->left, node2->left) == MATCH &&
        _is_same_tree(node1->right, node2->right) == MATCH) {
        return MATCH;
    }
    return NOT_MATCH;
}

int is_same_tree(SnapshotBST *bst1, SnapshotBST *bst2) {
    return _is_same_tree(bst1->root, bst2->root);
}

static void save_index_file_list(SnapshotBST *bst) {
    if (bst == NULL || bst->node_amount == 0) {
        return;
    }
    FILE *index_file = xfopen("index_list", "w");

    char **path_list = xmalloc(sizeof(*path_list) * bst->node_amount);

    size_t idx = 0;
    inorder_traversal_to_path_list(&path_list, bst->root, &idx);

    if (idx != bst->node_amount) {
        ErrorCustomMsg("Error: save index file failed: path list size not match\n");
    }
    fprintf(index_file, "%ld\n", idx);

    for (size_t i = 0; i < idx; i++) {
        fprintf(index_file, "%s\n", path_list[i]);
    }
    fclose(index_file);
    xfree(path_list);
}

void save_index_dic(SnapshotBST *bst) {
    if (bst == NULL || bst->node_amount == 0) {
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

    save_index_file_list(bst);
}

SnapshotBST *read_index_dic(SnapshotBST *leader_bst, const char *leader_id) {
    char *org_dir;
    cd_to_project_root(&org_dir);

    if (chdir(".big/index/root") == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    FILE *index_file = fopen("../index_list", "r");
    if (index_file == NULL) {
        SnapshotBST *bst = SnapshotBSTCreateEmpty();
        return bst;
    }

    size_t total_size;
    fscanf(index_file, "%lu\n", &total_size);

    if (total_size == 0) {
        SnapshotBST *bst = SnapshotBSTCreateEmpty();
        fclose(index_file);
        return bst;
    }

    char **path_list = xmalloc(sizeof(*path_list) * total_size);

    size_t count = 0;
    char buffer[4096];
    while (fscanf(index_file, "%s\n", buffer) == 1) {
        if (access(buffer, F_OK) == -1) {
            total_size--;
        } else {
            path_list[count++] = str_dup(buffer);
        }
    }

    fclose(index_file);

    SnapshotBST *bst = SnapshotBSTCreate(path_list, total_size, leader_bst, leader_id);

    for (size_t i = 0; i < count; i++) {
        xfree(path_list[i]);
    }
    xfree(path_list);

    chdir(org_dir);
    xfree(org_dir);
    return bst;
}

SnapshotBST *read_leader_commit_BST(char **leader_id) {
    *leader_id = load_leader();
    if (*leader_id == NULL) {
        return NULL;
    }

    char leader_dir[4096];
    snprintf(leader_dir, sizeof(leader_dir), ".big/objects/%s/root", *leader_id);
    if (chdir(leader_dir) == -1) {
        ErrnoHandler(__func__, __FILE__, __LINE__);
    }

    FILE *leader_commit_list = xfopen("../list", "r");

    size_t total_size;
    fscanf(leader_commit_list, "%lu\n", &total_size);

    char **path_list = xmalloc(sizeof(*path_list) * total_size);

    size_t count = 0;
    char buffer[4096];
    while (fscanf(leader_commit_list, "%s\n", buffer) == 1) {
        if (access(buffer, F_OK) == -1) {
            total_size--;
        } else {
            path_list[count++] = str_dup(buffer);
        }
    }

    SnapshotBST *leader_bst = SnapshotBSTCreate(path_list, total_size, NULL, NULL);
    fclose(leader_commit_list);
    for (size_t i = 0; i < count; i++) {
        xfree(path_list[i]);
    }
    xfree(path_list);

    cd_to_project_root(NULL);
    return leader_bst;
}

size_t amount_of_BST(SnapshotBST *bst) {
    return bst->node_amount;
}

void path_and_content_of_node(SnapshotNode *node, char **path, char **content) {
    if (path != NULL) {
        *path = node->file->path;
    }
    if (content != NULL) {
        *content = node->file->ref.content;
    }
}