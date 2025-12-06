#ifndef FILE_HANDLE_H
#define FILE_HANDLE_H

#include <stdint.h>
#include <stdio.h>

/* Internal fopen() handle
 * For error handle inside that can make me not need to type it every time
 * Use macro as API can pass outer file information instead of internal helper function's
 */
FILE *_xfopen(const char *target_file_name, const char *modes, const char *func_name,
              const char *file_name, const int line);

/* Internal ftell() handle
 * Aslo use macro as API
 */
long _xftell(FILE *file, const char *func_name, const char *file_name, const int line);

/* Internal getcwd() handle
 * Aslo use macro as API
 */
char *_xgetcwd(const char *func_name, const char *file_name, const int line);

/* Macro of above functions
 * Can pass GCC magic word from the caller
 */
#define xfopen(f, m) _xfopen((f), (m), __func__, __FILE__, __LINE__)
#define xftell(f) _xftell((f), __func__, __FILE__, __LINE__)
#define xgetcwd() _xgetcwd(__func__, __FILE__, __LINE__)

// Return file's length from FILE pointer
uint64_t get_file_len(FILE *file);

/* Read all content in a file from it's name
 * Length input can make sure binary file can correctly read
 */
char *read_whole_file(const char *file_name, uint64_t *len);

/* Create file and directory that the file inside
 * e.g. for src/utils/file_handle.h
 *      It will create src/ -> utils/ -> file_handle.h in the order
 */
void mk_dir_and_file(const char *path, const char *content, uint64_t content_len);

// Calculate the relative path from the project directory that .big/ locate
char *relative_path_calc(const char *org_dir, const char *root_path);

#endif