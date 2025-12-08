#include "ds/bst.h"

#include <stdint.h>
#include <stdlib.h>

#include "ds/stack.h"
#include "utils/memory.h"

/* BST node and BST structure implementaion */
struct bst_node {
    void* data;  // Generic data
    struct bst_node* left;
    struct bst_node* right;
};

struct bst {
    bst_node_t* root;
    uint64_t amount;                       // O(1) get amount
    int8_t (*cmp_callback)(void*, void*);  // Comparsion function for binary search
};

bst_node_t* bst_node_create(void* data) {
    if (data == NULL) {
        return NULL;
    }
    bst_node_t* new_node = xmalloc(sizeof(*new_node));

    new_node->data = data;
    new_node->left = NULL;
    new_node->right = NULL;

    return new_node;
}

/* Generic Binary Search Tree
 * For the BST data structure can be reused in future
 * I use void* pointer to store data,
 * and function pointer (cmp_callback) can let user decide how to compare data to build BST
 * That can make bst.c is completely independent, totally no need to know like "Index" or "Commit"
 */
bst_t* bst_create(int8_t (*cmp_callback)(void*, void*)) {
    bst_t* new_bst = xmalloc(sizeof(*new_bst));

    new_bst->root = NULL;
    new_bst->cmp_callback = cmp_callback;
    new_bst->amount = 0;
    return new_bst;
}

// Recurrsive helper use preorder to create a BST
static bst_node_t* _bst_create_from_list(void** list, int64_t left, int64_t right) {
    if (left > right) {
        return NULL;
    }
    // Make the middle to the root for every subtree
    int64_t middle = left + (right - left) / 2;

    bst_node_t* root = bst_node_create(list[middle]);

    root->left = _bst_create_from_list(list, left, middle - 1);
    root->right = _bst_create_from_list(list, middle + 1, right);

    return root;
}

// API for create a BST from sorted list, a balance BST that search time is O(log n)
bst_t* bst_create_from_list(void** list, uint64_t list_size, int8_t (*cmp_callback)(void*, void*)) {
    if (list == NULL) {
        return NULL;
    }
    bst_t* new_bst = bst_create(cmp_callback);
    new_bst->root = _bst_create_from_list(list, 0, (int64_t)list_size - 1);
    new_bst->amount = list_size;
    return new_bst;
}

void bst_insert(bst_t* bst, void* data, void (*equal_handle_callback)(void*, void*)) {
    if (bst == NULL || data == NULL) {
        return;
    }

    if (bst->root == NULL) {
        bst->root = bst_node_create(data);
        bst->amount++;
        return;
    }

    bst_node_t* curr = bst->root;
    // Iterative search to insert
    while (curr != NULL) {
        int8_t cmp_result = bst->cmp_callback(data, curr->data);
        if (cmp_result < 0) {
            if (curr->left == NULL) {
                curr->left = bst_node_create(data);
                bst->amount++;
                return;
            }
            curr = curr->left;
        } else if (cmp_result > 0) {
            if (curr->right == NULL) {
                curr->right = bst_node_create(data);
                bst->amount++;
                return;
            }
            curr = curr->right;

        } else {
            // If caller want to handle or just skip
            if (equal_handle_callback != NULL) {
                equal_handle_callback(curr, data);
            }
            return;
        }
    }
}

bst_node_t* bst_search(bst_t* bst, void* data) {
    if (bst == NULL || bst->root == NULL || data == NULL) {
        return NULL;
    }

    bst_node_t* curr = bst->root;
    // Iterative search
    while (curr != NULL) {
        int8_t cmp_result = bst->cmp_callback(data, curr->data);
        if (cmp_result < 0) {
            curr = curr->left;
        } else if (cmp_result > 0) {
            curr = curr->right;
        } else {
            return curr;
        }
    }
    return NULL;
}

