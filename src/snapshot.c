#include "snapshot.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_handle.h"

typedef struct FileInfo {
    char *content;
    char *path;  // Use path as search key
} FileInfo;

typedef struct SnapshotNode {
    FileInfo *file;  // TODO: If file wasn't changed. Direct point to earlier commit's FileInfo
    struct SnapshotNode *left;
    struct SnapshotNode *right;
} SnapshotNode;

struct SnapshotBST {
    SnapshotNode *root;
};

static char *str_dup(const char *string) {
    char *new_string = (char *)malloc(strlen(string) + 1);
    if (new_string == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    strcpy(new_string, string);

    return new_string;
}

static char *read_whole_file(const char *full_path) {
    FILE *file = fopen(full_path, "rb");
    if (file == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    fseek(file, 0, SEEK_END);
    size_t file_len = ftell(file);

    char *buffer = (char *)malloc(file_len + 1);
    if (buffer == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    rewind(file);

    size_t bytes_read = fread(buffer, 1, file_len, file);
    if (bytes_read != file_len)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    buffer[file_len] = '\0';

    fclose(file);

    return buffer;
}

static SnapshotNode *SnapshotNodeCreate(const char *path) {
    SnapshotNode *new_node = (SnapshotNode *)malloc(sizeof(SnapshotNode));
    if (new_node == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    new_node->file = (FileInfo *)malloc(sizeof(FileInfo));
    if (new_node->file == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    new_node->file->path = str_dup(path);
    new_node->file->content = read_whole_file(path);

    new_node->left = NULL;
    new_node->right = NULL;

    return new_node;
}

static SnapshotNode *path_list_build_BST(char **path_list, long long left, long long right) {
    if (left > right)
        return NULL;

    long long middle = left + (right - left) / 2;

    SnapshotNode *root = SnapshotNodeCreate(path_list[middle]);

    root->left = path_list_build_BST(path_list, left, middle - 1);

    root->right = path_list_build_BST(path_list, middle + 1, right);

    return root;
}

SnapshotBST *SnapshotBSTCreate(char **path_list, size_t list_len) {
    SnapshotBST *new_bst = (SnapshotBST *)malloc(sizeof(SnapshotBST));
    if (new_bst == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    new_bst->root = path_list_build_BST(path_list, 0, (long long)list_len - 1);

    return new_bst;
}

static void freeNode(SnapshotNode **node) {
    free((*node)->file->content);
    free((*node)->file->path);
    (*node)->file->content = NULL;
    (*node)->file->path = NULL;
    free((*node)->file);
    (*node)->file = NULL;
    free((*node));
    (*node) = NULL;
}

static int SnapshotBSTInsert(SnapshotBST **bst, const char *path) {
    SnapshotNode *new_node = SnapshotNodeCreate(path);

    if ((*bst)->root == NULL) {
        (*bst)->root = new_node;
        return 0;
    }

    SnapshotNode *current = (*bst)->root;
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

static void SnapshotNodesFree(SnapshotNode *node) {
    if (node == NULL)
        return;

    SnapshotNodesFree(node->left);
    SnapshotNodesFree(node->right);
    freeNode(&node);
}

void SnapshotBSTDestory(SnapshotBST **bst) {
    SnapshotNodesFree((*bst)->root);
    *bst = NULL;
}
