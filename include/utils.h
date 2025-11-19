#ifndef UTILS_H
#define UTILS_H

char *str_dup(const char *string);

void cd_to_project_root(char **org_dir);

unsigned long hash_function(char *string);

char *hash_to_string(unsigned long hash);

#endif