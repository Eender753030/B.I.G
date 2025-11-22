#ifndef CMD_H
#define CMD_H

typedef void (*cmd_func)(int argc, char *argv[]);

typedef struct {
    const char *cmd_name;
    cmd_func cmd;
} Command;

#endif