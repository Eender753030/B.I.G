#ifndef BLOB_H
#define BLOB_H

char *blob_create_from_file(const char *path);

char *blob_read_from_hash(const char *hash);

#endif