#include "utils/memory.h"

#include <stdlib.h>

#include "utils/error_handle.h"

void *_xmalloc(size_t size, const char *func_name, const char *file_name, const int line) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        ErrnoHandler(func_name, file_name, line);
    }
    return ptr;
}