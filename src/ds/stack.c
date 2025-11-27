#include "ds/stack.h"

#include <stdbool.h>
#include <stdint.h>

#include "utils/memory.h"

typedef struct stack_node {
    void *data;
    struct stack_node *next;
} stack_node_t;

struct stack {
    stack_node_t *top_node;
    uint64_t size;
};

stack_t *stack_create() {
    stack_t *new_stack = xmalloc(sizeof(*new_stack));

    new_stack->top_node = NULL;
    new_stack->size = 0;

    return new_stack;
}

void stack_push(stack_t *self, void *data) {
    if (self == NULL || data == NULL) {
        return;
    }
    stack_node_t *new_node = xmalloc(sizeof(*new_node));
    new_node->data = data;
    new_node->next = self->top_node;
    self->top_node = new_node;
    self->size++;
}

void *stack_pop(stack_t *self) {
    if (self == NULL || is_stack_empty(self)) {
        return NULL;
    }

    void *data = self->top_node->data;
    stack_node_t *temp = self->top_node;
    self->top_node = self->top_node->next;
    self->size--;
    xfree(temp);
    return data;
}

void *stack_peek(stack_t *self) {
    if (self == NULL || is_stack_empty(self)) {
        return NULL;
    }
    return self->top_node->data;
}

bool is_stack_empty(stack_t *self) {
    return self->size == 0;
}

void stack_free(stack_t **self) {
    if (self == NULL || *self == NULL) {
        return;
    }

    stack_node_t *curr = (*self)->top_node;
    stack_node_t *temp;
    while (curr != NULL) {
        temp = curr;
        curr = curr->next;
        xfree(temp);
    }
    xfree(*self);
}