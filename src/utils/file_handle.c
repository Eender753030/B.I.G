#include "utils/file_handle.h"

#include <stdio.h>
#include <stdlib.h>

#include "utils/error_handle.h"

char *read_whole_file(const char *full_path) {
    FILE *file = fopen(full_path, "rb");
    if (file == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    fseek(file, 0, SEEK_END);
    size_t file_len = ftell(file);

    char *buffer = (char *)malloc(file_len + 1);
    if (buffer == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    rewind(file);

    size_t bytes_read = fread(buffer, 1, file_len, file);
    if (bytes_read != file_len)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    buffer[file_len] = '\0';

    fclose(file);

    return buffer;
}