#ifndef FILE_HANDLE_H
#define FILE_HANDLE_H

#include <stdio.h>

FILE *_xfopen(const char *traget_file_name, const char *modes, const char *func_name,
              const char *file_name, const int line);

#define xfopen(f, m) _xfopen((f), (m), __func__, __FILE__, __LINE__)

char *read_whole_file(const char *file_name);

void mk_dir_and_file(const char *path, const char *content);

char *relative_path_calc(const char *org_dir, const char *root_path);

#endif