#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ds/stack.h"

typedef struct {
    int data;
} data_t;

int main() {
    stack_t *stack = stack_create();

    data_t data1, data2, data3;

    data1.data = 10;
    data2.data = 20;
    data3.data = 30;

    stack_push(stack, &data1);
    assert(((data_t *)stack_peek(stack))->data == 10);
    printf("push data: %d\n", data1.data);
    stack_push(stack, &data2);
    assert(((data_t *)stack_peek(stack))->data == 20);
    printf("push data: %d\n", data2.data);
    stack_push(stack, &data3);
    assert(((data_t *)stack_peek(stack))->data == 30);
    printf("push data: %d\n", data3.data);

    puts("");

    data_t *popped_data;
    int count = 0;
    while (is_stack_empty(stack) == false) {
        popped_data = stack_pop(stack);

        printf("popped data: %d\n", popped_data->data);

        if (count == 0) {
            assert(popped_data->data == 30);
        } else if (count == 1) {
            assert(popped_data->data == 20);
        } else {
            assert(popped_data->data == 10);
        }
        count++;
    }
    popped_data = stack_pop(stack);
    assert(popped_data == NULL);

    stack_free(&stack);

    assert(stack == NULL);
    return 0;
}