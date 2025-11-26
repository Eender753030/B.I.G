#include "core/blob.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

static uint64_t hash_binary(const void *data, uint64_t len) {
    uint64_t hash = 5381;
    const uint8_t *data_ptr = data;

    for (uint64_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + data_ptr[i];
    }
    return hash;
}

char *blob_create_from_file(const char *path) {
    FILE *file = xfopen(path, "rb");

    fseek(file, 0, SEEK_END);
    uint64_t file_len = (uint64_t)ftell(file);
    rewind(file);

    char *content = xmalloc(file_len);
    if (file_len > 0) {
        if (fread(content, 1, file_len, file) != file_len) {
            fclose(file);
            ErrorCustomMsg("Failed to read file: %s", path);
        }
    }
    fclose(file);

    char *hash_str = hash_to_string(hash_binary(content, file_len));

    char object_path[4096];
    snprintf(object_path, sizeof(object_path), ".big/objects/%s", hash_str);

    if (access(object_path, F_OK) != 0) {
        FILE *obj_file = fopen(object_path, "wb");
        if (obj_file == NULL) {
            ErrorCustomMsg("Failed to write object file: %s", object_path);
        }
        fwrite(content, 1, file_len, obj_file);
        fclose(obj_file);
    }

    xfree(content);

    return hash_str;
}

char *blob_read_from_hash(const char *hash) {
    char object_path[4096];
    snprintf(object_path, sizeof(object_path), ".big/objects/%s", hash);

    if (access(object_path, F_OK) != 0) {
        return NULL;
    }

    return read_whole_file(object_path);
}