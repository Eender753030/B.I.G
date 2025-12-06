#ifndef ERROR_HANDLE_H
#define ERROR_HANDLE_H

/* Handle the error that has errno from the std headers
 *  Using GCC magic word
 * __func__ for current function name
 * __FILE__ for current file name
 * __LINE__ for current line in file
 */
void errno_handle(const char *func_name, const char *file_name, const int line);

/* Custom message error handle
 * Can print out format string with input args just like printf
 */
void error_custom_msg(const char *msg, ...);

/* Custom message warning handle
 * Just like errno_handle but not end the program
 */
void warning_custom_msg(const char *msg, ...);

/*
 * Unvalid input command handle
 * Print out the hint usage list to user
 */
void error_input();

/*
 * Not initialize directory error handle
 */
void error_not_init();

#endif