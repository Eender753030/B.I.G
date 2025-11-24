#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

/**
 * @brief Internal function to allocate memory with error handling.
 * NOTE: Use the xmalloc() macro instead of calling this directly.
 */
void *_xmalloc(size_t size, const char *func_name, const char *file_name, const int line);

/**
 * @brief Macro for safe memory allocation.
 * Automatically passes the current function, file, and line number for error reporting.
 */
#define xmalloc(size) _xmalloc((size), __func__, __FILE__, __LINE__)

/**
 * @brief Macro for safe memory free.
 * This prevents double-free errors and use-after-free bug
 */
#define xfree(ptr)    \
    do {              \
        free(ptr);    \
        (ptr) = NULL; \
    } while (0)

#endif