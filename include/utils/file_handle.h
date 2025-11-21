#ifndef FILE_HANDLE_H
#define FILE_HANDLE_H

char *read_whole_file(const char *full_path);

void mk_dir_and_file(const char *path, const char *content);

char *relative_path_calc(const char *org_dir, const char *root_path);

#endif