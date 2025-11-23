#ifndef UTILS_H
#define UTILS_H

#define INITED 0
#define NOT_ININ -1

#define UNUSED(x) (void)(x)

char *str_dup(const char *string);

int check_init();

void cd_to_project_root(char **org_dir);

unsigned long hash_function(const char *string);

char *hash_to_string(unsigned long hash);

#endif