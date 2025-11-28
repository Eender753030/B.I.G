#include "core/snapshot.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/blob.h"
#include "ds/bst.h"
#include "utils/error_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

struct file_info {
    char *path;
    char *hash;
    bool is_changed;
};

static file_info_t *file_info_create(const char *path, snapshot_bst_t *leader_bst) {
    if (path == NULL) {
        return NULL;
    }

    file_info_t *new_file_info = xmalloc(sizeof(*new_file_info));

    new_file_info->path = str_dup(path);
    new_file_info->hash = blob_create_from_file(path);

    snapshot_node_t *leader_node = NULL;
    if (leader_bst != NULL && (leader_node = bst_search(leader_bst, new_file_info)) != NULL) {
        file_info_t *node_file_info = bst_node_get_data(leader_node);
        if (strcmp(new_file_info->hash, node_file_info->hash) == 0) {
            new_file_info->is_changed = false;
            return new_file_info;
        }
    }
    new_file_info->is_changed = true;
    return new_file_info;
}

static file_info_t *file_info_create_from_projectdir(const char *path) {
    if (path == NULL) {
        return NULL;
    }

    file_info_t *new_file_info = xmalloc(sizeof(*new_file_info));

    new_file_info->path = str_dup(path);
    new_file_info->hash = blob_get_file_hash(path);
    new_file_info->is_changed = true;
    return new_file_info;
}

file_info_t *file_info_create_from_index(const char *path, const char *hash, bool is_changed) {
    if (path == NULL || hash == NULL) {
        return NULL;
    }

    file_info_t *new_file_info = xmalloc(sizeof(*new_file_info));
    new_file_info->path = str_dup(path);
    new_file_info->hash = str_dup(hash);
    new_file_info->is_changed = is_changed;

    return new_file_info;
}

static int8_t path_cmp_func(void *file_info1, void *file_info2) {
    return (int8_t)strcmp(((file_info_t *)file_info1)->path, ((file_info_t *)file_info2)->path);
}

snapshot_bst_t *snapshot_bst_create() {
    return bst_create(path_cmp_func);
}

snapshot_bst_t *snapshot_bst_create_from_list(file_info_t **list, uint64_t size) {
    return bst_create_from_list((void **)list, size, path_cmp_func);
}

static void equal_handle_func(void *node, void *new_file_info) {
    file_info_t *file_info = bst_node_get_data(node);
    xfree(file_info->hash);

    file_info->hash = ((file_info_t *)new_file_info)->hash;
    file_info->is_changed = ((file_info_t *)new_file_info)->is_changed;
    xfree(((file_info_t *)new_file_info)->path);
    xfree(new_file_info);
}

void snapshot_bst_insert(snapshot_bst_t *self, const char *path, snapshot_bst_t *leader_bst) {
    if (self == NULL || path == NULL) {
        return;
    }

    file_info_t *new_file_info = file_info_create(path, leader_bst);

    bst_insert(self, new_file_info, equal_handle_func);
}

void snapshot_bst_insert_projectdir(snapshot_bst_t *self, const char *path) {
    if (self == NULL || path == NULL) {
        return;
    }

    file_info_t *new_file_info = file_info_create_from_projectdir(path);

    bst_insert(self, new_file_info, equal_handle_func);
}

static void file_info_free_func(void **file_info) {
    xfree(((file_info_t *)*file_info)->path);
    xfree(((file_info_t *)*file_info)->hash);
    xfree(*file_info);
}

void snapshot_bst_free(snapshot_bst_t **bst) {
    if (bst == NULL || *bst == NULL) {
        return;
    }
    bst_free(bst, file_info_free_func);
}

void file_info_get_content(file_info_t *file_info, char **path, char **hash, bool *is_changed) {
    if (file_info == NULL) {
        return;
    }
    if (path != NULL) {
        *path = file_info->path;
    }
    if (hash != NULL) {
        *hash = file_info->hash;
    }
    if (is_changed != NULL) {
        *is_changed = file_info->is_changed;
    }
}

bool is_same_file_info(void *file_info1, void *file_info2) {
    if (file_info1 == NULL && file_info2 == NULL) {
        return true;
    }
    if (file_info1 == NULL || file_info2 == NULL) {
        return false;
    }

    file_info_t *f1 = file_info1;
    file_info_t *f2 = file_info2;

    if (strcmp(f1->path, f2->path) != 0) {
        return false;
    }
    if (strcmp(f1->hash, f2->hash) != 0) {
        return false;
    }
    return true;
}