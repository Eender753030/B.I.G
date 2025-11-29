#include "utils/memory.h"

#include <stdlib.h>

#include "utils/error_handle.h"

void *_xmalloc(size_t size, const char *func_name, const char *file_name, const int line) {
    void *ptr = malloc(size);
    // If malloc fails (returns NULL), we trigger the error handler immediately.
    // This simplifies code elsewhere since we don't need to check for NULL every time.
    if (ptr == NULL) {
        errno_handle(func_name, file_name, line);
    }
    return ptr;
}