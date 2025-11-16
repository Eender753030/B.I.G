#ifndef ERROR_HANDLE_H
#define ERROR_HANDLE_H

void ErrnoHandler(const char *func_name, const char *file_name, const int line);

void InputError();

#endif