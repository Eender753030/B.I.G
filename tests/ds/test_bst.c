#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ds/bst.h"
#include "utils/memory.h"

typedef struct {
    int data;
} data_t;

static int32_t cmp_func(void* data1, void* data2) {
    data_t* data1_d = (data_t*)data1;
    data_t* data2_d = (data_t*)data2;
    return (data1_d->data < (data2_d->data) ? -1 : (data1_d->data > data2_d->data) ? 1 : 0);
}

static void equal_handle(void* node, void* data) {
    data_t* data_ = bst_node_get_data(node);
    printf("Drop equal data: %d == %d!\n", ((data_t*)data)->data, data_->data);
}

static void inoder_print(void* node) {
    printf("%d ", ((data_t*)bst_node_get_data((bst_node_t*)node))->data);
}

static void free_func(void** data) {
    xfree(*data);
    return;
}

int main() {
    data_t* data1 = xmalloc(sizeof(data1));
    data_t* data2 = xmalloc(sizeof(data2));
    data_t* data3 = xmalloc(sizeof(data3));
    data_t* data4 = xmalloc(sizeof(data4));
    data_t* data5 = xmalloc(sizeof(data5));
    data_t* data6 = xmalloc(sizeof(data6));
    data_t* data7 = xmalloc(sizeof(data7));

    bst_t* bst = bst_create(cmp_func);

    uint64_t size = 3;
    data_t** data_list = xmalloc(sizeof(*data_list) * size);
    data_list[0] = data5;
    data_list[1] = data6;
    data_list[2] = data7;

    bst_t* bst_list = bst_create_from_list((void**)data_list, size, cmp_func);

    data1->data = 10;
    data2->data = 20;
    data3->data = 30;
    data4->data = 40;
    data5->data = 50;
    data6->data = 60;
    data7->data = 70;

    bst_insert(bst, data1, equal_handle);
    bst_insert(bst, data2, equal_handle);
    bst_insert(bst, data3, equal_handle);
    bst_insert(bst, data1, equal_handle);

    bst_insert(bst_list, data5, equal_handle);
    bst_insert(bst_list, data4, equal_handle);

    bst_inorder_func(bst, inoder_print);
    printf("amount: %lu ", bst_get_amount(bst));
    puts("");
    bst_inorder_func(bst_list, inoder_print);
    printf("amount: %lu ", bst_get_amount(bst_list));
    puts("");

    printf("After delete\n");
    bst_delete(bst, data2, free_func);
    bst_delete(bst, data5, free_func);

    bst_delete(bst_list, data5, free_func);
    bst_delete(bst_list, data6, free_func);
    bst_inorder_func(bst, inoder_print);
    puts("");
    puts("");
    bst_inorder_func(bst_list, inoder_print);
    puts("");

    bst_node_t* searched_node = bst_search(bst, data3);
    assert(bst_node_get_data(searched_node) == data3);

    searched_node = bst_search(bst, data7);
    assert(searched_node == NULL);

    xfree(data_list);
    bst_free(&bst, free_func);
    bst_free(&bst_list, free_func);
    assert(bst == NULL);
    assert(bst_list == NULL);
}