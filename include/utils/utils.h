#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

// Return codes for initialization checks
#define INITED true
#define NOT_INIT false

// Macro for cheat the compiler of not to use variable
#define UNUSED(x) (void)(x)

/* Because strdup is POSIX function, so I implement one by myself
 * It malloc and copy a new string from a string
 */
char *str_dup(const char *string);

// Check that working directory whether in project directory that has .big/ included
bool check_init();

// Change working directory to project directory that has .big/ included
void cd_to_project_root(char **org_dir);

// Generate a hash value from a string
uint64_t hash_function(const char *string);

// Return a string that is Hex from hash
char *hash_to_string(uint64_t hash);

// Return a format string of datetime
char *datetime_now_to_str();

#endif