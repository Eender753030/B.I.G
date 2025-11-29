#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// Return codes for initialization checks
#define INITED 0
#define NOT_INIT -1

// Macro to suppress "unused variable" warnings during compilation
#define UNUSED(x) (void)(x)

/**
 * @brief Duplicates a string by allocating new memory.
 * @param string The source string.
 * @return Pointer to the new string (Caller must free it).
 */
char *str_dup(const char *string);

/**
 * @brief Checks if the current directory is inside a B.I.G repository.
 * Traverses upwards looking for the ".big" directory.
 * @return INITED if found, NOT_INIT otherwise.
 */
int check_init();

/**
 * @brief Changes the current working directory to the project root.
 * @param org_dir Optional pointer to store the original directory path before moving.
 */
void cd_to_project_root(char **org_dir);

/**
 * @brief Generates a hash using the DJB2 algorithm.
 * @param string Input string.
 * @return Unsigned long hash value.
 */
uint64_t hash_function(const char *string);

/**
 * @brief Converts a numeric hash to its hexadecimal string representation.
 * @param hash The hash value.
 * @return Pointer to the hex string (Caller must free it).
 */
char *hash_to_string(uint64_t hash);

char *datetime_now_to_str();

#endif