#ifndef BST_H
#define BST_H

#include <stdint.h>

typedef struct bst_node bst_node_t;

typedef struct bst bst_t;

bst_node_t* bst_node_create(void* data);

bst_t* bst_create(int8_t (*cmp_callback)(void*, void*));

bst_t* bst_create_from_list(void** list, uint64_t list_size, int8_t (*cmp_callback)(void*, void*));

void bst_insert(bst_t* bst, void* data, void (*equal_handle_callback)(void*, void*));

bst_node_t* bst_search(bst_t* bst, void* data);

void bst_delete(bst_t* bst, void* data);

void bst_inorder_func(bst_t* bst, void (*callback)(void*, void*), void* args);

void bst_node_free(bst_node_t** node, void (*free_callback)(void**));

void bst_free(bst_t** bst, void (*free_callback)(void**));

bst_node_t* bst_get_root(bst_t* bst);

void* bst_node_get_data(bst_node_t* node);

#endif
