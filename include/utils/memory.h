#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

/* Internal malloc() handle
 * For error handle inside that can make me not need to type it every time
 * Use macro as API can pass outer file information instead of internal helper function's
 */
void *_xmalloc(size_t size, const char *func_name, const char *file_name, const int line);

/* Macro of above functions
 * Can pass GCC magic word from the caller
 */
#define xmalloc(size) _xmalloc((size), __func__, __FILE__, __LINE__)

/* Marco for set pointer to NULL after free
 * Use do while to prevent one line if, for, while statement
 */
#define xfree(ptr)    \
    do {              \
        free(ptr);    \
        (ptr) = NULL; \
    } while (0)

#endif