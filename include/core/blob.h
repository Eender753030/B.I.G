#ifndef BLOB_H
#define BLOB_H

#include <stdint.h>

char *blob_create_from_file(const char *path);

char *blob_get_file_hash(const char *path);

char *blob_read_from_hash(const char *hash, uint64_t *len);

#endif