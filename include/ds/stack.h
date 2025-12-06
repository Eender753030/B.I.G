#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

// Type of stack that does not provide structure detail for client
typedef struct stack stack_t;

// Create a empty stack
stack_t *stack_create();

// Push generic data to stack
void stack_push(stack_t *self, void *data);

// Pop data from stack's top and remove from stack
void *stack_pop(stack_t *self);

// Just see data from stack's top, no remove from stack
void *stack_peek(stack_t *self);

// Check is stack empty
bool is_stack_empty(stack_t *self);

// Free stack but not data
void stack_free(stack_t **self);

#endif