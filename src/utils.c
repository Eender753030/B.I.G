#include "utils.h"

#include <stdlib.h>
#include <string.h>

#include "error_handle.h"

char *str_dup(const char *string) {
    char *new_string = (char *)malloc(strlen(string) + 1);
    if (new_string == NULL)
        ErrnoHandler(__func__, __FILE__, __LINE__);

    strcpy(new_string, string);

    return new_string;
}