#ifndef BLOB_H
#define BLOB_H

#include <stdint.h>

// Create a blob (binary large object) from a file content and return it's hash
char *blob_create_from_file(const char *path);

// Scan whole file to get it's hash but no create
char *blob_get_file_hash(const char *path);

// Get file content from the blob, len is a optional value that can provide to user content length
char *blob_read_from_hash(const char *hash, uint64_t *len);

#endif