void bst_delete(bst_t* bst, void* data, void (*free_callback)(void**)) {
    if (bst == NULL || bst->root == NULL || free_callback == NULL) {
        return;
    }
    bst_node_t* parent = NULL;
    bst_node_t* curr = bst->root;
    int8_t cmp_result;
    // Iterative search the node that be deleted
    while (curr != NULL && (cmp_result = bst->cmp_callback(data, curr->data)) != 0) {
        parent = curr;
        if (cmp_result < 0) {
            curr = curr->left;
        } else if (cmp_result > 0) {
            curr = curr->right;
        }
    }

    if (curr == NULL) {
        return;  // Not found
    }

    // Conditon of two child
    if (curr->left != NULL && curr->right != NULL) {
        // Find the successor (smallest of right subtree)
        bst_node_t* succ_parent = curr;
        bst_node_t* succ = curr->right;
        while (succ->left != NULL) {
            succ_parent = succ;
            succ = succ->left;
        }

        // Swap data
        free_callback(&(curr->data));
        curr->data = succ->data;
        succ->data = NULL;

        // Move delete target to successor
        parent = succ_parent;
        curr = succ;
    }

    bst_node_t* child = (curr->left != NULL) ? curr->left : curr->right;

    // Conditon of one child
    if (parent == NULL) {
        bst->root = child;  // No parent means is root
    } else {
        if (curr == parent->left) {
            parent->left = child;
        } else {
            parent->right = child;
        }
    }

    if (curr->data != NULL) {
        free_callback(&(curr->data));
    }
    xfree(curr);
    bst->amount--;
}

// Recurrsive check
bool _is_same_bst(bst_node_t* node1, bst_node_t* node2, bool (*is_same_callback)(void*, void*)) {
    if (node1 == NULL && node2 == NULL) {
        return true;
    }
    if (node1 == NULL || node2 == NULL) {
        return false;
    }

    return is_same_callback(node1->data, node2->data) &&
           _is_same_bst(node1->left, node2->left, is_same_callback) &&
           _is_same_bst(node1->right, node2->right, is_same_callback);
}

bool is_same_bst(bst_t* bst1, bst_t* bst2, bool (*is_same_callback)(void*, void*)) {
    if (bst1 == NULL && bst2 == NULL) {
        return true;
    }
    if (bst1 == NULL || bst2 == NULL) {
        return false;
    }
    if (bst1->amount != bst2->amount) {
        return false;
    }
    if (bst1->amount == 0) {
        return true;
    }

    return _is_same_bst(bst1->root, bst2->root, is_same_callback);
}

// Use iterative with stack to create a sorted list
void** bst_inorder_to_list(bst_t* bst) {
    if (bst == NULL || bst->root == NULL) {
        return NULL;
    }
    uint64_t idx = 0;
    uint64_t bst_amount = bst_get_amount(bst);
    void** list = xmalloc(sizeof(*list) * bst_amount);

    // Create a stack for iterative
    stack_t* stack = stack_create();
    bst_node_t* curr = bst->root;

    // Inorder with stack
    while (is_stack_empty(stack) == false || curr != NULL || idx < bst_amount) {
        // Push all left node to stack
        while (curr != NULL) {
            stack_push(stack, curr);
            curr = curr->left;
        }
        curr = stack_pop(stack);

        // Add to list
        list[idx++] = curr->data;

        curr = curr->right;
    }
    stack_free(&stack);  // Make sure to free stack
    return list;
}

// Use iterative with stack same as top
void bst_inorder_func(bst_t* bst, void (*callback)(void*, void*), void* args) {
    if (bst == NULL || bst->root == NULL || callback == NULL) {
        return;
    }

    stack_t* stack = stack_create();
    bst_node_t* curr = bst->root;

    while (is_stack_empty(stack) == false || curr != NULL) {
        while (curr != NULL) {
            stack_push(stack, curr);
            curr = curr->left;
        }
        curr = stack_pop(stack);

        // Call function pass data and args
        callback(curr->data, args);

        curr = curr->right;
    }
    stack_free(&stack);
}

void bst_node_free(bst_node_t** node, void (*free_callback)(void**)) {
    if (node == NULL || *node == NULL || free_callback == NULL) {
        return;
    }
    free_callback(&((*node)->data));
    xfree((*node));
}

// Recurrsive free
static void _bst_free(bst_node_t** node, void (*free_callback)(void**)) {
    if (node == NULL || *node == NULL || free_callback == NULL) {
        return;
    }
    _bst_free(&((*node)->left), free_callback);
    _bst_free(&((*node)->right), free_callback);
    bst_node_free(node, free_callback);
}

void bst_free(bst_t** bst, void (*free_callback)(void**)) {
    if (bst == NULL || *bst == NULL || free_callback == NULL) {
        return;
    }
    _bst_free(&((*bst)->root), free_callback);
    xfree((*bst));
}

bst_node_t* bst_get_root(bst_t* bst) {
    if (bst == NULL) {
        return NULL;
    }
    return bst->root;
}

uint64_t bst_get_amount(bst_t* bst) {
    if (bst == NULL) {
        return 0;
    }
    return bst->amount;
}

void* bst_node_get_data(bst_node_t* node) {
    if (node == NULL) {
        return NULL;
    }
    return node->data;
}