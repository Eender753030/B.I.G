#include "snapshot.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error_handle.h"
#include "utils.h"

typedef struct SnapshotNode {
    FileInfo *file;  // TODO: If file wasn't changed. Direct point to earlier commit's FileInfo
    struct SnapshotNode *left;
    struct SnapshotNode *right;
} SnapshotNode;

struct SnapshotBST {
    SnapshotNode *root;
};

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

static SnapshotBST *SnapshotBSTCreate(char **path_list, size_t list_len) {
    SnapshotBST *new_bst = (SnapshotBST *)malloc(sizeof(SnapshotBST));
    if (new_bst == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    new_bst->root = path_list_build_BST(path_list, 0, (long long)list_len - 1);

    return new_bst;
}

static SnapshotBST *SnapshotBSTCreateEmpty() {
    SnapshotBST *new_bst = (SnapshotBST *)malloc(sizeof(SnapshotBST));
    if (new_bst == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);
    new_bst->root = NULL;
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
    if (bst == NULL || *bst == NULL)
        return;

    SnapshotNodesFree((*bst)->root);
    free(*bst);
    *bst = NULL;
}

void process_path(SnapshotBST **bst, const char *root_path, size_t *list_length) {
    DIR *dir = opendir(root_path);
    if (dir == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    struct dirent *file_dirent;
    struct stat *file_stat = (struct stat *)malloc(sizeof(struct stat));
    if (file_stat == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    while ((file_dirent = readdir(dir)) != NULL) {
        if (strncmp(file_dirent->d_name, ".", 2) == 0 ||
            strncmp(file_dirent->d_name, "..", 3) == 0 ||
            strncmp(file_dirent->d_name, ".big", 5) == 0 ||
            strncmp(file_dirent->d_name, "big", 4) == 0)
            continue;

        char pathbuffer[1024];

        if (strcmp(root_path, ".") == 0)
            snprintf(pathbuffer, sizeof(pathbuffer), "%s", file_dirent->d_name);
        else
            snprintf(pathbuffer, sizeof(pathbuffer), "%s/%s", root_path, file_dirent->d_name);

        if (stat(pathbuffer, file_stat) == -1)
            ErrnoHandler(__func__, __FILE__, __LINE__);

        if (S_ISDIR(file_stat->st_mode))
            process_path(bst, pathbuffer, list_length);
        else {
            if (SnapshotBSTInsert(bst, pathbuffer) == 0)
                (*list_length)++;
        }
    }
    free(file_stat);
    file_stat = NULL;
    free(dir);
    dir = NULL;
}

static void inorder_traversal_to_path_list(char ***list, SnapshotNode *node, size_t *idx) {
    if (node == NULL)
        return;

    inorder_traversal_to_path_list(list, node->left, idx);
    (*list)[*idx] = node->file->path;
    (*idx)++;
    inorder_traversal_to_path_list(list, node->right, idx);
}

static void _inorder_traversal_func_recu(SnapshotNode *node, void (*action)(SnapshotNode *)) {
    if (node == NULL)
        return;
    _inorder_traversal_func_recu(node->left, action);
    action(node);
    _inorder_traversal_func_recu(node->right, action);
}

void inorder_traversal_func(SnapshotBST *bst, void (*action)(SnapshotNode *)) {
    _inorder_traversal_func_recu(bst->root, action);
}

static void save_index_file(SnapshotBST *bst, size_t total_size) {
    cd_to_project_root(NULL);

    FILE *index_file = fopen(".big/index", "w");
    if (index_file == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    char **path_list = (char **)malloc(sizeof(char *) * total_size);
    if (path_list == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    size_t idx = 0;
    inorder_traversal_to_path_list(&path_list, bst->root, &idx);

    if (idx != total_size)
        ErrorCustomMsg("Error: save index file failed: path list size not match\n");

    fprintf(index_file, "%ld\n", idx);
    for (size_t i = 0; i < idx; i++) fprintf(index_file, "%s\n", path_list[i]);

    fclose(index_file);
    free(path_list);
    path_list = NULL;
}

SnapshotBST *read_index_file(size_t *total_size) {
    char org_dir[1024];
    if (getcwd(org_dir, sizeof(org_dir)) == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    cd_to_project_root(NULL);

    FILE *index_file = fopen(".big/index", "r");
    if (index_file == NULL) {
        SnapshotBST *bst = SnapshotBSTCreateEmpty();
        return bst;
    }

    fscanf(index_file, "%ld\n", total_size);

    if (total_size == 0) {
        SnapshotBST *bst = SnapshotBSTCreateEmpty();
        fclose(index_file);
        return bst;
    }

    char **path_list = (char **)malloc(sizeof(char *) * *total_size);
    if (path_list == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    size_t count = 0;
    char buffer[256];
    while (fscanf(index_file, "%s\n", buffer) == 1) {
        if (access(buffer, F_OK) == -1)
            (*total_size)--;
        else
            path_list[count++] = str_dup(buffer);
    }

    SnapshotBST *bst = SnapshotBSTCreate(path_list, *total_size);

    fclose(index_file);

    for (size_t i = 0; i < count; i++) {
        free(path_list[i]);
        path_list[i] = NULL;
    }
    free(path_list);
    path_list = NULL;

    chdir(org_dir);

    return bst;
}

void add(size_t input_size, const char **root_path_list) {
    for (size_t i = 0; i < input_size; i++) {
        if (access(root_path_list[i], F_OK) == -1) {
            ErrorCustomMsg("Error: '%s' did not match to any file or directory.\n",
                           root_path_list[i]);
        }
        if (access(".big", F_OK) == 0 && strncmp(root_path_list[i], "..", 3) == 0)
            ErrorCustomMsg("Error: '..' is outside project directory.\n");
    }

    char *org_dir;
    cd_to_project_root(&org_dir);

    char root_dir[1024];
    if (getcwd(root_dir, 1024) == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    size_t total_size = 0;

    SnapshotBST *bst = read_index_file(&total_size);

    struct stat *file_stat = (struct stat *)malloc(sizeof(struct stat));
    if (file_stat == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    for (size_t i = 0; i < input_size; i++) {
        char normalized_path[2048];

        char temp_path[2048];
        snprintf(temp_path, sizeof(temp_path), "%s/%s", org_dir, root_path_list[i]);

        char absolute_path[2048];
        if (realpath(temp_path, absolute_path) == NULL)
            ErrnoHandler(__func__, __FILE__, __LINE__);
        char *relative_path = strstr(absolute_path, root_dir);

        if (relative_path != NULL && strcmp(relative_path, root_dir) != 0) {
            relative_path += strlen(root_dir) + 1;
            strcpy(normalized_path, relative_path);
        } else {
            strcpy(normalized_path, ".");
        }

        if (strncmp(normalized_path, ".", 2) == 0)
            process_path(&bst, ".", &total_size);
        else {
            if (stat(normalized_path, file_stat) == -1)
                ErrnoHandler(__func__, __FILE__, __LINE__);

            if (S_ISDIR(file_stat->st_mode))
                process_path(&bst, normalized_path, &total_size);
            else {
                if (SnapshotBSTInsert(&bst, normalized_path) == 0)
                    total_size++;
            }
        }
    }

    save_index_file(bst, total_size);

    free(org_dir);
    free(file_stat);
    file_stat = NULL;
    SnapshotBSTDestory(&bst);
}

FileInfo *get_fileinfo(SnapshotNode *node) {
    return node->file;
}