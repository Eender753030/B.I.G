#include "core/blob.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/error_handle.h"
#include "utils/file_handle.h"
#include "utils/memory.h"
#include "utils/utils.h"

// Aslo DJB2 hash algorithm but use length make sure not terminate by 0 value in binary file
static uint64_t hash_binary(const void *data, uint64_t len) {
    uint64_t hash = 5381;
    const uint8_t *data_ptr = data;

    for (uint64_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + data_ptr[i];
    }
    return hash;
}

// Create blob can make BST doesn't need to hold big content all the time that cause overhead
// Just need to hold the hash for future use
char *blob_create_from_file(const char *path) {
    FILE *file = xfopen(path, "rb");  // Use read binary mode that can read all types of file

    uint64_t file_len = get_file_len(file);  // get_file_len to malloc() enough space

    char *content = xmalloc(file_len);
    if (file_len > 0) {
        // Use fread with length make sure it will not terminate by 0 value in binary file
        // ('\0' is also 0)
        if (fread(content, 1, file_len, file) != file_len) {
            fclose(file);
            // If failed just warning
            error_custom_msg("Failed to read file: %s\n", path);
        }
    }
    fclose(file);

    // Get the hash string
    char *hash_str = hash_to_string(hash_binary(content, file_len));

    char object_path[4096];
    // Get the location of blob to store that in .big/objects
    snprintf(object_path, sizeof(object_path), ".big/objects/%s", hash_str);

    // Make sure objects/ is exist, if not, create a new one
    if (access(".big/objects", F_OK) != 0 && mkdir(".big/objects", 0775) == -1) {
        errno_handle(__func__, __FILE__, __LINE__);
    }

    // Write whole content into blob
    if (access(object_path, F_OK) != 0) {
        FILE *obj_file = fopen(object_path, "wb");  // Use write binary
        if (obj_file == NULL) {
            error_custom_msg("Failed to write object file: %s\n", object_path);
        }
        uint64_t write_bytes = fwrite(content, 1, file_len, obj_file);
        if (write_bytes != file_len) {
            fclose(obj_file);
            error_custom_msg("Error: Write object content failed\n");
        }
        fclose(obj_file);
    }

    xfree(content);

    return hash_str;
}

// Same as top but end when get the hash of content
char *blob_get_file_hash(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    uint64_t file_len = get_file_len(file);
    char *content = xmalloc(file_len + 1);

    if (file_len > 0) {
        if (fread(content, 1, file_len, file) != file_len) {
            fclose(file);
            xfree(content);
            return NULL;
        }
    }
    fclose(file);

    char *hash_str = hash_to_string(hash_binary(content, file_len));
    xfree(content);
    return hash_str;
}

char *blob_read_from_hash(const char *hash, uint64_t *len) {
    char object_path[4096];
    // Create the path to blob
    snprintf(object_path, sizeof(object_path), ".big/objects/%s", hash);

    // Make sure it is exist
    if (access(object_path, F_OK) != 0) {
        return NULL;
    }

    // Reture whole content in blob
    return read_whole_file(object_path, len);
}