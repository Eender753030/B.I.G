#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

void *_xmalloc(size_t size, const char *func_name, const char *file_name, const int line);

#define xmalloc(size) _xmalloc((size), __func__, __FILE__, __LINE__)

#define xfree(ptr)    \
    do {              \
        free(ptr);    \
        (ptr) = NULL; \
    } while (0)

#endif