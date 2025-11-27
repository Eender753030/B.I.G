#include "ds/bst.h"

#include <stdint.h>
#include <stdlib.h>

#include "ds/stack.h"
#include "utils/memory.h"

struct bst_node {
    void* data;
    struct bst_node* left;
    struct bst_node* right;
};

struct bst {
    bst_node_t* root;
    uint64_t amount;
    int8_t (*cmp_func)(void*, void*);
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

bst_t* bst_create(int8_t (*cmp_func)(void*, void*)) {
    bst_t* new_bst = xmalloc(sizeof(*new_bst));

    new_bst->root = NULL;
    new_bst->cmp_func = cmp_func;
    new_bst->amount = 0;
    return new_bst;
}

static bst_node_t* _bst_create_from_list(void** list, int64_t left, int64_t right,
                                         uint64_t* amount) {
    if (left > right) {
        return NULL;
    }
    int64_t middle = left + (right - left) / 2;

    bst_node_t* root = bst_node_create(list[middle]);

    root->left = _bst_create_from_list(list, left, middle - 1, amount);
    root->right = _bst_create_from_list(list, middle + 1, right, amount);

    return root;
}

bst_t* bst_create_from_list(void** list, uint64_t list_size, int8_t (*cmp_func)(void*, void*)) {
    if (list == NULL) {
        return NULL;
    }
    bst_t* new_bst = bst_create(cmp_func);
    new_bst->root = _bst_create_from_list(list, 0, (int64_t)list_size - 1, &(new_bst->amount));
    return new_bst;
}

void bst_insert(bst_t* bst, void* data, void (*equal_handle)(void*)) {
    if (bst == NULL || data == NULL) {
        return;
    }

    if (bst->root == NULL) {
        bst->root = bst_node_create(data);
        bst->amount++;
        return;
    }

    bst_node_t* curr = bst->root;
    while (curr != NULL) {
        int8_t cmp_result = bst->cmp_func(data, curr->data);
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
            if (equal_handle != NULL) {
                equal_handle(data);
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
    while (curr != NULL) {
        int8_t cmp_result = bst->cmp_func(data, curr->data);
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

void bst_delete(bst_t* bst, void* data) {
    if (bst == NULL || bst->root == NULL) {
        return;
    }
    bst_node_t* parent = NULL;
    bst_node_t* curr = bst->root;
    int8_t cmp_result;
    while (curr != NULL && (cmp_result = bst->cmp_func(data, curr->data)) != 0) {
        parent = curr;
        if (cmp_result < 0) {
            curr = curr->left;
        } else if (cmp_result > 0) {
            curr = curr->right;
        }
    }

    if (curr == NULL) {
        return;
    }
    if (curr->left != NULL && curr->right != NULL) {
        bst_node_t* succ_parent = curr;
        bst_node_t* succ = curr->right;
        while (succ->left != NULL) {
            succ_parent = succ;
            succ = succ->left;
        }

        xfree(curr->data);
        curr->data = succ->data;
        succ->data = NULL;

        parent = succ_parent;
        curr = succ;
    }

    bst_node_t* child = (curr->left != NULL) ? curr->left : curr->right;

    if (parent == NULL) {
        bst->root = child;
    } else {
        if (curr == parent->left) {
            parent->left = child;
        } else {
            parent->right = child;
        }
    }

    bst->amount--;
    if (curr->data != NULL) {
        xfree(curr->data);
    }
    xfree(curr);
}

void bst_inorder_func(bst_t* bst, void (*func)(void*)) {
    if (bst == NULL || bst->root == NULL) {
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
        func(curr->data);
        curr = curr->right;
    }
    stack_free(&stack);
}

void bst_node_free(bst_node_t** node) {
    if (node == NULL || *node == NULL) {
        return;
    }
    bst_node_free(&((*node)->left));
    bst_node_free(&((*node)->right));
    xfree((*node)->data);
    xfree((*node));
}

void bst_free(bst_t** bst) {
    if (bst == NULL || *bst == NULL) {
        return;
    }
    bst_node_free(&((*bst)->root));
    xfree((*bst));
}

bst_node_t* bst_get_root(bst_t* bst) {
    return bst->root;
}

void* bst_node_get_data(bst_node_t* node) {
    return node->data;
}