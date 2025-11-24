#ifndef FILE_HANDLE_H
#define FILE_HANDLE_H

#include <stdio.h>

/**
 * @brief Internal wrapper for fopen with error handling.
 * Use the xfopen() macro instead.
 */
FILE *_xfopen(const char *target_file_name, const char *modes, const char *func_name,
              const char *file_name, const int line);

// Macro to capture call site information
#define xfopen(f, m) _xfopen((f), (m), __func__, __FILE__, __LINE__)

/**
 * @brief Reads the entire content of a file into memory.
 * @param file_name Path to the file.
 * @return Dynamically allocated string containing file content (Caller must free).
 */
char *read_whole_file(const char *file_name);

/**
 * @brief Creates directories recursively and writes content to the file.
 * Equivalent to "mkdir -p" followed by writing the file.
 * @param path Full path including filename.
 * @param content Content to write to the file.
 */
void mk_dir_and_file(const char *path, const char *content);

/**
 * @brief Calculates the relative path of a file from the project root.
 * @param org_dir The directory where the user ran the command.
 * @param root_path The target path provided by the user.
 * @return Normalized relative path string (e.g., "src/main.c").
 */
char *relative_path_calc(const char *org_dir, const char *root_path);

#endif