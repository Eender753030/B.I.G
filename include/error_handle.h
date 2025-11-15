#ifndef ERROR_HANDLE_H
#define ERROR_HANDLE_H

void MemoryError(const char *func_name, const char *file_name, const int line);

void InputError();

void MakeDirectoryError(const char *func_name, const char *file_name, const int line);

#endif