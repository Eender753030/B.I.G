#ifndef ERROR_HANDLE_H
#define ERROR_HANDLE_H

/**
 * @brief Prints system error message based on errno and exits.
 * Typically used after system calls (malloc, open, etc.) fail.
 * @param func_name Name of the function where error occurred (__func__).
 * @param file_name Source file name (__FILE__).
 * @param line Line number (__LINE__).
 */
void ErrnoHandler(const char *func_name, const char *file_name, const int line);

/**
 * @brief Prints a formatted custom error message and exits.
 * Supports a subset of printf format specifiers (%s, %d).
 * @param msg Format string.
 * @param ... Variable arguments.
 */
void ErrorCustomMsg(const char *msg, ...);

void WarningCustomMsg(const char *msg, ...);

/**
 * @brief Prints usage instructions (help menu) and exits.
 */
void InputError();

/**
 * @brief Prints "Not initialized" error and exits.
 */
void NotInitError();

#endif