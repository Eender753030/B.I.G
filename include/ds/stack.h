#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

typedef struct stack stack_t;

stack_t *stack_create();

void stack_push(stack_t *self, void *data);

void *stack_pop(stack_t *self);

void *stack_peak(stack_t *self);

bool is_stack_empty(stack_t *self);

void stack_free(stack_t **self);

#endif