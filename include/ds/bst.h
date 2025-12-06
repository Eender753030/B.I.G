#ifndef BST_H
#define BST_H

#include <stdbool.h>
#include <stdint.h>

// Type of BST node that does not provide structure detail for client
typedef struct bst_node bst_node_t;

// Type of BST that does not provide structure detail for client
typedef struct bst bst_t;

// Create and return a newly allocated node that with generic data
bst_node_t* bst_node_create(void* data);

// Create and return a new empty BST that require a comparsion function for binary search
bst_t* bst_create(int8_t (*cmp_callback)(void*, void*));

/* Create a new BST from a sorted list that can ensure search time is O(log n)
 * Also require a comparsion function for binary search
 */
bst_t* bst_create_from_list(void** list, uint64_t list_size, int8_t (*cmp_callback)(void*, void*));

/* Insert data into BST
 * Caller can decide whether handle the equal data situation for passing function pointer
 */
void bst_insert(bst_t* bst, void* data, void (*equal_handle_callback)(void*, void*));

// Search node by data
bst_node_t* bst_search(bst_t* bst, void* data);

/* Delete node by data
 * Require free function to tell how to free data and it's contents (if it has)
 */
void bst_delete(bst_t* bst, void* data, void (*free_func)(void**));

/* Check two BST is same or not
 * Require a function to know they are the same
 */
bool is_same_bst(bst_t* bst1, bst_t* bst2, bool (*is_same_callback)(void*, void*));

// Use inorder traversal to flatten BST to a sorted list
void** bst_inorder_to_list(bst_t* bst);

// Inorder traversal and can call caller's callback for passing data and args
void bst_inorder_func(bst_t* bst, void (*callback)(void*, void*), void* args);

/* Free single BST node
 * Require free function to tell how to free data and it's contents (if it has)
 */
void bst_node_free(bst_node_t** node, void (*free_callback)(void**));

/* Free whole BST
 * Require free function to tell how to free data and it's contents (if it has)
 */
void bst_free(bst_t** bst, void (*free_callback)(void**));

// Getter for get BST's root
bst_node_t* bst_get_root(bst_t* bst);

// Getter for get BST's node amount
uint64_t bst_get_amount(bst_t* bst);

// Getter for get BST node's data
void* bst_node_get_data(bst_node_t* node);

#endif
